/**
  * This file is part of the KDE project
  * Copyright (C) 2007, 2006 Rafael Fernández López <ereslibre@gmail.com>
  * Copyright (C) 2001 George Staikos <staikos@kde.org>
  * Copyright (C) 2000 Matej Koss <koss@miesto.sk>
  *                    David Faure <faure@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include <QWidget>
#include <QBoxLayout>
#include <QAction>
#include <QToolBar>
#include <QTabWidget>

#include <ksqueezedtextlabel.h>
#include <kconfig.h>
#include <kconfigdialog.h>
#include <kstandarddirs.h>
#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kdialog.h>
#include <ksystemtrayicon.h>
#include <kmenu.h>
#include <kaction.h>
#include <klineedit.h>
#include <kio/defaultprogress.h>
#include <kio/jobclasses.h>
#include <kjob.h>

#include "uiserver.h"
#include "uiserveradaptor_p.h"
#include "uiserveriface.h"
#include "progresslistmodel.h"
#include "progresslistdelegate.h"

#include "uiserver.h"

UIServer::UIServer()
    : KMainWindow(0)
{
    tabWidget = new QTabWidget();

    QString configure = i18n("Configure");

    toolBar = addToolBar(configure);
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QAction *configureAction = toolBar->addAction(configure);
    configureAction->setIcon(KIcon("configure"));
    configureAction->setIconText(configure);

    connect(configureAction, SIGNAL(triggered(bool)), this,
            SLOT(showConfigurationDialog()));

    toolBar->addSeparator();

    searchText = new KLineEdit(toolBar);
    searchText->setText(i18n("Search"));
    searchText->setClearButtonShown(true);

    toolBar->addWidget(searchText);

    listProgress = new QListView(tabWidget);
    listProgress->setObjectName("progresslist");
    listProgress->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    listFinished = new QListView(tabWidget);
    listFinished->setObjectName("progresslistFinished");
    listFinished->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    tabWidget->addTab(listProgress, i18n("In progress"));
    tabWidget->addTab(listFinished, i18n("Finished"));

    progressListModel = new ProgressListModel(this);
    progressListFinishedModel = new ProgressListModel(this);

    serverAdaptor = new UIServerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/UIServer"), this);

    listProgress->setModel(progressListModel);
    listFinished->setModel(progressListFinishedModel);

    setCentralWidget(tabWidget);

    progressListDelegate = new ProgressListDelegate(this, listProgress);
    progressListDelegate->setSeparatorPixels(10);
    progressListDelegate->setLeftMargin(10);
    progressListDelegate->setRightMargin(10);
    progressListDelegate->setProgressBarHeight(20);
    progressListDelegate->setMinimumItemHeight(100);
    progressListDelegate->setMinimumContentWidth(300);
    progressListDelegate->setEditorHeight(20);
    listProgress->setItemDelegate(progressListDelegate);

    progressListDelegateFinished = new ProgressListDelegate(this, listFinished);
    progressListDelegate->setSeparatorPixels(10);
    progressListDelegate->setLeftMargin(10);
    progressListDelegate->setRightMargin(10);
    progressListDelegate->setProgressBarHeight(20);
    progressListDelegate->setMinimumItemHeight(100);
    progressListDelegate->setMinimumContentWidth(300);
    progressListDelegate->setEditorHeight(20);
    listFinished->setItemDelegate(progressListDelegateFinished);

    connect(progressListModel, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            listProgress, SLOT(rowsInserted(const QModelIndex&,int,int)));

    connect(progressListModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            listProgress, SLOT(rowsAboutToBeRemoved(const QModelIndex&,int,int)));

    connect(progressListModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            this, SLOT(slotRowsRemoved(const QModelIndex&,int,int)));

    connect(progressListModel, SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
            listProgress, SLOT(dataChanged(const QModelIndex&,const QModelIndex&)));

    connect(progressListDelegate, SIGNAL(actionPerformed(int,int)), serverAdaptor,
            SIGNAL(actionPerformed(int,int)));

    connect(progressListFinishedModel, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            listFinished, SLOT(rowsInserted(const QModelIndex&,int,int)));

    connect(progressListFinishedModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            listFinished, SLOT(rowsAboutToBeRemoved(const QModelIndex&,int,int)));

    connect(progressListModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            this, SLOT(slotRowsRemoved(const QModelIndex&,int,int)));

    connect(progressListFinishedModel, SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
            listFinished, SLOT(dataChanged(const QModelIndex&,const QModelIndex&)));

    connect(progressListDelegateFinished, SIGNAL(actionPerformed(int,int)), serverAdaptor,
            SIGNAL(actionPerformed(int,int)));

    applySettings();

    hide();
}

UIServer::~UIServer()
{
}

UIServer* UIServer::createInstance()
{
    return new UIServer;
}

int UIServer::newJob(const QString &appServiceName, int capabilities, bool showProgress, const QString &internalAppName, const QString &jobIcon, const QString &appName)
{
    // Uncomment if you want to see kuiserver in action (ereslibre)
    // if (isHidden()) show();

    s_jobId++;

    progressListModel->newJob(s_jobId, internalAppName, jobIcon, appName, showProgress);
    progressListModel->setData(progressListModel->indexForJob(s_jobId), s_jobId,
                               ProgressListDelegate::JobId);

    m_jobTimesAdded.insert(s_jobId, 0);

    if (capabilities == KJob::NoCapabilities)
        return s_jobId;

    if (capabilities & KJob::Pausable)
        newAction(s_jobId, KJob::Pausable, i18n("Pause"));

    if (capabilities & KJob::Killable)
        newAction(s_jobId, KJob::Killable, i18n("Cancel"));

    return s_jobId;
}

void UIServer::jobFinished(int id)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    m_jobTimesAdded.remove(s_jobId);

    progressListModel->finishJob(id);
}


/// ===========================================================


void UIServer::newAction(int jobId, int actionId, const QString &actionText)
{
    m_hashActions.insert(actionId, jobId);

    progressListModel->newAction(jobId, actionId, actionText);
}

/// ===========================================================


void UIServer::totalSize(int id, KIO::filesize_t size)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), KIO::convertSize(size),
                               ProgressListDelegate::SizeTotals);
}

void UIServer::totalFiles(int id, unsigned long files)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), qulonglong(files),
                               ProgressListDelegate::FileTotals);
}

void UIServer::totalDirs(int id, unsigned long dirs)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), qulonglong(dirs),
                               ProgressListDelegate::DirTotals);
}

void UIServer::processedSize(int id, KIO::filesize_t bytes)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), KIO::convertSize(bytes),
                               ProgressListDelegate::SizeProcessed);
}

void UIServer::processedFiles(int id, unsigned long files)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), qulonglong(files),
                               ProgressListDelegate::FilesProcessed);
}

void UIServer::processedDirs(int id, unsigned long dirs)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), qulonglong(dirs),
                               ProgressListDelegate::DirsProcessed);
}

void UIServer::percent(int id, unsigned long ipercent)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), qulonglong(ipercent),
                               ProgressListDelegate::Percent);
}

void UIServer::speed(int id, QString bytes_per_second)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), bytes_per_second,
                               ProgressListDelegate::Speed);
}

void UIServer::infoMessage(int id, QString msg)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), msg,
                               ProgressListDelegate::Message);
}

void UIServer::progressInfoMessage(int id, QString msg)
{
    if ((id < 1) || !m_jobTimesAdded.contains(id)) return;

    progressListModel->setData(progressListModel->indexForJob(id), msg,
                               ProgressListDelegate::ProgressMessage);
}


/// ===========================================================


bool UIServer::copying(int id, QString from, QString to)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Copying"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), from,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), to,
                               ProgressListDelegate::To);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("From"),
                               ProgressListDelegate::FromLabel);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("To"),
                               ProgressListDelegate::ToLabel);

    return true;
}

bool UIServer::moving(int id, QString from, QString to)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Moving"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), from,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), to,
                               ProgressListDelegate::To);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("From"),
                               ProgressListDelegate::FromLabel);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("To"),
                               ProgressListDelegate::ToLabel);

    return true;
}

bool UIServer::deleting(int id, QString url)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Deleting"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), url,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("File"),
                               ProgressListDelegate::FromLabel);

    return true;
}

bool UIServer::transferring(int id, QString url)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Transferring"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), url,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("Source on"),
                               ProgressListDelegate::FromLabel);

    return true;
}

bool UIServer::creatingDir(int id, QString dir)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Creating directory"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), dir,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("New directory"),
                               ProgressListDelegate::FromLabel);

    return true;
}

bool UIServer::stating(int id, QString url)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Stating"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), url,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("Stating"),
                               ProgressListDelegate::FromLabel);

    return true;
}

bool UIServer::mounting(int id, QString dev, QString point)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Mounting device"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), dev,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), point,
                               ProgressListDelegate::To);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("Device"),
                               ProgressListDelegate::FromLabel);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("Mount point"),
                               ProgressListDelegate::ToLabel);

    return true;
}

bool UIServer::unmounting(int id, QString point)
{
    if ((id < 1) || (m_jobTimesAdded.contains(id) && m_jobTimesAdded[id])) return false;

    m_jobTimesAdded[id]++;

    QString delegateMessage(i18n("Unmounting device"));

    progressListModel->setData(progressListModel->indexForJob(id), delegateMessage,
                               ProgressListDelegate::Message);

    progressListModel->setData(progressListModel->indexForJob(id), point,
                               ProgressListDelegate::From);

    progressListModel->setData(progressListModel->indexForJob(id), i18n("Mount point"),
                               ProgressListDelegate::FromLabel);

    return true;
}

void UIServer::setJobVisible(int id, bool visible)
{
    listProgress->setRowHidden(progressListModel->indexForJob(id).row(), !visible);
}

void UIServer::slotRemoveSystemTrayIcon()
{
#ifdef __GNUC__
    #warning implement me (ereslibre)
#endif
    return;
}

void UIServer::updateConfiguration()
{
    Configuration::writeConfig();
}

void UIServer::applySettings()
{
     KSystemTrayIcon *m_systemTray = new KSystemTrayIcon(this);
     m_systemTray->setIcon(KSystemTrayIcon::loadIcon("display")); // found something better ? (ereslibre)
     m_systemTray->show();
}

void UIServer::showConfigurationDialog()
{
    if (KConfigDialog::showDialog("configuration"))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, "configuration",
                                              Configuration::self());

    UIConfigurationDialog *configurationUI = new UIConfigurationDialog(0);

    dialog->addPage(configurationUI, i18n("Behavior"), "display");

    connect(dialog, SIGNAL(settingsChanged(const QString&)), this,
            SLOT(updateConfiguration()));

    dialog->show();
}

void UIServer::slotRowsRemoved(const QModelIndex &parent, int start, int end)
{
    // TODO: unify all width's of progress bars
}

/// ===========================================================

UIConfigurationDialog::UIConfigurationDialog(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    adjustSize();
}

UIConfigurationDialog::~UIConfigurationDialog()
{
}

/// ===========================================================


int UIServer::s_jobId = 0;

extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
    KLocale::setMainCatalog("kdelibs");
    //  GS 5/2001 - I changed the name to "KDE" to make it look better
    //              in the titles of dialogs which are displayed.
    KAboutData aboutdata("kuiserver", I18N_NOOP("Progress Manager"),
                         "0.8", I18N_NOOP("KDE Progress Information UI Server"),
                         KAboutData::License_GPL, "(C) 2000-2005, David Faure & Matt Koss");
    aboutdata.addAuthor("David Faure",I18N_NOOP("Maintainer"),"faure@kde.org");
    aboutdata.addAuthor("Matej Koss",I18N_NOOP("Developer"),"koss@miesto.sk");
    aboutdata.addAuthor("Rafael Fernández López",I18N_NOOP("Developer"),"ereslibre@gmail.com");

    KCmdLineArgs::init( argc, argv, &aboutdata );
    // KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start())
    {
      kDebug(7024) << "kuiserver is already running!" << endl;
      return (0);
    }

    KUniqueApplication app;

    // This app is started automatically, no need for session management
    app.disableSessionManagement();
    app.setQuitOnLastWindowClosed( false );
    //app.dcopClient()->setDaemonMode( true );

    UIServer::createInstance();

    return app.exec();
}

#include "uiserver.moc"
#include "uiserver_p.moc"
