/*
Copyright (C) 2011  Smit Shah <Who828@gmail.com>

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
#include "writebackplugin.h"
#include <KDebug>


class Nepomuk::WritebackPlugin::Private
{
public:
    Private( WritebackPlugin* parent)
        : q (parent)
    {
    }

private:
    WritebackPlugin* q;
};


Nepomuk::WritebackPlugin::WritebackPlugin(QObject* parent)
        : QObject(parent),
          d( new Private(this) )
{

}

Nepomuk::WritebackPlugin::~WritebackPlugin()
{
    delete d;
}


void Nepomuk::WritebackPlugin::writeback( const QUrl& url )
{
    doWriteback( url );
}


void Nepomuk::WritebackPlugin::emitFinished( bool status)
{
    emit finished( this , status);
}

#include "writebackplugin.moc"
