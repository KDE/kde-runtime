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

namespace Nepomuk {
/**
  * \class WritebackPlugin writebackplugin.h Nepomuk/WritebackPlugin
  *
  * \brief Base class for all metadata writeback plugins.
  *
  * The metadata writeback is intended to perfrom writeback opration
  * on a given resource provided if the plugin can support the given
  * RDF type.
  *
  * The plugins will based on WritebackPlugin class, thus they will be
  * coherent.
  *
  * To create a new plugin we just need to dervie the class from
  * Nepomuk::WritebackPlugin and export it as a plugin
  *
  * \code
  * class MyPlugin : public Nepomuk::WritebackPlugin
  * {
  *    // define the methods in this subclass
  * };
  * \endcode
  *
  * After this we will export it as a %Nepomuk Writeback plugin:
  *
  * \code
  * NEPOMUK_EXPORT_WRITEBACK_PLUGIN(MyPlugin,"mypluginservice")
  * \endcode
  *
  * A desktop file describes the plugin's properties:
  *
  * \code
  * [Desktop Entry]
  * Name=My Plugin
  * Name[x-test]=xxMy Pluginxx
  * Comment=NepomukPlugin which writeback to the metadata to a resource.
  * X-KDE-ServiceTypes=Nepomuk/WritebackPlugin
  * Type=Service
  * X-KDE-Library=myplugin
  * X-Nepomuk-ResourceTypes=*
  * \endcode
  *
  * \author Smit Shah <who828@gmail.org>
  */
class NEPOMUK_WRITEBACK_EXPORT WritebackPlugin : public QObject
{
    Q_OBJECT

public:
    /**
      * Create a new Plugin.
      *
      * \param parent The parent object
      */
    WritebackPlugin( QObject* parent=0);

    /**
      * Destructor
      */
    virtual ~WritebackPlugin();

Q_SIGNALS:
    /**
      * Emitted once the plugin has finished the writeback operation.
      *
      * It can't be emitted directly by subclasses of WritebackPlugin,
      * use emitFinished() instead.
      *
      * \param plugin The plugin emitting the signal.
      */
    void finished( Nepomuk::WritebackPlugin* plugin, bool status );

public Q_SLOTS:
    /**
     * This method is used to perform the writeback on the resource,
     * it calls the virtual method doWriteback to do the actual metadata
     * writing which is implemented in a subclass.
     *
     * \param url It requires the url pointing to the resource to perform
     * the writeback.
     */
    void writeback( const QUrl& url );

protected:
    /**
      * This is a virtaul method reimplemented by the subclass,
      * this metod actually performs the writeback , and once
      * the writeback is performed it will emit the finished
      * signal , hence the writeback is finished on the giving
      * resource.
      *
      * \param url It requires the url pointing to the resource to perform
      * the writeback.
      */
    virtual void doWriteback( const QUrl& url )=0;

protected Q_SLOTS:
    /**
      * Emits the finished signal.
      */
    void emitFinished(bool status);

private:
    class Private;
    Private* const d;
};
} // End namespace Nepomuk

/**
 * Export a %Nepomuk WritebackPlugin.
 *
 * \param classname The name of the  plugin to export.
 * \param libname The name of the library which should export the plugin.
 */
#define NEPOMUK_EXPORT_WRITEBACK_PLUGIN( classname, libname )  \
    K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)     \
    K_EXPORT_PLUGIN(factory(#libname))

#endif
