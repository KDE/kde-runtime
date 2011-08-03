/*
    Copyright (C) 2011  Smit Shah <who828@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef NEPOMUKEXAMPLESERVICE_H
#define NEPOMUKEXAMPLESERVICE_H
#include <Nepomuk/Service>
#include <Nepomuk/Resource>
#include <Nepomuk/Types/Property>
#include <QVariant>
#include <nepomuk/resourcewatcher.h>
#include <KService>

namespace Nepomuk {
/**
  * Service controlling the writeback
  */
class WriteBackService : public Service
{
    Q_OBJECT
    
public:
    WriteBackService( QObject * parent = 0, const QList<QVariant>& args = QList<QVariant>() );
    ~WriteBackService();

private:
/**
  * Makes plugin list and passes it to writebackjob along with the resource.
  */
    void performWriteback(const KService::List services,const Nepomuk::Resource & resource);

public Q_SLOTS:
    /**
      * This method is selects the plugin using either the mimetype or rdf type
      * than passes it to performWriteback method to do the actual writeback.
      */
    Q_SCRIPTABLE  void test( const Nepomuk::Resource & resource);
};
}

#endif // NEPOMUKWRITEBACKSERVICE_H
