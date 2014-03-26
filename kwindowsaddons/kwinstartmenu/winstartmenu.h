/* This file is part of the KDE Project

   Copyright (C) 2008-2010 Ralf Habacker <ralf.habacker@freenet.de>
   All rights reserved.

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

#ifndef _WINSTARTMENU_H_
#define _WINSTARTMENU_H_

#include <kdedmodule.h>
#include <kservicegroup.h>
#include <kurl.h>

/**
 * KDED Module to create windows start menu entries for KDE applications
 * WinStartMenuModule implements a KDED Module that handles the creating of 
 * windows start menu entries for KDE application. 
 * 
 * On ksycoca database changes the modules will be triggered to create/update 
 * windows start menu entries 
 *
 * @short KDED Module for KDE Windows Start Menu creating
 * @author Ralf Habacker <ralf.habacker@freenet.de>
 */
class WinStartMenuModule : public KDEDModule
{
    Q_OBJECT
public:
    WinStartMenuModule(QObject* parent, const QList<QVariant>&);
    virtual ~WinStartMenuModule();

public Q_SLOTS: // dbus methods, called by the adaptor
    /**
     */
    void createStartMenuEntries();
    /**
     */
    void removeStartMenuEntries();

signals: // DBUS signals

private:

private Q_SLOTS:
    void databaseChanged();

private:
    KServiceGroup::Ptr findGroup(const QString &relPath);
//    bool createEntries(const KUrl &url, const QString &relPathTranslated=QString());
    struct WinStartMenuModulePrivate *d;
};

#endif

// vim: ts=4 sw=4 et
