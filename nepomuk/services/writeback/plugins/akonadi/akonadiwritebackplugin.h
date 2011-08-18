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

#ifndef NEPOMUKWRITEBACKPLUGIN_H
#define NEPOMUKWRITEBACKPLUGIN_H
#include "writebackplugin.h"
#include <QString>

class KJob;
namespace Nepomuk
{
class AkonadiWritebackPlugin : public Nepomuk::WritebackPlugin
{
    Q_OBJECT
public:
    AkonadiWritebackPlugin(QObject* parent, const QList<QVariant>&);
    ~AkonadiWritebackPlugin();
public:
    void doWriteback(const QUrl& url);

public Q_SLOTS:
    /**
     * Fetches the payload of the given item , its called
     * using connect and it compares with the metadata
     * in nepomuk storage if there is any changes calls
     * modifyJob to change the metadata.
     */
    void fetchFinished (KJob* job);
    /**
      * Modifies the item and informs if it was a success
      * or a failure.
      */
    void modifyFinished (KJob *job);

};

}


#endif
