/*
   Copyright 2009-2012 Sebastian Trueg <trueg@kde.org>

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

#include "resourcepagegenerator.h"

#include <QtCore/QByteArray>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>

#include <KComponentData>
#include <KDebug>
#include <KUser>
#include <KMessageBox>
#include <kdeversion.h>
#include <KUrl>

#include <nepomuk2/file.h>
#include <nepomuk2/resourcemanager.h>
#include <nepomuk2/variant.h>
#include <nepomuk2/class.h>
#include <nepomuk2/property.h>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/NFO>
#include <nepomuk2/utils.h>

#include <Soprano/Model>
#include <Soprano/Node>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>


using namespace Nepomuk2::Vocabulary;

namespace {
    const char* s_noFollow = "noFollow";
    const char* s_showUri = "showUris";
    const char* s_showNonUserVisibible = "showNonUserVisible";
    const char* s_true = "true";

    KUrl configureUrl( const KUrl& url, Nepomuk2::ResourcePageGenerator::Flags flags ) {
        KUrl newUrl( url );

        newUrl.removeEncodedQueryItem( s_noFollow );
        if ( url.scheme() == QLatin1String( "nepomuk" ) ) {
            newUrl.addEncodedQueryItem( s_noFollow, s_true );
        }

        newUrl.removeEncodedQueryItem( s_showUri );
        if ( flags & Nepomuk2::ResourcePageGenerator::ShowUris ) {
            newUrl.addEncodedQueryItem( s_showUri, s_true );
        }

        newUrl.removeEncodedQueryItem( s_showNonUserVisibible );
        if ( flags & Nepomuk2::ResourcePageGenerator::ShowNonUserVisible ) {
            newUrl.addEncodedQueryItem( s_showNonUserVisibible, s_true );
        }

        return newUrl;
    }
}


Nepomuk2::ResourcePageGenerator::ResourcePageGenerator( const Nepomuk2::Resource& res )
    : m_resource( res )
{
}


Nepomuk2::ResourcePageGenerator::~ResourcePageGenerator()
{
}


void Nepomuk2::ResourcePageGenerator::setFlagsFromUrl( const KUrl& url )
{
    m_flags = NoFlags;
    if ( url.encodedQueryItemValue( s_showUri ) == s_true )
        m_flags |= ShowUris;
    if ( url.encodedQueryItemValue( s_showNonUserVisibible ) == s_true )
        m_flags |= ShowNonUserVisible;
}


KUrl Nepomuk2::ResourcePageGenerator::url() const
{
    return configureUrl( m_resource.uri(), m_flags );
}


// TODO: create an html template rather than having it hardcoded here
QByteArray Nepomuk2::ResourcePageGenerator::generatePage() const
{
    bool exists = m_resource.exists();

    QByteArray output;

    QString label = m_resource.genericLabel();

    QTextStream os( &output, QIODevice::WriteOnly );
    os.setCodec( "UTF-8" );
    os << "<html xmlns=\"http://www.w3.org/1999/xhtml\" version=\"-//W3C//DTD XHTML 1.2//EN\" xml:lang=\"en\">"
       << "<head>"
       << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
       << "<title>" << label << "</title>"
       << "<style>"
        "body { "
        "  margin: 0; "
        "  padding: 0; "
        "  text-align: center; "
        "  font-size: 0.8em; "
        "  font-family: \"Bitstream Vera Sans\", \"Lucida Grande\", \"Trebuchet MS\", sans-serif; "
        "  color: #535353; "
        "  background: #ffffff; } "
        "table { "
        "  font-size: 1.0em; "
        "} "
        "h1 { "
        "  padding: 0; "
        "  text-align: left; "
        "  font-weight: bold; "
        "  color: #f7800a; "
        "  background: transparent; } "
        "h1 { "
        "  margin: 0 0 0.3em 0; "
        "  font-size: 1.7em; } "
        "h2, h3, h4 { "
        "  margin: 1.3em 0 0 0.3em } "
        "h2 { "
        "  font-size: 1.2em; } "
        "h3 { font-size: 1.4em; } "
        "h4 { font-size: 1.3em; } "
        "h5 { font-size: 1.2em; } "
        "a:link { "
        "  padding-bottom: 0; "
        "  text-decoration: none; "
        "  color: #0057ae; } "
        "a:visited { "
        "  padding-bottom: 0; "
        "  text-decoration: none; "
        "  color: #644A9B; } "
        "a[href]:hover { "
        "  text-decoration: underline; }"
        "p { "
        "  margin-top: 0.5em; "
        "  margin-bottom: 0.9em; "
        "  text-align: justify; } "
        "li { "
        "  text-align: left; } "
        ". clearer { "
        "  clear: both; "
        "  height: 1px; } "
        "#content { "
        "  width: 100%; } "
        "#main { "
        "  padding: 10px; "
        "  text-align: left; } "
        "#body_wrapper { "
        "  margin: 0 auto; "
        "  width: 60em; "
        "  min-width: 770px; "
        "  max-width: 45em; "
        "  border: #bcbcbc solid; "
        "  border-width: 0 0 0 1px; } "
        "#body { "
        "  float: left; "
        "  margin: 0; "
        "  padding: 0; "
        "  min-height: 40em; "
        "  width: 60em; "
        "  min-width: 770px; "
        "  max-width: 45em; } "
        "#relations { "
        "  padding: 0 20 0 20; }"
        "</style></head>"
       << "<body>"
       << createConfigureBoxHtml()
       << "<div id=\"body_wrapper\"><div id=\"body\"><div class=\"content\"><div id=\"main\"><div class=\"clearer\">&nbsp;</div>";

    os << "<h1>" << label << "</h1>"
       << "Type: " << ( exists ? typesToHtml( m_resource.types() ) : i18n( "Resource does not exist" ) );

    if(m_resource.isFile() && m_resource.hasType(Nepomuk2::Vocabulary::NFO::Image())) {
        os << "<img src=\"" << m_resource.uri().toString() << "\" />";
    }
    else if(m_resource.hasProperty(Nepomuk2::Vocabulary::NFO::depiction())) {
        os << "<img src=\"" << m_resource.property(Nepomuk2::Vocabulary::NFO::depiction()).toUrlList().first().toString() << "\" />";
    }

    os << "<h2>" << i18n("Relations:") << "</h2><div id=\"relations\"><table>";

    // query relations
    Soprano::StatementIterator it = ResourceManager::instance()->mainModel()->listStatements( m_resource.uri(), Soprano::Node(), Soprano::Node() );
    while ( it.next() ) {
        Soprano::Statement s = it.current();
        Nepomuk2::Types::Property p( s.predicate().uri() );
        if ( p != Soprano::Vocabulary::RDF::type() &&
             ((m_flags & ShowNonUserVisible) || p.userVisible())) {
            os << "<tr><td align=right><i>" << entityLabel( p ) << "</i></td><td width=16px></td><td>";
            if ( s.object().isLiteral() ) {
                os << formatLiteral(p, s.object().literal());
            }
            else {
                //
                // nie:url is a special case for which we should never use Resource
                // since Resource does in turn use nie:url to resolve resource URIs.
                // Thus, we would get back to m_resource.
                //
                KUrl uri = s.object().uri();
                QString label = uri.fileName();
                if ( s.predicate() != Nepomuk2::Vocabulary::NIE::url() ) {
                    Resource resource( uri );
                    uri = resource.uri();
                    label = QString::fromLatin1( "%1 (%2)" )
                            .arg( resourceLabel( resource ),
                                  typesToHtml( resource.types() ) );
                }
                os << QString( "<a href=\"%1\">%2</a>" )
                    .arg( encodeUrl( uri ),
                          label );
            }
            os << "</td></tr>";
        }
    }
    os << "</table></div>";

    // query backlinks
    os << "<h2>" << i18n("Backlinks:") << "</h2><div id=\"relations\"><table>";
    Soprano::StatementIterator itb = ResourceManager::instance()->mainModel()->listStatements( Soprano::Node(), Soprano::Node(), m_resource.uri() );
    while ( itb.next() ) {
        Soprano::Statement s = itb.current();
        Resource resource( s.subject().uri() );
        Nepomuk2::Types::Property p( s.predicate().uri() );
        if((m_flags & ShowNonUserVisible) || p.userVisible()) {
            os << "<td align=right>"
               << QString( "<a href=\"%1\">%2</a> (%3)" )
                  .arg( encodeUrl( s.subject().uri() ),
                        resourceLabel( resource ),
                        typesToHtml( resource.types() ) )
               << "</td>"
               << "<td width=16px></td>"
               << "<td><i>" << entityLabel( p ) << "</i></td></tr>";
        }
    }
    os << "</table></div>";

    if ( exists ) {
        os << "<h2>" << i18n("Actions:") << "</h2>"
           << "<div id=\"relations\"><a href=\"" << KUrl( m_resource.uri() ).url() << "?action=delete\">" << i18n( "Delete resource" ) << "</a></div>";
    }

    os << "</div></div></div></div></body></html>";
    os.flush();

    return output;
}


QString Nepomuk2::ResourcePageGenerator::resourceLabel( const Resource& res ) const
{
    if ( m_flags & ShowUris )
        return KUrl( res.uri() ).prettyUrl();
    else
        return res.genericLabel();
}


QString Nepomuk2::ResourcePageGenerator::entityLabel( const Nepomuk2::Types::Entity& e ) const
{
    if ( m_flags & ShowUris )
        return KUrl( e.uri() ).prettyUrl();
    else
        return e.label();
}


QString Nepomuk2::ResourcePageGenerator::typesToHtml( const QList<QUrl>& types ) const
{
    QList<Nepomuk2::Types::Class> typeClasses;
    foreach( const QUrl& type, types ) {
        typeClasses << Nepomuk2::Types::Class( type );
    }

    // remove all types that are supertypes of others in the list
    QList<Nepomuk2::Types::Class> normalizedTypes;
    for ( int i = 0; i < typeClasses.count(); ++i ) {
        Nepomuk2::Types::Class& type = typeClasses[i];
        bool use = true;
        for ( int j = 0; j < typeClasses.count(); ++j ) {
            if ( type != typeClasses[j] &&
                 typeClasses[j].isSubClassOf( type ) ) {
                use = false;
                break;
            }
        }
        if ( use ) {
            normalizedTypes << type;
        }
    }

    // extract the labels
    QStringList typeStrings;
    for ( int i = 0; i < normalizedTypes.count(); ++i ) {
        QUrl url("nepomuk:");
        url.addQueryItem(QLatin1String("resource"), normalizedTypes[i].uri().toString());
        url.addQueryItem(QLatin1String("noFollow"), QLatin1String("true"));
        typeStrings << QString::fromLatin1("<a href=\"nepomuk:?resource=%1&noFollow=true\">%2</a>").arg(QUrl::toPercentEncoding(normalizedTypes[i].uri().toString(), "", "/:"), entityLabel( normalizedTypes[i] ));
    }

    return typeStrings.join( ", " );
}


QString Nepomuk2::ResourcePageGenerator::encodeUrl( const QUrl& url ) const
{
    return QString::fromAscii( configureUrl( url, m_flags ).toEncoded() );
}


QString Nepomuk2::ResourcePageGenerator::createConfigureBoxHtml() const
{
    QString html
        = QString::fromLatin1( "<div style=\"position:fixed; right:10px; top:10px; text-align:right;\"><a href=\"%1\">%2</a><br/><a href=\"%3\">%4</a></div>" )
        .arg( configureUrl( url(), m_flags^ShowUris ).url(),
              m_flags&ShowUris ? i18n( "Hide URIs" ) : i18n( "Show URIs" ),
              configureUrl( url(), m_flags^ShowNonUserVisible ).url(),
              m_flags&ShowNonUserVisible ? i18n( "Hide non-user visible properties" ) : i18n( "Show non-user visible properties" ));

    return html;
}

QString Nepomuk2::ResourcePageGenerator::formatLiteral(const Nepomuk2::Types::Property &p, const Soprano::LiteralValue &value) const
{
    return Nepomuk2::Utils::formatPropertyValue(p, Nepomuk2::Variant(value.variant()));
}

QString Nepomuk2::ResourcePageGenerator::formatResource(const Nepomuk2::Types::Property &p, const QUrl &uri_) const
{
    //
    // nie:url is a special case for which we should never use Resource
    // since Resource does in turn use nie:url to resolve resource URIs.
    // Thus, we would get back to m_resource.
    //
    KUrl uri(uri_);
    QString label = uri.fileName();
    if ( p != NIE::url() ) {
        Resource resource( uri );
        uri = resource.uri();
        label = QString::fromLatin1( "%1 (%2)" )
                .arg( resourceLabel( resource ),
                      typesToHtml( resource.types() ) );
    }
    return QString( "<a href=\"%1\">%2</a>" )
            .arg( encodeUrl( uri ),
                  label );
}
