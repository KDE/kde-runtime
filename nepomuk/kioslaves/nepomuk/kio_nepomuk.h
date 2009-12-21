/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2008-2009 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _NEPOMUK_KIO_NEPOMUK_H_
#define _NEPOMUK_KIO_NEPOMUK_H_

#include <kio/slavebase.h>

namespace Nepomuk {
    class NepomukProtocol : public KIO::SlaveBase
    {
    public:
        NepomukProtocol( const QByteArray& poolSocket, const QByteArray& appSocket );
        ~NepomukProtocol();

        void listDir( const KUrl& url );
        void get( const KUrl& url );
        void stat( const KUrl& url );
        void mimetype( const KUrl& url );
        void del(const KUrl&, bool);

    private:
        enum Operation {
            Get,
            Stat,
            Other
        };
        bool ensureNepomukRunning();
        bool redirectUrl( const KUrl& url, Operation op = Other );
    };
}

#endif
