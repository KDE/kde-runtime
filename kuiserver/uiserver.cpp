/**
  * This file is part of the KDE project
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
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

#include "uiserver.h"
#include "uiserver_p.h"
#include "jobviewadaptor.h"
#include "jobviewserveradaptor.h"
#include "progresslistmodel.h"
#include "progresslistdelegate.h"

#include <QtGui/QWidget>
#include <QtGui/QAction>
#include <QtGui/QBoxLayout>
#include <QtGui/QCloseEvent>

#include <ksqueezedtextlabel.h>
#include <kconfig.h>
#include <kconfigdialog.h>
#include <kstandarddirs.h>
#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <ktabwidget.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kdialog.h>
#include <ksystemtrayicon.h>
#include <kmenu.h>
#include <kaction.h>
#include <klineedit.h>
#include <kio/jobclasses.h>
#include <kjob.h>

UIServer *UIServer::s_uiserver = 0;

UIServer::JobView::JobView(QObject *parent)
    : QObject(parent)
{
    m_objectPath.setPath(QString("/JobViewServer/JobView_%1").arg(s_jobId));

    new JobViewAdaptor(this);
    QDBusConnection::sessionBus().registerObject(m_objectPath.path(), this);
}

void UIServer::JobView::terminate(const QString &errorMessage)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    s_uiserver->m_progressListModel->setData(index, JobInfo::Cancelled, ProgressListDelegate::State);

    if (errorMessage.isNull())
    {
        s_uiserver->m_progressListFinishedModel->newJob(s_uiserver->m_progressListModel->data(index, ProgressListDelegate::ApplicationName).toString(),
                                                        s_uiserver->m_progressListModel->data(index, ProgressListDelegate::Icon).toString(),
                                                        s_uiserver->m_progressListModel->data(index, ProgressListDelegate::Capabilities).toInt(),
                                                        0);
    }

    s_uiserver->m_progressListModel->finishJob(this);

    QDBusConnection::sessionBus().unregisterObject(m_objectPath.path(), QDBusConnection::UnregisterTree);
}

void UIServer::JobView::setSuspended(bool suspended)
{
    QModelIndex currentIndex = s_uiserver->listProgress->currentIndex();

    s_uiserver->m_progressListModel->setData(currentIndex, suspended ? JobInfo::Suspended
                                                                     : JobInfo::Running, ProgressListDelegate::State);

    if (currentIndex.isValid() &&
        s_uiserver->listProgress->selectionModel()->isSelected(currentIndex))
    {
        JobView *jobView = s_uiserver->m_progressListModel->jobView(currentIndex);

        if (s_uiserver->m_progressListModel->state(currentIndex) == JobInfo::Running)
        {
            s_uiserver->pauseResumeButton->setText(i18n("Pause"));
            s_uiserver->pauseResumeButton->setIcon(KIcon("media-playback-pause"));
        }
        else
        {
            s_uiserver->pauseResumeButton->setText(i18n("Resume"));
            s_uiserver->pauseResumeButton->setIcon(KIcon("media-playback-start"));
        }
    }
}

void UIServer::JobView::setTotalAmount(qlonglong amount, const QString &unit)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (unit == "bytes") {
        s_uiserver->m_progressListModel->setData(index, amount ? KGlobal::locale()->formatByteSize(amount)
                                                               : QString(), ProgressListDelegate::SizeTotals);
    } else if (unit == "files") {
        s_uiserver->m_progressListModel->setData(index, amount ? i18np("%1 file", "%1 files", amount)
                                                               : QString(), ProgressListDelegate::SizeTotals);
    } else if (unit == "dirs") {
        s_uiserver->m_progressListModel->setData(index, amount ? i18np("%1 folder", "%1 folders", amount)
                                                               : QString(), ProgressListDelegate::SizeTotals);
    }
}

void UIServer::JobView::setProcessedAmount(qlonglong amount, const QString &unit)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (unit == "bytes") {
        s_uiserver->m_progressListModel->setData(index, amount ? KGlobal::locale()->formatByteSize(amount)
                                                               : QString(), ProgressListDelegate::SizeProcessed);
    } else if (unit == "files") {
        s_uiserver->m_progressListModel->setData(index, amount ? i18np("%1 file", "%1 files", amount)
                                                               : QString(), ProgressListDelegate::SizeProcessed);
    } else if (unit == "dirs") {
        s_uiserver->m_progressListModel->setData(index, amount ? i18np("%1 folder", "%1 folders", amount)
                                                               : QString(), ProgressListDelegate::SizeProcessed);
    }
}

void UIServer::JobView::setPercent(uint percent)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (index.isValid())
        s_uiserver->m_progressListModel->setData(index, percent, ProgressListDelegate::Percent);
}

void UIServer::JobView::setSpeed(qlonglong bytesPerSecond)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (index.isValid())
        s_uiserver->m_progressListModel->setData(index, bytesPerSecond ? KGlobal::locale()->formatByteSize(bytesPerSecond)
                                                                       : QString(), ProgressListDelegate::Speed);
}

void UIServer::JobView::setInfoMessage(const QString &infoMessage)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (index.isValid())
        s_uiserver->m_progressListModel->setData(index, infoMessage, ProgressListDelegate::Message);
}

bool UIServer::JobView::setDescriptionField(uint number, const QString &name, const QString &value)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (index.isValid())
        return s_uiserver->m_progressListModel->setDescriptionField(index, number, name, value);

    return false;
}

void UIServer::JobView::clearDescriptionField(uint number)
{
    QModelIndex index = s_uiserver->m_progressListModel->indexForJob(this);

    if (index.isValid())
        s_uiserver->m_progressListModel->clearDescriptionField(index, number);
}

QDBusObjectPath UIServer::JobView::objectPath() const
{
    return m_objectPath;
}

UIServer::UIServer()
    : KXmlGuiWindow(0)
{
    // Register necessary services and D-Bus adaptors.
    new JobViewServerAdaptor(this);
    QDBusConnection::sessionBus().registerService(QLatin1String("org.kde.JobViewServer"));
    QDBusConnection::sessionBus().registerObject(QLatin1String("/JobViewServer"), this);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    centralWidget->setLayout(layout);

    tabWidget = new KTabWidget(this);

    QString configure = i18n("Configure...");

    toolBar = addToolBar(configure);
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QAction *configureAction = toolBar->addAction(configure);
    configureAction->setIcon(KIcon("configure"));
    configureAction->setIconText(configure);

    connect(configureAction, SIGNAL(triggered(bool)), this,
            SLOT(showConfigurationDialog()));

    connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(const QString&,const QString&,const QString&)), this,
            SLOT(slotServiceOwnerChanged(const QString&,const QString&,const QString&)));


    toolBar->addSeparator();

    searchText = new KLineEdit(toolBar);
    searchText->setClickMessage(i18n("Search"));
    searchText->setClearButtonShown(true);

    toolBar->addWidget(searchText);

    listProgress = new QListView(tabWidget);
    listProgress->setAlternatingRowColors(true);
    listProgress->setObjectName("progresslist");
    listProgress->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    listFinished = new QListView(tabWidget);
    listFinished->setAlternatingRowColors(true);
    listFinished->setVisible(false);
    listFinished->setObjectName("progresslistFinished");
    listFinished->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    layout->addWidget(listProgress);

    QHBoxLayout *horizLayout = new QHBoxLayout;

    cancelButton = new KPushButton(KIcon("media-playback-stop"), i18n("Cancel"), this);
    pauseResumeButton = new KPushButton(KIcon("media-playback-pause"), i18n("Pause"), this);

    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(slotCancelClicked()));
    connect(pauseResumeButton, SIGNAL(clicked(bool)), this, SLOT(slotPauseResumeClicked()));

    cancelButton->setEnabled(false);
    pauseResumeButton->setEnabled(false);

    horizLayout->addStretch();
    horizLayout->addWidget(pauseResumeButton);
    horizLayout->addSpacing(KDialog::spacingHint());
    horizLayout->addWidget(cancelButton);

    layout->addLayout(horizLayout);

    tabWidget->addTab(centralWidget, i18n("In Progress"));
    //tabWidget->addTab(listFinished, i18n("Finished"));

    setCentralWidget(tabWidget);

    m_progressListModel = new ProgressListModel(this);
    m_progressListFinishedModel = new ProgressListModel(this);

    listProgress->setModel(m_progressListModel);
    listFinished->setModel(m_progressListFinishedModel);

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
    progressListDelegateFinished->setSeparatorPixels(10);
    progressListDelegateFinished->setLeftMargin(10);
    progressListDelegateFinished->setRightMargin(10);
    progressListDelegateFinished->setProgressBarHeight(20);
    progressListDelegateFinished->setMinimumItemHeight(100);
    progressListDelegateFinished->setMinimumContentWidth(300);
    progressListDelegateFinished->setEditorHeight(20);
    listFinished->setItemDelegate(progressListDelegateFinished);

    applySettings();

    connect(listProgress->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)), this,
            SLOT(slotSelectionChanged(const QItemSelection&)));

    hide();
}

UIServer::~UIServer()
{
}

UIServer* UIServer::createInstance()
{
    s_uiserver = new UIServer;
    return s_uiserver;
}

QDBusObjectPath UIServer::requestView(const QString &appName, const QString &appIconName, int capabilities)
{
    if (isHidden()) show();

    s_jobId++;

    // Since s_jobId is an unsigned int, if we got overflow and go back to 0,
    // be sure we do not assign 0 to a valid job, since 0 is reserved only for
    // reporting problems.
    if (!s_jobId) s_jobId++;

    JobView *jobView = new JobView();

    m_progressListModel->newJob(appName, appIconName, capabilities, jobView);

    return jobView->objectPath();
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
    Configuration::self()->writeConfig();
}

void UIServer::applySettings()
{
     KSystemTrayIcon *m_systemTray = new KSystemTrayIcon(this);
     m_systemTray->setIcon(KSystemTrayIcon::loadIcon("display")); // found something better ? (ereslibre)
     m_systemTray->show();
}

void UIServer::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
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
    dialog->enableButton(KDialog::Help, false);
    dialog->show();
}

void UIServer::slotServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    kDebug() << "dbus name: " << name << " oldowner: " << oldOwner << " newowner: " << newOwner;
}

void UIServer::slotCancelClicked()
{
    if (listProgress->currentIndex().isValid())
    {
        JobView *jobView = m_progressListModel->jobView(listProgress->currentIndex());

        emit jobView->cancelRequested();
    }
}

void UIServer::slotPauseResumeClicked()
{
    if (listProgress->currentIndex().isValid())
    {
        JobView *jobView = m_progressListModel->jobView(listProgress->currentIndex());

        if (s_uiserver->m_progressListModel->state(listProgress->currentIndex()) == JobInfo::Running)
        {
            emit jobView->suspendRequested();
            pauseResumeButton->setText(i18n("Resume"));
            pauseResumeButton->setIcon(KIcon("media-playback-start"));
        }
        else
        {
            emit jobView->resumeRequested();
            pauseResumeButton->setText(i18n("Pause"));
            pauseResumeButton->setIcon(KIcon("media-playback-pause"));
        }
    }
}

void UIServer::slotSelectionChanged(const QItemSelection &selection)
{
    bool enableCancelButton = false;
    bool enableResumeButton = false;

    if (selection.indexes().count())
    {
        JobView *jobView = m_progressListModel->jobView(listProgress->currentIndex());

        if ((m_progressListModel->state(listProgress->currentIndex()) == JobInfo::Running) &&
            ((m_progressListModel->data(listProgress->currentIndex(), ProgressListDelegate::Capabilities).toInt() &
              KJob::Suspendable)))
        {
            pauseResumeButton->setText(i18n("Pause"));
            pauseResumeButton->setIcon(KIcon("media-playback-pause"));
            enableResumeButton = true;
        }
        else if ((m_progressListModel->data(listProgress->currentIndex(), ProgressListDelegate::Capabilities).toInt() &
                  KJob::Suspendable))
        {
            pauseResumeButton->setText(i18n("Resume"));
            pauseResumeButton->setIcon(KIcon("media-playback-start"));
            enableResumeButton = true;
        }

        if ((m_progressListModel->data(listProgress->currentIndex(), ProgressListDelegate::Capabilities).toInt() &
             KJob::Killable))
        {
            enableCancelButton = true;
        }
    }

    cancelButton->setEnabled(enableCancelButton);
    pauseResumeButton->setEnabled(enableResumeButton);
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


uint UIServer::s_jobId = 0;

extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
    //  GS 5/2001 - I changed the name to "KDE" to make it look better
    //              in the titles of dialogs which are displayed.
    KAboutData aboutdata("kuiserver", "kdelibs4", ki18n("Progress Manager"),
                         "0.8", ki18n("KDE Progress Information UI Server"),
                         KAboutData::License_GPL, ki18n("(C) 2000-2008, KDE Team"));
    aboutdata.addAuthor(ki18n("Rafael Fernández López"),ki18n("Maintainer"),"ereslibre@kde.org");
    aboutdata.addAuthor(ki18n("David Faure"),ki18n("Former maintainer"),"faure@kde.org");
    aboutdata.addAuthor(ki18n("Matej Koss"),ki18n("Developer"),"koss@miesto.sk");

    KCmdLineArgs::init( argc, argv, &aboutdata );
    // KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start())
    {
      kDebug(7024) << "kuiserver is already running!";
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
