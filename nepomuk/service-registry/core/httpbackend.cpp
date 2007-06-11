/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "httpbackend.h"
#include "message.h"

Nepomuk::Backbone::Registry::Http::Backend::Backend( Core* core )
    : QObject( core ),
      Backend(),
      m_core( core )
{
}


Nepomuk::Backbone::Registry::Http::Backend::~Backend()
{
}


Nepomuk::Backbone::Result
Nepomuk::Backbone::Registry::Http::Backend::methodCall( const Nepomuk::Message& message )
{
    //
    // 1. Open socket to the http service
    // 2. construct http command (GET?)
    // 3. Parse reply
    //

}

#include "httpbackend.moc"
