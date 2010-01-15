/*
   Copyright 2008-2010 Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _NEPOMUK_KIO_NEPOMUK_H_
#define _NEPOMUK_KIO_NEPOMUK_H_

#include <kio/forwardingslavebase.h>

#include <Nepomuk/Resource>

namespace Nepomuk {
    class NepomukProtocol : public KIO::ForwardingSlaveBase
    {
    public:
        NepomukProtocol( const QByteArray& poolSocket, const QByteArray& appSocket );
        ~NepomukProtocol();

        void listDir( const KUrl& url );
        void get( const KUrl& url );
        void put( const KUrl& url, int permissions, KIO::JobFlags flags );
        void stat( const KUrl& url );
        void mimetype( const KUrl& url );
        void del(const KUrl&, bool);

    protected:
        /**
         * reimplemented from ForwardingSlaveBase
         */
        bool rewriteUrl( const KUrl& url, KUrl& newURL );

        void prepareUDSEntry( KIO::UDSEntry &entry,
                              bool listing = false ) const;

    private:
        enum Operation {
            Get,
            Stat,
            Other
        };
        Operation m_currentOperation;
        bool ensureNepomukRunning();

        /**
         * Creates an UDS entry for an arbitrary Nepomuk resource by relying on its properties.
         */
        KIO::UDSEntry statNepomukResource( const Nepomuk::Resource& res );
    };
}

#endif
