
/*  This file is part of the KDE project

    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

    Advanced web shortcuts:
    Copyright (C) 2001 Andreas Hochsteger <e9625392@student.tuwien.ac.at>


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "kuriikwsfiltereng.h"

#include <unistd.h>

#include <QTextCodec>

#include <kdebug.h>
#include <kconfiggroup.h>
#include <kprotocolinfo.h>

#include "searchprovider.h"

#define PIDDBG kDebug(7023) << "(" << getpid() << ") "
#define PDVAR(n,v) PIDDBG << n << " = '" << v << "'\n"

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * ../tests/kurifiltertest
 */

class KURISearchFilterEnginePrivate
{
public:
  KURISearchFilterEngine instance;
};

K_GLOBAL_STATIC(KURISearchFilterEnginePrivate, kURISearchFilterEngine)

KURISearchFilterEngine::KURISearchFilterEngine()
{
  loadConfig();
}

QString KURISearchFilterEngine::webShortcutQuery( const QString& typedString ) const
{
  QString result;

  if (m_bWebShortcutsEnabled)
  {
    QString search = typedString;
    int pos = search.indexOf(m_cKeywordDelimiter);

    QString key;
    if ( pos > -1 )
      key = search.left(pos);
    else if ( m_cKeywordDelimiter == ' ' && !search.isEmpty() )
      key = search;

    if (!key.isEmpty() && !KProtocolInfo::isKnownProtocol( key ))
    {
      SearchProvider *provider = SearchProvider::findByKey(key);

      if (provider)
      {
        result = formatResult(provider->query(), provider->charset(),
                              QString(), search.mid(pos+1), true);
        delete provider;
      }
    }
  }

  return result;
}


QString KURISearchFilterEngine::autoWebSearchQuery( const QString& typedString ) const
{
  QString result;

  if (m_bWebShortcutsEnabled && !m_defaultSearchEngine.isEmpty())
  {
    // Make sure we ignore supported protocols, e.g. "smb:", "http:"
    int pos = typedString.indexOf(':');

    if (pos == -1 || !KProtocolInfo::isKnownProtocol(typedString.left(pos)))
    {
      SearchProvider *provider = SearchProvider::findByDesktopName(m_defaultSearchEngine);

      if (provider)
      {
        result = formatResult (provider->query(), provider->charset(),
                               QString(), typedString, true);
        delete provider;
      }
    }
  }

  return result;
}

QByteArray KURISearchFilterEngine::name() const
{
  return "kuriikwsfilter";
}

KURISearchFilterEngine* KURISearchFilterEngine::self()
{
  return &kURISearchFilterEngine->instance;
}

QStringList KURISearchFilterEngine::modifySubstitutionMap(SubstMap& map,
                                                          const QString& query) const
{
  // Returns the number of query words
  QString userquery = query;

  // Do some pre-encoding, before we can start the work:
  {
    int start = 0;
    int pos = 0;
    QRegExp qsexpr("\\\"[^\\\"]*\\\"");

    // Temporary substitute spaces in quoted strings (" " -> "%20")
    // Needed to split user query into StringList correctly.
    while ((pos = qsexpr.indexIn(userquery, start)) >= 0)
    {
      QString s = userquery.mid (pos, qsexpr.matchedLength());
      s.replace (' ', "%20");
      start = pos + s.length(); // Move after last quote
      userquery = userquery.replace (pos, qsexpr.matchedLength(), s);
    }
  }

  // Split user query between spaces:
  QStringList l = userquery.simplified().split(' ', QString::SkipEmptyParts);

  // Back-substitute quoted strings (%20 -> " "):
  userquery.replace (QLatin1String("%20"), QLatin1String(" "));
  l.replaceInStrings(QLatin1String("%20"), QLatin1String(" "));

  PIDDBG << "Generating substitution map:\n";
  // Generate substitution map from user query:
  for (int i=0; i<=l.count(); i++)
  {
    int pos = 0;
    QString v;
    QString nr = QString::number(i);

    // Add whole user query (\{0}) to substitution map:
    if (i==0)
      v = userquery;
    // Add partial user query items to substitution map:
    else
      v = l[i-1];

    // Insert partial queries (referenced by \1 ... \n) to map:
    map.insert(QString::number(i), v);
    PDVAR ("  map['" + nr + "']", map[nr]);

    // Insert named references (referenced by \name) to map:
    if ((i>0) && (pos = v.indexOf("=")) > 0)
    {
      QString s = v.mid(pos + 1);
      QString k = v.left(pos);

      // Back-substitute references contained in references (e.g. '\refname' substitutes to 'thisquery=\0')
      s.replace(QLatin1String("%5C"), QLatin1String("\\"));
      map.insert(k, s);
      PDVAR ("  map['" + k + "']", map[k]);
    }
  }

  return l;
}

static QString encodeString(const QString &s, QTextCodec *codec)
{
  QStringList l = s.split(' ');
  for(QStringList::Iterator it = l.begin();
      it != l.end(); ++it)
  {
     *it = codec->fromUnicode( *it ).toPercentEncoding();
  }
  return l.join("+");
}

QString KURISearchFilterEngine::substituteQuery(const QString& url, SubstMap &map, const QString& userquery, QTextCodec *codec) const
{
  QString newurl = url;
  QStringList ql = modifySubstitutionMap (map, userquery);
  int count = ql.count();

  // Check, if old style '\1' is found and replace it with \{@} (compatibility mode):
  {
    int pos = -1;
    if ((pos = newurl.indexOf("\\1")) >= 0)
    {
      PIDDBG << "WARNING: Using compatibility mode for newurl='" << newurl
             << "'. Please replace old style '\\1' with new style '\\{0}' "
                "in the query definition.\n";
      newurl = newurl.replace(pos, 2, "\\{@}");
    }
  }

  PIDDBG << "Substitute references:\n";
  // Substitute references (\{ref1,ref2,...}) with values from user query:
  {
    int pos = 0;
    QRegExp reflist("\\\\\\{[^\\}]+\\}");

    // Substitute reflists (\{ref1,ref2,...}):
    while ((pos = reflist.indexIn(newurl)) >= 0)
    {
      bool found = false;

      //bool rest = false;
      QString v = "";
      QString rlstring = newurl.mid(pos + 2, reflist.matchedLength() - 3);
      PDVAR ("  reference list", rlstring);

      // \{@} gets a special treatment later
      if (rlstring == "@")
      {
        v = "\\@";
        found = true;
      }

      // TODO: strip whitespaces around commas
      QStringList rl = rlstring.split(",", QString::SkipEmptyParts);
      int i = 0;

      while ((i<rl.count()) && !found)
      {
        QString rlitem = rl[i];
        QRegExp range("[0-9]*\\-[0-9]*");

        // Substitute a range of keywords
        if (range.indexIn(rlitem) >= 0)
        {
          int pos = rlitem.indexOf("-");
          int first = rlitem.left(pos).toInt();
          int last  = rlitem.right(rlitem.length()-pos-1).toInt();

          if (first == 0)
            first = 1;

          if (last  == 0)
            last = count;

          for (int i=first; i<=last; i++)
          {
            v += map[QString::number(i)] + ' ';
            // Remove used value from ql (needed for \{@}):
            ql[i-1] = "";
          }

          v = v.trimmed();
          if (!v.isEmpty())
            found = true;

          PDVAR ("    range", QString::number(first) + '-' + QString::number(last) + " => '" + v + '\'');
          v = encodeString(v, codec);
        }
        else if ( rlitem.startsWith('\"') && rlitem.endsWith('\"') )
        {
          // Use default string from query definition:
          found = true;
          QString s = rlitem.mid(1, rlitem.length() - 2);
          v = encodeString(s, codec);
          PDVAR ("    default", s);
        }
        else if (map.contains(rlitem))
        {
          // Use value from substitution map:
          found = true;
          PDVAR ("    map['" + rlitem + "']", map[rlitem]);
          v = encodeString(map[rlitem], codec);

          // Remove used value from ql (needed for \{@}):
          QString c = rlitem.left(1);
          if (c=="0")
          {
            // It's a numeric reference to '0'
            for (QStringList::Iterator it = ql.begin(); it!=ql.end(); ++it)
              (*it) = "";
          }
          else if ((c>="0") && (c<="9"))
          {
            // It's a numeric reference > '0'
            int n = rlitem.toInt();
            ql[n-1] = "";
          }
          else
          {
            // It's a alphanumeric reference
            QStringList::Iterator it = ql.begin();
            while ((it != ql.end()) && !it->startsWith(rlitem + '='))
              ++it;
            if (it != ql.end())
              it->clear();
          }

          // Encode '+', otherwise it would be interpreted as space in the resulting url:
          v.replace('+', "%2B");
        }
        else if (rlitem == "@")
        {
          v = "\\@";
          PDVAR ("    v", v);
        }

        i++;
      }

      newurl.replace(pos, reflist.matchedLength(), v);
    }

    // Special handling for \{@};
    {
      PDVAR ("  newurl", newurl);
      // Generate list of unmatched strings:
      QString v = ql.join(" ").simplified();

      PDVAR ("    rest", v);
      v = encodeString(v, codec);

      // Substitute \{@} with list of unmatched query strings
      newurl.replace("\\@", v);
    }
  }

  return newurl;
}

QString KURISearchFilterEngine::formatResult( const QString& url,
                                              const QString& cset1,
                                              const QString& cset2,
                                              const QString& query,
                                              bool isMalformed ) const
{
  SubstMap map;
  return formatResult (url, cset1, cset2, query, isMalformed, map);
}

QString KURISearchFilterEngine::formatResult( const QString& url,
                                              const QString& cset1,
                                              const QString& cset2,
                                              const QString& query,
                                              bool /* isMalformed */,
                                              SubstMap& map ) const
{
  // Return nothing if userquery is empty and it contains
  // substitution strings...
  if (query.isEmpty() && url.indexOf("\\{") > 0)
    return QString();

  // Debug info of map:
  if (!map.isEmpty())
  {
    PIDDBG << "Got non-empty substitution map:\n";
    for(SubstMap::Iterator it = map.begin(); it != map.end(); ++it)
      PDVAR ("    map['" + it.key() + "']", it.value());
  }

  // Create a codec for the desired encoding so that we can transcode the user's "url".
  QString cseta = cset1;
  if (cseta.isEmpty())
    cseta = "UTF-8";

  QTextCodec *csetacodec = QTextCodec::codecForName(cseta.toLatin1());
  if (!csetacodec)
  {
    cseta = "UTF-8";
    csetacodec = QTextCodec::codecForName(cseta.toLatin1());
  }

  // Decode user query:
  QString userquery = QUrl::fromPercentEncoding( query.toUtf8() );

  PDVAR ("user query", userquery);
  PDVAR ("query definition", url);

  // Add charset indicator for the query to substitution map:
  map.insert("ikw_charset", cseta);

  // Add charset indicator for the fallback query to substitution map:
  QString csetb = cset2;
  if (csetb.isEmpty())
    csetb = "UTF-8";
  map.insert("wsc_charset", csetb);

  QString newurl = substituteQuery (url, map, userquery, csetacodec);

  PDVAR ("substituted query", newurl);

  return newurl;
}

void KURISearchFilterEngine::loadConfig()
{
  PIDDBG << "Keywords Engine: Loading config..." << endl;

  // Load the config.
  KConfig config( name() + "rc", KConfig::NoGlobals );
  KConfigGroup group = config.group( "General" );

  m_cKeywordDelimiter = QString(group.readEntry("KeywordDelimiter", ":")).at(0).toLatin1();
  m_bWebShortcutsEnabled = group.readEntry("EnableWebShortcuts", true);
  m_defaultSearchEngine = group.readEntry("DefaultSearchEngine");
  m_bVerbose = group.readEntry("Verbose", false);

  // Use either a white space or a : as the keyword delimiter...
  if (strchr (" :",m_cKeywordDelimiter) == 0)
    m_cKeywordDelimiter = ':';

  PIDDBG << "Keyword Delimiter: " << m_cKeywordDelimiter << endl;
  PIDDBG << "Default Search Engine: " << m_defaultSearchEngine << endl;
  PIDDBG << "Web Shortcuts Enabled: " << m_bWebShortcutsEnabled << endl;
  PIDDBG << "Verbose: " << m_bVerbose << endl;
}
