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

#ifndef WRITEBACKJOB_H
#define WRITEBACKJOB_H
#include <kjob.h>
#include "writebackplugin.h"
#include <Nepomuk/Resource>

namespace Nepomuk {
class WritebackJob : public KJob
{
    Q_OBJECT
public:
    WritebackJob(QObject* parent = 0);
    ~WritebackJob();
    /**
      * Sets plugins as member variable of the class.
      */
    void setPlugins(const QList<WritebackPlugin*>& plugins);
    /**
      * Sets the resource as a member variable of the class.
      */
    void setResource(const Nepomuk::Resource & resource);
private:
    QList<WritebackPlugin*> m_plugins;
    Nepomuk::Resource m_resource;

public Q_SLOTS:
    /**
     * Starts the writeback job.
     */
    void start();

private Q_SLOTS:
    /**
      * Selects a plugin from the list and performs the writeback,
      * than connects with slotWritebackFinished.
      */
    void tryNextPlugin();
    /**
     * If there are more plugins avaliable, it will again call tryNextPlugin
     * to perform the writeback , else it will emitResult hence finishing the
     * job.
     */
    void slotWritebackFinished(Nepomuk::WritebackPlugin* plugin);
};
}


#endif // WRITEBACKJOB_H
