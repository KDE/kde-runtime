/**
  * This file is part of the KDE project
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
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

#include <QtGui/QListView>
#include <QtDBus/QDBusObjectPath>

#include <kxmlguiwindow.h>
#include <kio/global.h>
#include <kio/authinfo.h>
#include <kurl.h>
#include <ktoolbar.h>

#include <kuiserversettings.h>

class ProgressListModel;
class ProgressListDelegate;
class JobViewServerAdaptor;
class QToolBar;
class KTabWidget;
class KLineEdit;
class KPushButton;
class KSystemTrayIcon;
class OrgKdeJobViewInterface;
class JobViewAdaptor;

class UIServer
    : public KXmlGuiWindow
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.JobViewServer")

public:
    class JobView;

    static UIServer* createInstance();

    QDBusObjectPath requestView(const QString &appName, const QString &appIconName, int capabilities);

public Q_SLOTS:
    void updateConfiguration();
    void applySettings();

protected:
    virtual void closeEvent(QCloseEvent *event);

private Q_SLOTS:
    void showConfigurationDialog();
    void slotServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

private:
    UIServer();
    virtual ~UIServer();

    ProgressListModel *m_progressListModel;
    ProgressListModel *m_progressListFinishedModel;
    ProgressListDelegate *progressListDelegate;
    ProgressListDelegate *progressListDelegateFinished;
    QListView *listProgress;
    QListView *listFinished;
    KTabWidget *tabWidget;

    QToolBar *toolBar;
    KLineEdit *searchText;
    KSystemTrayIcon *m_systemTray;

    static uint s_jobId;
    static UIServer *s_uiserver;

    friend class JobView;
};

class UIServer::JobView
    : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.JobView")

public:
    JobView(QObject *parent = 0);
    ~JobView();

    void terminate(const QString &errorMessage);
    void setSuspended(bool suspended);
    void setTotalAmount(qlonglong amount, const QString &unit);
    void setProcessedAmount(qlonglong amount, const QString &unit);
    void setPercent(uint percent);
    void setSpeed(qlonglong bytesPerSecond);
    void setInfoMessage(const QString &infoMessage);
    bool setDescriptionField(uint number, const QString &name, const QString &value);
    void clearDescriptionField(uint number);

    QDBusObjectPath objectPath() const;

Q_SIGNALS:
    void suspendRequested();
    void resumeRequested();
    void cancelRequested();

private:
    QDBusObjectPath m_objectPath;

    friend class ProgressListDelegate;
};

#endif // UISERVER_H
