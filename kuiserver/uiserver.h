/**
  * This file is part of the KDE project
  * Copyright (C) 2007, 2006 Rafael Fernández López <ereslibre@gmail.com>
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

#ifndef UISERVER_H
#define UISERVER_H

#include <QTimer>
#include <QDateTime>
#include <QListView>
#include <QPersistentModelIndex>
#include <kmainwindow.h>
#include <ksslcertdialog.h>
#include <kio/global.h>
#include <kio/authinfo.h>
#include <kurl.h>
#include <kmainwindow.h>
#include <k3listview.h>
#include <ksslcertdialog.h>
#include <ktoolbar.h>

#include "uiserver_p.h"
#include <kuiserversettings.h>

class ProgressListModel;
class ProgressListDelegate;
class UIServerAdaptor;
class QToolBar;
class QTabWidget;
class KLineEdit;
class OrgKdeKIOObserverInterface;

class UIServer
    : public KMainWindow
{
  Q_OBJECT

  UIServer();

  virtual ~UIServer();

public:
   static UIServer* createInstance();

    /**
      * Signal a new job
      *
      * @param appServiceName   the DBUS service name
      * @see                    KIO::Observer::newJob
      * @param capabilities		the capabilities that this job accepts
      * @param showProgress     whether to popup the progress for the job.
      *                         Usually true, but may be false when we use kuiserver for
      *                         other things, like SSL dialogs.
      * @param internalAppName  the application name that launched the job (kopete, konqueror...)
      * @param jobIcon          the job icon name
      * @param appName          the translated application name (Kopete, Konqueror...)
      * @return                 the identification number of the job (jobId)
      */
    int newJob(const QString &appServiceName, int capabilities, bool showProgress, const QString &internalAppName, const QString &jobIcon, const QString &appName);

    /**
      * Finishes a job
      *
      * @param jobId the identification number of the job
      */
    void jobFinished(int jobId);

    /**
      * Adds an action (button) to a job
      *
      * @param jobId        the identification number of the job in which the action will be added
      * @param actionId     the identification number of the action
      * @param actionText   the button text
      * @return             the identification number of the action (actionId)
      */
    void newAction(int jobId, int actionId, const QString &actionText);

    /**
      * Sets the total size of a job
      *
      * @param jobId    the identification number of the job
      * @param size     the total size
      */
    void totalSize(int jobId, KIO::filesize_t size);

    /**
      * Sets the total files of a job
      *
      * @param jobId    the identification number of the job
      * @param files    total files to be processed
      */
    void totalFiles(int jobId, unsigned long files);

    /**
      * Sets the total directories of a job
      *
      * @param jobId    the identification number of the job
      * @param dirs     total dirs to be processed
      */
    void totalDirs(int jobId, unsigned long dirs);

    /**
      * Sets the current processed size
      *
      * @param jobId    the identification number of the job
      * @param bytes    the current processed bytes
      */
    void processedSize(int jobId, KIO::filesize_t bytes);

    /**
      * Sets the current processed files
      *
      * @param jobId    the identification number of the job
      * @param files    the current processed files
      */
    void processedFiles(int jobId, unsigned long files);

    /**
      * Sets the current processed dirs
      *
      * @param jobId    the identification number of the job
      * @param dirs     the current processed dirs
      */
    void processedDirs(int jobId, unsigned long dirs);

    /**
      * Sets the current percent
      *
      * @param jobId    the identification number of the job
      * @param ipercent the current percent
      */
    void percent(int jobId, unsigned long ipercent);

    /**
      * Sets the current speed transfer
      *
      * @param jobId            the identification number of the job
      * @param bytes_per_second number of bytes per second on the transfer
      */
    void speed(int jobId, QString bytes_per_second);

    /**
      * Sets the current info message
      *
      * @param jobId    the identification number of the job
      * @param msg      the info message to show
      */
    void infoMessage(int jobId, QString msg);

    /**
      * Sets the current progress info message
      *
      * @param jobId    the identification number of the job
      * @param msg      the progress info message to show
      */
    void progressInfoMessage(int jobId, QString msg);

    /**
      * Starts a copying progress
      *
      * @param jobId    the identification number of the job
      * @param from     the source of the copying
      * @param to       the destination of copying transfer
      * @return         whether it was added to the list/tree or not
      */
    bool copying(int jobId, QString from, QString to);

    /**
      * Starts a moving progress
      *
      * @param jobId    the identification number of the job
      * @param from     the source of the moving
      * @param to       the destination of the moving
      * @return         whether it was added to the list/tree or not
      */
    bool moving(int jobId, QString from, QString to);

    /**
      * Starts a deleting progress
      *
      * @param jobId    the identification number of the job
      * @param url      the path that is going to be deleted
      * @return         whether it was added to the list/tree or not
      */
    bool deleting(int jobId, QString url);

    /**
      * Starts a transferring progress
      *
      * @param jobId    the identification number of the job
      * @param url      the path that is going to be transferred
      * @return         whether it was added to the list/tree or not
      */
    bool transferring(int jobId, QString url);

    /**
      * Starts a dir creation progress
      *
      * @param jobId    the identification number of the job
      * @param dir      the path where the directory is going to be created
      * @return         whether it was added to the list/tree or not
      */
    bool creatingDir(int jobId, QString dir);

    /**
      * Starts a stating progress
      *
      * @param jobId    the identification number of the job
      * @param url      the path that is going to be stated
      * @return         whether it was added to the list/tree or not
      */
    bool stating(int jobId, QString url);

    /**
      * Starts a mounting progress
      *
      * @param jobId    the identification number of the job
      * @param dev      the device that is going to be mounted
      * @param point    the mount point where the device will be mounted
      * @return         whether it was added to the list/tree or not
      */
    bool mounting(int jobId, QString dev, QString point);

    /**
      * Starts an unmounting progress
      *
      * @param jobId    the identification number of the job
      * @param point    the mount point that is going to be unmounted
      * @return         whether it was added to the list/tree or not
      */
    bool unmounting(int jobId, QString point);

    /**
      * Sets a job visible or hidden
      *
      * @param jobId    the job that will be hidden or shown
      * @param visible  whether the job will be shown or not
      */
    void setJobVisible(int jobId, bool visible);

public Q_SLOTS:
    void slotRemoveSystemTrayIcon();
    void updateConfiguration();
    void applySettings();

private Q_SLOTS:
    void showConfigurationDialog();
    void slotRowsRemoved(const QModelIndex &parent, int start, int end);

private:
    ProgressListModel *progressListModel;
    ProgressListModel *progressListFinishedModel;
    ProgressListDelegate *progressListDelegate;
    ProgressListDelegate *progressListDelegateFinished;
    QListView *listProgress;
    QListView *listFinished;
    QTabWidget *tabWidget;

    QToolBar *toolBar;
    KLineEdit *searchText;
    UIServerAdaptor *serverAdaptor;
    QHash<int, int> m_hashActions;
    QHash<int, OrgKdeKIOObserverInterface*> m_hashObserverInterfaces;

    static int s_jobId;
};

#endif // UISERVER_H
