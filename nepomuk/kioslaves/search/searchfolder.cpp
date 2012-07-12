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
#include "resourcestat.h"
#include "queryutils.h"

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Node> // for qHash( QUrl )

#include <Nepomuk2/Variant>
#include <Nepomuk2/Thing>
#include <Nepomuk2/Types/Class>
#include <Nepomuk2/Query/Query>
#include <Nepomuk2/Query/Result>
#include <Nepomuk2/Query/ResourceTypeTerm>
#include <Nepomuk2/Vocabulary/NFO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/PIMO>

#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Resource>

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
    Soprano::Model* model = ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it = model->executeQuery( m_sparqlQuery, Soprano::Query::QueryLanguageSparql );
    while( it.next() ) {
        Query::Result result = extractResult( it );
        KIO::UDSEntry uds = statResult( result );
        if ( uds.count() ) {
            m_slave->listEntry(uds, false);
        }
    }
}

namespace {
    bool statFile( const KUrl& url, const KUrl& fileUrl, KIO::UDSEntry& uds )
    {
        if ( !fileUrl.isEmpty() ) {
            if ( KIO::StatJob* job = KIO::stat( fileUrl, KIO::HideProgressInfo ) ) {
                // we do not want to wait for the event loop to delete the job
                QScopedPointer<KIO::StatJob> sp( job );
                job->setAutoDelete( false );
                if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
                    uds = job->statResult();
                    return true;
                }
            }
        }

        Nepomuk2::Resource res( url );
        if ( res.exists() ) {
            uds = Nepomuk2::statNepomukResource( res );
            return true;
        }

        kDebug() << "failed to stat" << url;
        return false;
    }
}

// This method tries to avoid loading the Nepomuk2::Resource as long as possible by only using the
// request property nie:url in the Result for local files.
KIO::UDSEntry Nepomuk2::SearchFolder::statResult( const Query::Result& result )
{
    Resource res( result.resource() );
    const KUrl uri( res.uri() );
    KUrl nieUrl( result[NIE::url()].uri() );

    // the additional bindings that we only have on unix systems
    // Either all are bound or none of them.
    // see also parseQueryUrl (queryutils.h)
    const Soprano::BindingSet additionalVars = result.additionalBindings();

    // the UDSEntry that will contain the final result to list
    KIO::UDSEntry uds;

#ifdef Q_OS_UNIX
    if( !nieUrl.isEmpty() &&
            nieUrl.isLocalFile() &&
            additionalVars[QLatin1String("mtime")].isLiteral() ) {
        // make sure we have unique names for everything
        uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( nieUrl ) );

        // set the name the user will see
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, nieUrl.fileName() );

        // set the basic file information which we got from Nepomuk
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, additionalVars[QLatin1String("mtime")].literal().toDateTime().toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_SIZE, additionalVars[QLatin1String("size")].literal().toInt() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, additionalVars[QLatin1String("mode")].literal().toInt() & S_IFMT );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, additionalVars[QLatin1String("mode")].literal().toInt() & 07777 );
        uds.insert( KIO::UDSEntry::UDS_USER, additionalVars[QLatin1String("user")].toString() );
        uds.insert( KIO::UDSEntry::UDS_GROUP, additionalVars[QLatin1String("group")].toString() );

        // since we change the UDS_NAME KFileItem cannot handle mimetype and such anymore
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, additionalVars[QLatin1String("mime")].toString() );
        if( uds.stringValue(KIO::UDSEntry::UDS_MIME_TYPE).isEmpty())
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, KMimeType::findByUrl(nieUrl)->name() );
    }
    else
#endif // Q_OS_UNIX
    {
        // not a simple local file result

        // check if we have a pimo thing relating to a file
        if ( nieUrl.isEmpty() )
            nieUrl = Nepomuk2::nepomukToFileUrl( uri );

        // try to stat the file
        if ( statFile( uri, nieUrl, uds ) ) {
            // make sure we have unique names for everything
            // We encode the resource URL or URI into the name so subsequent calls to stat or
            // other non-listing commands can easily forward to the appropriate slave.
            if ( !nieUrl.isEmpty() ) {
                uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( nieUrl ) );
            }
            else {
                uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( uri ) );
            }

            // make sure we do not use these ugly names for display
            if ( !uds.contains( KIO::UDSEntry::UDS_DISPLAY_NAME ) ) {
                if ( !nieUrl.isEmpty() ) {
                    uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, nieUrl.fileName() );

                    // since we change the UDS_NAME KFileItem cannot handle mimetype and such anymore
                    QString mimetype = uds.stringValue( KIO::UDSEntry::UDS_MIME_TYPE );
                    if ( mimetype.isEmpty() ) {
                        mimetype = KMimeType::findByUrl(nieUrl)->name();
                        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, mimetype );
                    }
                }
                else {
                    uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, res.genericLabel() );
                }
            }
        }
        else {
            kDebug() << "Stating" << result.resource().uri() << "failed";
            return KIO::UDSEntry();
        }
    }

    if( !nieUrl.isEmpty() ) {
        // There is a trade-off between using UDS_URL or not. The advantage is that we get proper
        // file names in opening applications and non-KDE apps can handle the URLs properly. The downside
        // is that we lose the context information, i.e. query results cannot be browsed in the opening
        // application. We decide pro-filenames and pro-non-kde-apps here.
        if( !uds.isDir() ) {
            uds.insert( KIO::UDSEntry::UDS_TARGET_URL, nieUrl.url() );
        }

        // set the local path so that KIO can handle the rest
        if( nieUrl.isLocalFile() )
            uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, nieUrl.toLocalFile() );
    }
    else {
        uds.insert( KIO::UDSEntry::UDS_TARGET_URL, uri.url() );
    }

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

// copied from the QueryService
Nepomuk2::Query::Result Nepomuk2::SearchFolder::extractResult(const Soprano::QueryResultIterator& it) const
{
    Query::Result result( Resource::fromResourceUri( it[0].uri() ) );
    const Query::RequestPropertyMap map = m_query.requestPropertyMap();
    for( Query::RequestPropertyMap::const_iterator rit = map.begin(); rit != map.constEnd(); rit++ ) {
        result.addRequestProperty( rit.value(), it.binding( rit.key() ) );
    }

    // make sure we do not store values twice
    QStringList names = it.bindingNames();
    names.removeAll( QLatin1String( "r" ) );

    static const char* s_scoreVarName = "_n_f_t_m_s_";
    static const char* s_excerptVarName = "_n_f_t_m_ex_";

    Soprano::BindingSet set;
    int score = 0;
    Q_FOREACH( const QString& var, names ) {
        if ( var == QLatin1String( s_scoreVarName ) )
            score = it[var].literal().toInt();
        else if ( var == QLatin1String( s_excerptVarName ) )
            result.setExcerpt( it[var].toString() );
        else
            set.insert( var, it[var] );
    }

    result.setAdditionalBindings( set );
    result.setScore( ( double )score );

    return result;
}