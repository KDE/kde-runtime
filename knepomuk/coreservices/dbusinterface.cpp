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

#include "dbusinterface.h"
#include "sopranordfrepository.h"

#include <QCoreApplication>


class Nepomuk::CoreServices::DBusInterface::Private
{
public:
    Nepomuk::CoreServices::SopranoRDFRepository* rep;
};

Nepomuk::CoreServices::DBusInterface::DBusInterface( QObject* parent, SopranoRDFRepository* rep )
    : QDBusAbstractAdaptor( parent )
{
    d = new Private;
    d->rep = rep;
}


Nepomuk::CoreServices::DBusInterface::~DBusInterface()
{
    delete d;
}


QStringList Nepomuk::CoreServices::DBusInterface::dumpGraph( const QString& graph )
{
    return d->rep->dumpGraph( graph );
}


void Nepomuk::CoreServices::DBusInterface::quit()
{
    QCoreApplication::instance()->quit();
}


Soprano::Statement Nepomuk::CoreServices::DBusInterface::testStatement()
{
    return Soprano::Statement();
}

Soprano::Node Nepomuk::CoreServices::DBusInterface::testNode()
{
    return Soprano::Node();
}

#include "dbusinterface.moc"
