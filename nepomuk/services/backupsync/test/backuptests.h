/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

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


#ifndef BACKUPTESTS_H
#define BACKUPTESTS_H

#include <Nepomuk/testbase.h>
#include "backupmanager.h"

namespace Soprano {
    class Statement;
    class Model;
}

class BackupTests : public Nepomuk::TestBase
{
    Q_OBJECT
public:
    BackupTests();
    
private slots:
    void basicRatingRetrivalTest();
    void allPropertiesRetrivalTest();

private:
    typedef org::kde::nepomuk::services::nepomukbackupsync::BackupManager BackupManager;
    BackupManager* m_backupManager;

    Soprano::Model * m_model;
    
    void compare( const QList<Soprano::Statement> & l1, const QList<Soprano::Statement> & l2 );

    void backup();
    void restore();
};

#endif // BACKUPTESTS_H
