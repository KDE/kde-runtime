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
#include "removablemediacache.h"

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
    : Soprano::FilterModel(parentModel)
{
    setParent(parent);
    m_removableMediaCache = new RemovableMediaCache(this);
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

Soprano::Node Nepomuk::RemovableMediaModel::convertFileUrl(const Soprano::Node &node) const
{
    if(node.isResource()) {
        const QUrl url = node.uri();
        if(url.scheme() == QLatin1String("file")) {
            const QString localFilePath = url.toLocalFile();
            if(const RemovableMediaCache::Entry* entry = m_removableMediaCache->findEntryByFilePath(localFilePath)) {
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
        if(m_removableMediaCache->hasRemovableSchema(url)) {
            if(const RemovableMediaCache::Entry* entry = m_removableMediaCache->findEntryByUrl(url)) {
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

#include "removablemediamodel.moc"
