/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SYNCFILEIDENTIFIER_H
#define SYNCFILEIDENTIFIER_H

#include "resourceidentifier.h"
#include "changelog.h"
#include "identificationset.h"
#include "syncfile.h"

namespace Nepomuk {

    class SyncFileIdentifier : public Sync::ResourceIdentifier
    {
    public:
        SyncFileIdentifier( const SyncFile & sf );
        virtual ~SyncFileIdentifier();

        virtual void identifyAll();
        int id();

        ChangeLog convertedChangeLog();
        void load();

    protected:
        virtual bool runIdentification(const KUrl& uri);

    private:
        ChangeLog m_changeLog;
        IdentificationSet m_identificationSet;

        static int NextId;
        int m_id;

        Resource createNewResource(const Nepomuk::Sync::SyncResource& simpleRes) const;

    };
}

#endif // SYNCFILEIDENTIFIER_H
