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
#ifndef WRITEBACKPLUGIN_H
#define WRITEBACKPLUGIN_H


#include <QtCore/QObject>
#include<QString>

#include "nepomukwriteback_export.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>

namespace Soprano {
class Node;
}

namespace Nepomuk {

class NEPOMUK_WRITEBACK_EXPORT WritebackPlugin : public QObject
{
    Q_OBJECT

public:
    WritebackPlugin( QObject* parent);
    virtual ~WritebackPlugin();

Q_SIGNALS:
    // to signal that writeback is finished
    void finished();

    //private:
    // emited by plugin once plugin is done, can't be called directly must use emitfinished // in public.
    //void finished( Nepomuk::WritebackPlugin* plugin );

public Q_SLOTS:

    void writeback( const QUrl& url );

protected:
    // its declared as virtual as subclasses will reimplement it based on what type of //file it is.
    virtual void doWriteback( const QUrl& url )=0;

protected Q_SLOTS:
    // emit finish signal by the subclass to clarify that writeback is over.
    void emitFinished();

private:
    class Private;
    Private* const d;

};

} // End namespace Nepomuk


#define NEPOMUK_EXPORT_WRITEBACK_PLUGIN( classname, libname )  \
    K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)     \
    K_EXPORT_PLUGIN(factory(#libname))

#endif
