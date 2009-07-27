/*
   This file is part of the KDE libraries
   Copyright (c) 2009 Till Adam <adam@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KTIMEZONEDBASE_H
#define KTIMEZONEDBASE_H

#include <kdedmodule.h>


class KTimeZonedBase : public KDEDModule
{
        Q_OBJECT
    public:
        KTimeZonedBase(QObject* parent, const QList<QVariant>&)
        :KDEDModule(parent) {}
        virtual ~KTimeZonedBase() {};

    public Q_SLOTS:
        /** D-Bus call to initialize the module.
         *  @param reinit determines whether to reinitialize if the module has already
         *                initialized itself
         */
        Q_SCRIPTABLE void initialize(bool reinit)
        {
            // If we reach here, the module has already been constructed and therefore
            // initialized. So only do anything if reinit is true.
            if (reinit)
                init(true);
        }

    Q_SIGNALS:
        /** D-Bus signal emitted when the time zone configuration in the registry has changed. */
        void configChanged();
    protected:
        virtual void init(bool) = 0;

        QString     mLocalZone;         // local system time zone name
        QString     mConfigLocalZone;   // local system time zone name as stored in config file
};


#endif
