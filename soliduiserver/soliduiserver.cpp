/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>
   Copyright (c) 2007 Alexis Ménard <darktears31@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "soliduiserver.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QByteArray>
#include <QLinkedList>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusError>
#include <kjobuidelegate.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>
#include <kstandarddirs.h>
#include <kdesktopfileactions.h>
#include <kwindowsystem.h>
#include <kpassworddialog.h>

#include "deviceactionsdialog.h"
#include "deviceaction.h"
#include "deviceserviceaction.h"
#include "devicenothingaction.h"


SolidUiServer::SolidUiServer() : KDEDModule()
{
}

SolidUiServer::~SolidUiServer()
{
}

void SolidUiServer::showActionsDialog(const QString &udi,
                                      const QStringList &desktopFiles)
{
    if (m_udiToActionsDialog.contains(udi)) {
        DeviceActionsDialog *dialog = m_udiToActionsDialog[udi];
        dialog->activateWindow();
        return;
    }


    QList<DeviceAction*> actions;

    foreach (QString desktop, desktopFiles) {
        QString filePath = KStandardDirs::locate("data", "solid/actions/"+desktop);

        QList<KDesktopFileActions::Service> services
            = KDesktopFileActions::userDefinedServices(filePath, true);

        foreach (KDesktopFileActions::Service service, services) {
            DeviceServiceAction *action = new DeviceServiceAction();
            action->setService(service);
            actions << action;
        }
    }
    actions << new DeviceNothingAction();

    DeviceActionsDialog *dialog = new DeviceActionsDialog();
    dialog->setDevice(Solid::Device(udi));
    dialog->setActions(actions);

    connect(dialog, SIGNAL(finished()),
            this, SLOT(onActionDialogFinished()));

    m_udiToActionsDialog[udi] = dialog;

    // Update user activity timestamp, otherwise the notification dialog will be shown
    // in the background due to focus stealing prevention. Entering a new media can
    // be seen as a kind of user activity after all. It'd be better to update the timestamp
    // as soon as the media is entered, but it apparently takes some time to get here.
    kapp->updateUserTimestamp();

    dialog->show();
}

void SolidUiServer::onActionDialogFinished()
{
    DeviceActionsDialog *dialog = qobject_cast<DeviceActionsDialog*>(sender());

    if (dialog) {
        QString udi = dialog->device().udi();
        m_udiToActionsDialog.remove(udi);
    }
}


void SolidUiServer::showPassphraseDialog(const QString &udi,
                                         const QString &returnService, const QString &returnObject,
                                         WId wId, const QString &appId)
{
    if (m_idToPassphraseDialog.contains(returnService+":"+udi)) {
        KPasswordDialog *dialog = m_idToPassphraseDialog[returnService+":"+udi];
        dialog->activateWindow();
        return;
    }

    Solid::Device device(udi);

    KPasswordDialog *dialog = new KPasswordDialog();

    QString label = device.vendor();
    if (!label.isEmpty()) label+=" ";
    label+= device.product();

    dialog->setPrompt(i18n("'%1' needs a password to be accessed. Please enter a password.", label));
    dialog->setPixmap(KIcon(device.icon()).pixmap(64, 64));
    dialog->setProperty("soliduiserver.udi", udi);
    dialog->setProperty("soliduiserver.returnService", returnService);
    dialog->setProperty("soliduiserver.returnObject", returnObject);

    connect(dialog, SIGNAL(gotPassword(const QString&, bool)),
            this, SLOT(onPassphraseDialogCompleted(const QString&, bool)));
    connect(dialog, SIGNAL(rejected()),
            this, SLOT(onPassphraseDialogRejected()));

    m_idToPassphraseDialog[returnService+":"+udi] = dialog;

    reparentDialog(dialog, wId, appId, true);
    dialog->show();
}

void SolidUiServer::onPassphraseDialogCompleted(const QString &pass, bool keep)
{
    KPasswordDialog *dialog = qobject_cast<KPasswordDialog*>(sender());

    if (dialog) {
        QString returnService = dialog->property("soliduiserver.returnService").toString();
        QString returnObject = dialog->property("soliduiserver.returnObject").toString();
        QDBusInterface returnIface(returnService, returnObject);

        QDBusReply<void> reply = returnIface.call("passphraseReply", pass);

        QString udi = dialog->property("soliduiserver.udi").toString();
        m_idToPassphraseDialog.remove(returnService+":"+udi);

        if (!reply.isValid()) {
            kWarning() << "Impossible to send the passphrase to the application, D-Bus said: "
                       << reply.error().name() << ", " << reply.error().message() << endl;
        }
    }
}

void SolidUiServer::onPassphraseDialogRejected()
{
    onPassphraseDialogCompleted(QString(), false);
}

void SolidUiServer::reparentDialog(QWidget *dialog, WId wId, const QString &appId, bool modal)
{
    // Code borrowed from kwalletd

    KWindowSystem::setMainWindow(dialog, wId); // correct, set dialog parent

#ifdef Q_WS_X11
    if (modal) {
        KWindowSystem::setState(dialog->winId(), NET::Modal);
    } else {
        KWindowSystem::clearState(dialog->winId(), NET::Modal);
    }
#endif

    // allow dialog activation even if it interrupts, better than trying hacks
    // with keeping the dialog on top or on all desktops
    kapp->updateUserTimestamp();
}


#if 0
void SolidUiServer::onMediumChange(const QString &name, bool allowNotification)
{
    kDebug() << "SolidUiServer::onMediumChange(" << name << ", "
             << allowNotification << ")" << endl;

// Update user activity timestamp, otherwise the notification dialog will be shown
// in the background due to focus stealing prevention. Entering a new media can
// be seen as a kind of user activity after all. It'd be better to update the timestamp
// as soon as the media is entered, but it apparently takes some time to get here.
    kapp->updateUserTimestamp();

    KUrl url("system:/media/"+name);

    KIO::SimpleJob *job = KIO::stat(url, false);
    job->setUiDelegate(0);

    m_allowNotificationMap[job] = allowNotification;

    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotStatResult(KJob*)));
}

void SolidUiServer::slotStatResult(KJob *job)
{
    bool allowNotification = m_allowNotificationMap[job];
    m_allowNotificationMap.remove(job);

    if (job->error() != 0) return;

    KIO::StatJob *stat_job = static_cast<KIO::StatJob *>(job);

    KIO::UDSEntry entry = stat_job->statResult();
    KUrl url = stat_job->url();

    KFileItem medium(entry, url);

    if (autostart(medium)) return;
}

bool SolidUiServer::autostart(const KFileItem &medium)
{
    QString mimetype = medium.mimetype();

    bool is_cdrom = mimetype.contains("cd") || mimetype.contains("dvd");
    bool is_mounted = mimetype.endsWith("_mounted");

    // We autorun only on CD/DVD or removable disks (USB, Firewire)
    if (!(is_cdrom && is_mounted)
        && mimetype!="media/removable_mounted") {
        return false;
    }


    // Here starts the 'Autostart Of Applications After Mount' implementation


    // From now we're sure the medium is already mounted.
    // We can use the local path for stating, no need to use KIO here.
    bool local;
    QString path = medium.mostLocalUrl(local).path(); // local is always true here...

    // When a new medium is mounted the root directory of the medium should
    // be checked for the following Autostart files in order of precedence:
    // .autorun, autorun, autorun.sh
    QStringList autorun_list;
    autorun_list << ".autorun" << "autorun" << "autorun.sh";

    QStringList::iterator it = autorun_list.begin();
    QStringList::iterator end = autorun_list.end();

    for (; it!=end; ++it) {
        if (QFile::exists(path + '/' + *it)) {
            return execAutorun(medium, path, *it);
        }
    }

    // When a new medium is mounted the root directory of the medium should
    // be checked for the following Autoopen files in order of precedence:
    // .autoopen, autoopen
    QStringList autoopen_list;
    autoopen_list << ".autoopen" << "autoopen";

    it = autoopen_list.begin();
    end = autoopen_list.end();

    for (; it!=end; ++it) {
        if (QFile::exists(path + '/' + *it)) {
            return execAutoopen(medium, path, *it);
        }
    }

    return false;
}

bool SolidUiServer::execAutorun(const KFileItem &medium, const QString &path,
                                const QString &autorunFile)
{
    // The desktop environment MUST prompt the user for confirmation
    // before automatically starting an application.
    QString mediumType = medium.mimeTypePtr()->name();
    QString text = i18n("An autorun file as been found on your '%1'."
                        " Do you want to execute it?\n"
                        "Note that executing a file on a medium may compromise"
                        " your system's security", mediumType);
    QString caption = i18n("Autorun - %1", medium.url().prettyUrl());
    KGuiItem yes = KStandardGuiItem::yes();
    KGuiItem no = KStandardGuiItem::no();

    int answer = KMessageBox::warningYesNo(0L, text, caption, yes, no,
                                           QString(), KMessageBox::Notify | KMessageBox::Dangerous);

    if (answer == KMessageBox::Yes) {
        // When an Autostart file has been detected and the user has
        // confirmed its execution the autostart file MUST be executed
        // with the current working directory (CWD) set to the root
        // directory of the medium.
        QProcess::startDetached(QLatin1String("sh"), QStringList(autorunFile), path, 0);
    }

    return true;
}

bool SolidUiServer::execAutoopen(const KFileItem &medium, const QString &path,
                                 const QString &autoopenFile)
{
    // An Autoopen file MUST contain a single relative path that points
    // to a non-executable file contained on the medium. [...]
    QFile file(path + '/' + autoopenFile);
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);

    QString relative_path = stream.readLine().trimmed();

    // The relative path MUST NOT contain path components that
    // refer to a parent directory (../)
    if (relative_path.startsWith("/") || relative_path.contains("../")) {
        return false;
    }

    // The desktop environment MUST verify that the relative path points
    // to a file that is actually located on the medium [...]
    QString resolved_path
        = KStandardDirs::realFilePath(path + '/' + relative_path);

    if (!resolved_path.startsWith(path)) {
        return false;
    }


    QFile document(resolved_path);

    // TODO: What about FAT all files are executable...
    // If the relative path points to an executable file then the desktop
    // environment MUST NOT execute the file.
    if (!document.exists() /*|| QFileInfo(document).isExecutable()*/) {
        return false;
    }

    KUrl url = medium.url();
    url.addPath(relative_path);

    // The desktop environment MUST prompt the user for confirmation
    // before opening the file.
    QString mediumType = medium.mimeTypePtr()->name();
    QString filename = url.fileName();
    QString text = i18n("An autoopen file as been found on your '%1'."
                        " Do you want to open '%2'?\n"
                        "Note that opening a file on a medium may compromise"
                        " your system's security", mediumType, filename);
    QString caption = i18n("Autoopen - %1", medium.url().prettyUrl());
    KGuiItem yes = KStandardGuiItem::yes();
    KGuiItem no = KStandardGuiItem::no();

    int answer = KMessageBox::warningYesNo(0L, text, caption, yes, no,
                                           QString(), KMessageBox::Notify | KMessageBox::Dangerous);

    // TODO: Take case of the "UNLESS" part?
    // When an Autoopen file has been detected and the user has confirmed
    // that the file indicated in the Autoopen file should be opened then
    // the file indicated in the Autoopen file MUST be opened in the
    // application normally preferred by the user for files of its kind
    // UNLESS the user instructed otherwise.
    if (answer == KMessageBox::Yes) {
        (void) new KRun(url, 0L);
    }

    return true;
}
#endif

extern "C"
{
    KDE_EXPORT KDEDModule *create_soliduiserver()
    {
        return new SolidUiServer();
    }
}

#include "soliduiserver.moc"
