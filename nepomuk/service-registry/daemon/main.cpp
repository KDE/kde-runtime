/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "../core/core.h"
#include "../core/dbusbackend.h"

#include <QtCore/QCoreApplication>
#include <KComponentData>


int main(int argc, char **argv)
{
    QCoreApplication app( argc, argv );

    KComponentData data( "Nepomuk Daemon" );

    Nepomuk::Middleware::Registry::Core* core = new Nepomuk::Middleware::Registry::Core( &app );
    (void)new Nepomuk::Middleware::Registry::DBus::Backend( core );

    return app.exec();
}
