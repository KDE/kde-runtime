/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_STORAGE_H_
#define _NEPOMUK_STORAGE_H_

#include <Nepomuk/Service>

namespace Nepomuk {

    class Core;

    class Storage : public Service
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.Storage" )

    public:
        Storage( QObject* parent, const QList<QVariant>& args = QList<QVariant>() );
        ~Storage();

    public Q_SLOTS:
        Q_SCRIPTABLE void optimize( const QString& repo );
        Q_SCRIPTABLE QString usedSopranoBackend() const;

    private Q_SLOTS:
        void slotNepomukCoreInitialized( bool success );

    private:
        Nepomuk::Core* m_core;
    };
}

#endif
