/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef RESOURCEIDENTIFIER_H
#define RESOURCEIDENTIFIER_H

#include "../backupsync/lib/resourceidentifier.h"

#include <KUrl>

namespace Nepomuk {

class ResourceIdentifier : public Sync::ResourceIdentifier
{
public:
    ResourceIdentifier(Soprano::Model *model);

protected:
    virtual KUrl duplicateMatch(const KUrl& uri, const QSet< KUrl >& matchedUris );
    virtual bool runIdentification(const KUrl& uri);

private:
    bool isIdentifyingProperty( const QUrl& uri );

    /// Returns true if a resource with uri \p uri exists
    bool exists( const KUrl& uri );
};

}

#endif // RESOURCEIDENTIFIER_H
