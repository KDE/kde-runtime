/*
   Copyright 2008-2010 Sebastian Trueg <trueg@kde.org>
   Copyright 2012 Vishesh Handa <me@vhanda.in>

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

#include "searchfolder.h"
#include "nepomuksearchurltools.h"
#include "queryutils.h"

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Node> // for qHash( QUrl )

#include <Nepomuk2/Variant>
#include <Nepomuk2/Types/Class>
#include <Nepomuk2/Query/Query>
#include <Nepomuk2/Query/Result>
#include <Nepomuk2/Query/ResultIterator>
#include <Nepomuk2/Query/ResourceTypeTerm>
#include <Nepomuk2/Vocabulary/NFO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/PIMO>

#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Resource>
#include <Nepomuk2/File>

#include <QtCore/QMutexLocker>
#include <QTextDocument>
#include <Soprano/QueryResultIterator>
#include <Soprano/Model>

#include <KUrl>
#include <KDebug>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KUser>
#include <KMimeType>
#include <KConfig>
#include <KConfigGroup>

using namespace Nepomuk2::Vocabulary;
using namespace Soprano::Vocabulary;

Nepomuk2::SearchFolder::SearchFolder( const KUrl& url, KIO::SlaveBase* slave )
    : QObject( 0 ),
      m_url( url ),
      m_slave( slave )
{
    // parse URL (this may fail in which case we fall back to pure SPARQL below)
    Query::parseQueryUrl( url, m_query, m_sparqlQuery );

    if ( m_query.isValid() ) {
        m_sparqlQuery = m_query.toSparqlQuery();
    }
}


Nepomuk2::SearchFolder::~SearchFolder()
{
}

void Nepomuk2::SearchFolder::list()
{
    //FIXME: Do the result count as well?
    Query::ResultIterator it( m_sparqlQuery );
    while( it.next() ) {
        Query::Result result = it.result();
        KIO::UDSEntry uds = statResult( result );
        if ( uds.count() ) {
            m_slave->listEntry(uds, false);
        }
    }
}


KIO::UDSEntry Nepomuk2::SearchFolder::statResult( const Query::Result& result )
{
    Resource res( result.resource() );
    const KUrl uri( res.uri() );
    KUrl nieUrl( result[NIE::url()].uri() );

    // the additional bindings that we only have on unix systems
    // Either all are bound or none of them.
    // see also parseQueryUrl (queryutils.h)
    const Soprano::BindingSet additionalVars = result.additionalBindings();

    // Check if we can get a nie:url, otherwise ignore the result, we do not show non file
    // results in the kioslaves
    if ( nieUrl.isEmpty() ) {
        nieUrl = res.property( NIE::url() ).toUrl();
        if( nieUrl.isEmpty() )
            return KIO::UDSEntry();
    }

    // the UDSEntry that will contain the final result to list
    KIO::UDSEntry uds;

#ifdef Q_OS_UNIX
    if( nieUrl.isLocalFile() && additionalVars[QLatin1String("mtime")].isLiteral() ) {
        // set the basic file information which we got from Nepomuk
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, additionalVars[QLatin1String("mtime")].literal().toDateTime().toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_SIZE, additionalVars[QLatin1String("size")].literal().toInt() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, additionalVars[QLatin1String("mode")].literal().toInt() & S_IFMT );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, additionalVars[QLatin1String("mode")].literal().toInt() & 07777 );
        uds.insert( KIO::UDSEntry::UDS_USER, additionalVars[QLatin1String("user")].toString() );
        uds.insert( KIO::UDSEntry::UDS_GROUP, additionalVars[QLatin1String("group")].toString() );

        // since we change the UDS_NAME KFileItem cannot handle mimetype and such anymore
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, additionalVars[QLatin1String("mime")].toString() );
    }
    else
#endif // Q_OS_UNIX
    {
        // not a simple file search
        // FIXME: Isn't this very expensive? Woudln't it make sense to just directly read of QFileInfo?
        KIO::StatJob* job = KIO::stat( nieUrl, KIO::HideProgressInfo );
        // we do not want to wait for the event loop to delete the job
        QScopedPointer<KIO::StatJob> sp( job );
        job->setAutoDelete( false );
        if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
            uds = job->statResult();
        }
    }

    // We might have multiple files with the same filename, but it's okay since they each have
    // a unique url
    uds.insert( KIO::UDSEntry::UDS_NAME, nieUrl.fileName() );

    // There is a trade-off between using UDS_URL or not. The advantage is that we get proper
    // file names in opening applications and non-KDE apps can handle the URLs properly. The downside
    // is that we lose the context information, i.e. query results cannot be browsed in the opening
    // application.
    // We set the UDS_URL since it works better and the context information was rarely used
    uds.insert( KIO::UDSEntry::UDS_URL, nieUrl.url() );

    // set the local path so that KIO can handle the rest
    if( nieUrl.isLocalFile() )
        uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, nieUrl.toLocalFile() );

    // Tell KIO which Nepomuk resource this actually is
    uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, uri.url() );

    // add optional full-text search excerpts
    QString excerpt = result.excerpt();
    if( !excerpt.isEmpty() ) {
        // KFileItemDelegate cannot handle rich text yet. Thus we need to remove the formatting.
        QTextDocument doc;
        doc.setHtml(excerpt);
        excerpt = doc.toPlainText();
        uds.insert( KIO::UDSEntry::UDS_COMMENT, i18n("Search excerpt: %1", excerpt) );
    }

    return uds;
}
