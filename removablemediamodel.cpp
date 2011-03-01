/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "removablemediamodel.h"

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/Predicate>

#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/IteratorBackend>
#include <Soprano/QueryResultIterator>
#include <Soprano/QueryResultIteratorBackend>
#include <Soprano/Vocabulary/NAO>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>

#include <KUrl>
#include <KDebug>

#include <QtCore/QFile>


using namespace Nepomuk::Vocabulary;

// TODO: how do we handle this scenario: the indexer indexes files on a removable medium. This includes
//       a nie:isPartOf relation to the parent folder of the mount point. This is technically not correct
//       as it should be part of the removable file system instead. Right?
// TODO: Optical media do not have a uuid. Here we could use a combination of label and identifier of the medium.
// TODO: we somehow need to handle network mounts here, too.

namespace {
    bool isUsableVolume( const Solid::Device& dev ) {
        if ( dev.is<Solid::StorageVolume>() &&
             dev.is<Solid::StorageAccess>() &&
             dev.parent().is<Solid::StorageDrive>() &&
             ( dev.parent().as<Solid::StorageDrive>()->isRemovable() ||
               dev.parent().as<Solid::StorageDrive>()->isHotpluggable() ) ) {
            const Solid::StorageVolume* volume = dev.as<Solid::StorageVolume>();
            if ( !volume->isIgnored() && volume->usage() == Solid::StorageVolume::FileSystem )
                return true;
        }

        return false;
    }

    bool isUsableVolume( const QString& udi ) {
        Solid::Device dev( udi );
        return isUsableVolume( dev );
    }
}


/**
 * A simple wrapper iterator which converts all filex:/ URLs to local file:/ URLs (if possible).
 */
class Nepomuk::RemovableMediaModel::StatementIteratorBackend : public Soprano::IteratorBackend<Soprano::Statement>
{
public:
    StatementIteratorBackend(const RemovableMediaModel* model,
                             const Soprano::StatementIterator& it)
        : m_it(it),
          m_model(model) {
    }

    bool next() {
        return m_it.next();
    }

    Soprano::Statement current() const {
        return m_model->convertFilexUrls(m_it.current());
    }

    void close() {
        m_it.close();
    }

private:
    Soprano::StatementIterator m_it;
    const RemovableMediaModel* m_model;
};


/**
 * A simple wrapper iterator which converts all filex:/ URLs to local file:/ URLs (if possible).
 */
class Nepomuk::RemovableMediaModel::QueryResultIteratorBackend : public Soprano::QueryResultIteratorBackend
{
public:
    QueryResultIteratorBackend(const Nepomuk::RemovableMediaModel* model, const Soprano::QueryResultIterator& it)
        : m_it(it),
          m_model(model) {
    }

    bool next() {
        return m_it.next();
    }

    void close() {
        m_it.close();
    }

    Soprano::Statement currentStatement() const {
        return m_model->convertFilexUrls(m_it.currentStatement());
    }

    Soprano::Node binding( const QString &name ) const {
        return m_model->convertFilexUrl(m_it.binding(name));
    }

    Soprano::Node binding( int offset ) const {
        return m_model->convertFilexUrl(m_it.binding(offset));
    }

    int bindingCount() const {
        return m_it.bindingCount();
    }

    QStringList bindingNames() const {
        return m_it.bindingNames();
    }

    bool isGraph() const {
        return m_it.isGraph();
    }

    bool isBinding() const {
        return m_it.isBinding();
    }

    bool isBool() const {
        return m_it.isBool();
    }

    bool boolValue() const {
        return m_it.boolValue();
    }

private:
    Soprano::QueryResultIterator m_it;
    const RemovableMediaModel* m_model;
};


Nepomuk::RemovableMediaModel::RemovableMediaModel(Soprano::Model* parentModel, QObject* parent)
    : Soprano::FilterModel(parentModel),
      m_filexSchema(QLatin1String("filex")),
      m_fileSchema(QLatin1String("file"))
{
    setParent(parent);
    initCacheEntries();

    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString& ) ),
             this, SLOT( slotSolidDeviceAdded( const QString& ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString& ) ),
             this, SLOT( slotSolidDeviceRemoved( const QString& ) ) );
}

Nepomuk::RemovableMediaModel::~RemovableMediaModel()
{
}

Soprano::Error::ErrorCode Nepomuk::RemovableMediaModel::addStatement(const Soprano::Statement &statement)
{
        //
        // First we create the filesystem resource. We mostly need this for the user readable label.
        //
//        Nepomuk::Resource fsRes( entry.m_uuid, Nepomuk::Vocabulary::NFO::Filesystem() );
//        fsRes.setLabel( entry.m_description );

        // IDEA: What about nie:hasPart nfo:FileSystem for all top-level items instead of the nfo:Folder that is the mount point.
        // But then we run into the recursion problem
//        Resource fileRes( resource );
//        fileRes.addProperty( Nepomuk::Vocabulary::NIE::isPartOf(), fsRes );

    return FilterModel::addStatement(convertFileUrls(statement));
}

Soprano::Error::ErrorCode Nepomuk::RemovableMediaModel::removeStatement(const Soprano::Statement &statement)
{
    return FilterModel::removeStatement(convertFileUrls(statement));
}

Soprano::Error::ErrorCode Nepomuk::RemovableMediaModel::removeAllStatements(const Soprano::Statement &statement)
{
    return FilterModel::removeAllStatements(convertFileUrls(statement));
}

bool Nepomuk::RemovableMediaModel::containsStatement(const Soprano::Statement &statement) const
{
    return FilterModel::containsStatement(convertFileUrls(statement));
}

bool Nepomuk::RemovableMediaModel::containsAnyStatement(const Soprano::Statement &statement) const
{
    return FilterModel::containsAnyStatement(convertFileUrls(statement));
}

Soprano::StatementIterator Nepomuk::RemovableMediaModel::listStatements(const Soprano::Statement &partial) const
{
    return new StatementIteratorBackend(this, FilterModel::listStatements(partial));
}

Soprano::QueryResultIterator Nepomuk::RemovableMediaModel::executeQuery(const QString &query, Soprano::Query::QueryLanguage language, const QString &userQueryLanguage) const
{
    return new QueryResultIteratorBackend(this, FilterModel::executeQuery(convertFileUrls(query), language, userQueryLanguage));
}

void Nepomuk::RemovableMediaModel::initCacheEntries()
{
    QList<Solid::Device> devices
            = Solid::Device::listFromQuery( QLatin1String( "StorageVolume.usage=='FileSystem'" ) );
    foreach( const Solid::Device& dev, devices ) {
        if ( isUsableVolume( dev ) ) {
            if(Entry* entry = createCacheEntry( dev )) {
                const Solid::StorageAccess* storage = entry->m_device.as<Solid::StorageAccess>();
                if ( storage && storage->isAccessible() )
                    slotAccessibilityChanged( true, dev.udi() );
            }
        }
    }
}

Nepomuk::RemovableMediaModel::Entry* Nepomuk::RemovableMediaModel::createCacheEntry( const Solid::Device& dev )
{
    QMutexLocker lock(&m_entryCacheMutex);

    Entry entry;
    entry.m_device = dev;
    entry.m_description = dev.description();
    entry.m_uuid = entry.m_device.as<Solid::StorageVolume>()->uuid().toLower();
    if(!entry.m_uuid.isEmpty()) {
        kDebug() << "Found removable storage volume for Nepomuk docking:" << dev.udi() << dev.description();
        connect( dev.as<Solid::StorageAccess>(), SIGNAL(accessibilityChanged(bool, QString)),
                this, SLOT(slotAccessibilityChanged(bool, QString)) );
                m_metadataCache.insert( dev.udi(), entry );
        return &m_metadataCache[dev.udi()];
    }
    else {
        kDebug() << "Cannot use device de to empty UUID:" << dev.udi();
        return 0;
    }
}


Nepomuk::RemovableMediaModel::Entry* Nepomuk::RemovableMediaModel::findEntryByFilePath( const QString& path )
{
    QMutexLocker lock(&m_entryCacheMutex);

    for( QHash<QString, Entry>::iterator it = m_metadataCache.begin();
         it != m_metadataCache.end(); ++it ) {
        Entry& entry = *it;
        if ( entry.m_device.as<Solid::StorageAccess>()->isAccessible() &&
             path.startsWith( entry.m_device.as<Solid::StorageAccess>()->filePath() ) )
            return &entry;
    }

    return 0;
}


const Nepomuk::RemovableMediaModel::Entry* Nepomuk::RemovableMediaModel::findEntryByFilePath( const QString& path ) const
{
    return const_cast<RemovableMediaModel*>(this)->findEntryByFilePath(path);
}


const Nepomuk::RemovableMediaModel::Entry* Nepomuk::RemovableMediaModel::findEntryByUuid(const QString &uuid) const
{
    QMutexLocker lock(&m_entryCacheMutex);

    for( QHash<QString, Entry>::const_iterator it = m_metadataCache.constBegin();
        it != m_metadataCache.constEnd(); ++it ) {
        const Entry& entry = *it;
        if(entry.m_uuid == uuid) {
            return &entry;
        }
    }

    return 0;
}


void Nepomuk::RemovableMediaModel::slotSolidDeviceAdded( const QString& udi )
{
    kDebug() << udi;

    if ( isUsableVolume( udi ) ) {
        createCacheEntry( Solid::Device( udi ) );
    }
}


void Nepomuk::RemovableMediaModel::slotSolidDeviceRemoved( const QString& udi )
{
    kDebug() << udi;
    if ( m_metadataCache.contains( udi ) ) {
        kDebug() << "Found removable storage volume for Nepomuk undocking:" << udi;
        m_metadataCache.remove( udi );
    }
}


void Nepomuk::RemovableMediaModel::slotAccessibilityChanged( bool accessible, const QString& udi )
{
    kDebug() << accessible << udi;

    //
    // cache new mount path
    //
    if ( accessible ) {
        QMutexLocker lock(&m_entryCacheMutex);
        Entry& entry = m_metadataCache[udi];
        entry.m_lastMountPath = entry.m_device.as<Solid::StorageAccess>()->filePath();
        kDebug() << udi << "accessible at" << entry.m_lastMountPath << "with uuid" << entry.m_uuid;
    }
}


Soprano::Node Nepomuk::RemovableMediaModel::convertFileUrl(const Soprano::Node &node) const
{
    if(node.isResource()) {
        const QUrl url = node.uri();
        if(url.scheme() == m_fileSchema) {
            const QString localFilePath = url.toLocalFile();
            if(const Entry* entry = findEntryByFilePath(localFilePath)) {
                return entry->constructRelativeUrl(localFilePath);
            }
        }
    }

    return node;
}


Soprano::Statement Nepomuk::RemovableMediaModel::convertFileUrls(const Soprano::Statement &statement) const
{
    if(statement.predicate().uri() == NIE::url() ||
            (statement.predicate().isEmpty() && statement.object().isResource())) {
        Soprano::Statement newStatement(statement);
        newStatement.setObject(convertFileUrl(statement.object()));
        return newStatement;
    }

    return statement;
}


Soprano::Statement Nepomuk::RemovableMediaModel::convertFilexUrls(const Soprano::Statement &s) const
{
    if(s.predicate().uri() == NIE::url()) {
        Soprano::Statement newStatement(s);
        newStatement.setObject(convertFilexUrl(s.object()));
        return newStatement;
    }
    else {
        return s;
    }
}

Soprano::Node Nepomuk::RemovableMediaModel::convertFilexUrl(const Soprano::Node &node) const
{
    if(node.isResource()) {
        const QUrl url = node.uri();
        if(url.scheme() == m_filexSchema) {
            const QString uuid = url.host();
            if(const Entry* entry = findEntryByUuid(uuid.toLower())) {
                return QUrl::fromLocalFile(entry->constructLocalPath(url));
            }
        }
    }
    // fallback
    return node;
}

QString Nepomuk::RemovableMediaModel::convertFileUrls(const QString &query) const
{
    //
    // There are at least two cases to handle:
    // 1. Simple file:/ URLs used as resources (Example: "<file:///home/foobar>")
    // 2. REGEX filters which contain partial file:/ URLs (Example: "FILTER(REGEX(STR(?u),'^file:///home'))")
    //
    // In theory there are more candidates like matching part of a URL but these have no relevance in Nepomuk.
    //
    // We cannot simply match via a regular expression since in theory file URLs could appear in literals
    // as well. We do not want to change literals.
    //
    // Literals are either enclosed in a set of single or double quotes or in two sets of 3 of the same.
    // Examples: 'Hello', "Hello", '''Hello''', """Hello"""
    //

    // is 0, 1, or 3 - nothing else
    int quoteCnt = 0;
    bool inRegEx = false;
    bool inRes = false;
    QChar quote;
    QString newQuery;
    for(int i = 0; i < query.length(); ++i) {
        const QChar c = query[i];

        // first we update the quoteCnt
        if(c == '\'' || c == '"') {
           if(quoteCnt == 0) {
               // opening a literal
               quote = c;
               ++quoteCnt;
               newQuery.append(c);
               // peek forward to see if there are three
               if(i+2 < query.length() &&
                       query[i+1] == c &&
                       query[i+2] == c) {
                   quoteCnt = 3;
                   i += 2;
                   newQuery.append(c);
                   newQuery.append(c);
               }
               continue;
           }
           else if(c == quote) {
               // possibly closing a literal
               if(quoteCnt == 1) {
                   quoteCnt = 0;
                   newQuery.append(c);
                   continue;
               }
               else { // quoteCnt == 3
                   // peek forward to see if we have three closing ones
                   if(i+2 < query.length() &&
                           query[i+1] == c &&
                           query[i+2] == c) {
                       quoteCnt = 0;
                       i += 2;
                       newQuery.append(c);
                       newQuery.append(c);
                       newQuery.append(c);
                       continue;
                   }
               }
           }
        }

        //
        // If we are not in a quote we can look for a resource
        //
        if(!quoteCnt && c == '<') {
            // peek forward to see if its a file:/ URL
            if(i+6 < query.length() &&
                    query[i+1] == 'f' &&
                    query[i+2] == 'i' &&
                    query[i+3] == 'l' &&
                    query[i+4] == 'e' &&
                    query[i+5] == ':' &&
                    query[i+6] == '/') {
                // look for the end of the file URL
                int pos = query.indexOf('>', i+6);
                if(pos > i+6) {
                    kDebug() << "Found resource at" << i;
                    // convert the file URL into a filex URL (if necessary)
                    const KUrl fileUrl = query.mid(i+1, pos-i-1);
                    newQuery += convertFileUrl(fileUrl).toN3();
                    i = pos;
                    continue;
                }
            }
            inRes = true;
        }

        else if(inRes && c == '>') {
            inRes = false;
        }

        //
        // Or we check for a regex filter
        //
        else if(!inRes && !inRegEx && !quoteCnt && c.toLower() == 'r') {
            // peek forward to see if we have a REGEX filter
            if(i+4 < query.length() &&
                    query[i+1].toLower() == 'e' &&
                    query[i+2].toLower() == 'g' &&
                    query[i+3].toLower() == 'e' &&
                    query[i+4].toLower() == 'x') {
                kDebug() << "Found regex filter at" << i;
                inRegEx = true;
            }
        }

        //
        // Find the end of a regex.
        // FIXME: this is a bit tricky. There might be additional brackets in a regex. not sure.
        //
        else if(inRegEx && !quoteCnt && c == ')') {
            inRegEx = false;
        }

        //
        // Check for a file URL in a regex. This means we need to be in quotes
        // and in a regex
        // This is not perfect as in theory we could have a regex which checks
        // some random literal which happens to mention a file URL. However,
        // that case seems unlikely enough for us to go this way.
        // FIXME: it would be best to check if the filter is done on nie:url.
        //
        else if(inRegEx && quoteCnt && c == 'f') {
            // peek forward to see if its a file URL
            if(i+5 < query.length() &&
                    query[i+1] == 'i' &&
                    query[i+2] == 'l' &&
                    query[i+3] == 'e' &&
                    query[i+4] == ':' &&
                    query[i+5] == '/') {
                // find end of regex
                QString quoteEnd = quote;
                if(quoteCnt == 3) {
                    quoteEnd += quote;
                    quoteEnd += quote;
                }
                int pos = query.indexOf(quoteEnd, i+6);
                if(pos > 0) {
                    kDebug() << "Found file:/ regex filter at" << i;
                    // convert the file URL into a filex URL (if necessary)
                    const KUrl fileUrl = query.mid(i, pos-i);
                    newQuery += KUrl(convertFileUrl(fileUrl).uri()).url();
                    // set i to last char we handled and let the loop continue with the end of the quote
                    i = pos-1;
                    continue;
                }
            }
        }

        newQuery += c;
    }

    return newQuery;
}


KUrl Nepomuk::RemovableMediaModel::Entry::constructRelativeUrl( const QString& path ) const
{
    const QString relativePath = path.mid( m_lastMountPath.count() );
    return KUrl( QLatin1String("filex://") + m_uuid + relativePath );
}


QString Nepomuk::RemovableMediaModel::Entry::constructLocalPath( const KUrl& filexUrl ) const
{
    QString path( m_lastMountPath );
    if ( path.endsWith( QLatin1String( "/" ) ) )
        path.truncate( path.length()-1 );
    path += filexUrl.path();
    return path;
}

#include "removablemediamodel.moc"
