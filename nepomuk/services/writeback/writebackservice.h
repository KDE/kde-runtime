/*
    <one line to give the program's name and a brief idea of what it does.>
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

namespace Nepomuk {

    class WriteBackService : public Service
    {
        Q_OBJECT
    
    public:
        WriteBackService( QObject * parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~WriteBackService();

    public Q_SLOTS:
       Q_SCRIPTABLE void test( const QString& url);
      
    };

}

#endif // NEPOMUKWRITEBACKSERVICE_H
