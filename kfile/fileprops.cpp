/* This file is part of the KDE libraries
   Copyright (C) 2002,2003 Carsten Pfeiffer <pfeiffer@kde.org>

   library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation, version 2.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "fileprops.h"

#include <iostream>

#include <QtCore/QFile>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kfilemetainfo.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <kdebug.h>

#define KFILEVERSION "0.2"
#define INDENT "\t"

using namespace std;

static QString beatifyValue( const QString& value )
{
    if ( value.isNull() )
        return QString("(no value for key available)");
    else if ( value.isEmpty() )
        return QString("(empty)");

    return value;
}

FileProps::FileProps( const QString& path )
    : m_dirty( false )
{
    m_info = new KFileMetaInfo(path, QString(), KFileMetaInfo::Everything);
}

FileProps::~FileProps()
{
    sync();
    delete m_info;
}

bool FileProps::sync()
{
    if ( !m_dirty )
        return true;

    return m_info->applyChanges();
}

bool FileProps::isValid() const
{
    return m_info->isValid();
}

QStringList FileProps::supportedKeys() const
{
    return QStringList();
}

QStringList FileProps::availableKeys() const
{
    return m_info->keys();
}

QString FileProps::getValue( const QString& key ) const
{
    return FileProps::createKeyValue( m_info->item(key) );
}

bool FileProps::setValue( const QString& key, const QString &value )
{
    bool ok = m_info->item(key).setValue( value );
    m_dirty |= ok;
    return ok;
}

QStringList FileProps::allValues() const
{
    return FileProps::createKeyValueList( m_info->items().values() );
}

// static helper:
// creates strings like
// "group:       translatedKey:               value"
QString FileProps::createKeyValue( const KFileMetaInfoItem& item )
{
    static const int MAX_SPACE = 25;

    QString result("%1");
    result = result.arg(item.name() + ':', -MAX_SPACE );
    result.append( beatifyValue( item.value().toString() ) );

    return result;
}

// static
QStringList FileProps::createKeyValueList( const KFileMetaInfoItemList& items )
{
    QStringList result;
    KFileMetaInfoItemList::ConstIterator it = items.begin();

    for ( ; it != items.end(); ++it )
        result.append( FileProps::createKeyValue( *it ) );

    return result;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////



// kfile --mimetype --listsupported --listavailable --listwritable --getValue "key" --setValue "key=value" --allValues --dialog --quiet file [file...]
// "key" may be a list of keys, separated by commas

//
// helper functions
//

// Caller needs to delete the items in the list after use!
static KFileItemList fileItemList( const KCmdLineArgs *args )
{
    KFileItemList items;
    for ( int i = 0; i < args->count(); i++ )
        items.append( KFileItem( KFileItem::Unknown,
                                 KFileItem::Unknown,
                                 args->url( i ) ));
    return items;
}

static void showPropertiesDialog( const KCmdLineArgs *args ) {
    const KFileItemList items = fileItemList( args );
    KPropertiesDialog::showDialog( items, 0, true );
}
/*
static void printMimeTypes( const KCmdLineArgs *args )
{
    for ( int i = 0; i < args->count(); i++ )
    {
        KUrl url = args->url( i );
        KMimeType::Ptr mt = KMimeType::findByUrl( url );
        kDebug() << args->arg(i) << ": " << mt->comment().toLocal8Bit() << " ("
             << mt->name().toLocal8Bit() << ")" << endl;
    }
}*/

static void printList( const QStringList& list )
{
    QStringList::ConstIterator it = list.begin();
    for ( ; it != list.end(); ++it )
        kDebug() << (*it).toLocal8Bit();
    kDebug();
}

static void processMetaDataOptions( const QList<FileProps *> propList,
                                    KCmdLineArgs *args )
{
// kfile --mimetype --listsupported --listavailable --listwritable --getValue "key" --setValue "key=value" --allValues --dialog --quiet file [file...]
// "key" may be a list of keys, separated by commas

    QString line("-- -------------------------------------------------------");
    foreach ( FileProps *props, propList )
    {
        QString file = props->fileName() + ' ';
        QString fileString = line;
        fileString.replace( 3, file.length(), file );
        kDebug() << QFile::encodeName( fileString );

        if ( args->isSet( "listsupported" ) )
        {
            kDebug() << "=Supported Keys=";
            printList( props->supportedKeys() );
        }
        if ( args->isSet( "listavailable" ) )
        {
            kDebug() << "=Available Keys=";
            printList( props->availableKeys() );
        }
//         if ( args->isSet( "listwritable" ) )
//         {
//             kDebug() << "TODO :)";
//         }
        if ( args->isSet( "getValue" ) )
        {
            kDebug() << "=Value=";
            QString key = args->getOption("getValue");
            kDebug() << props->getValue( key ).toLocal8Bit();
        }

        if ( args->isSet( "setValue" ) )
        {
            // separate key and value from the line "key=value"
            QString cmd = args->getOption("setValue");
            QString key = cmd.section( '=', 0, 0 );
            QString value = cmd.section( '=', 1 );

            props->setValue(key, value);
        }

        if ( args->isSet( "allValues" ) )
        {
            kDebug() << "=All Values=";
            printList( props->allValues() );
        }
    }

}

int main( int argc, char **argv )
{
    KAboutData about(
	  "kfile", 0, ki18n( "kfile" ), KFILEVERSION,
	  ki18n("A command-line tool to read and modify metadata of files."),
	  KAboutData::License_LGPL, ki18n("(c) 2002, Carsten Pfeiffer"),
	  ki18n(0 /*text*/), "http://devel-home.kde.org/~pfeiffer/",
	  "pfeiffer@kde.org" );

    about.addAuthor( ki18n("Carsten Pfeiffer"), KLocalizedString(), "pfeiffer@kde.org",
		     "http://devel-home.kde.org/~pfeiffer/" );

    KCmdLineArgs::init( argc, argv, &about );


    KCmdLineOptions options;

    options.add("m"); // short option for --mimetype
    options.add("nomimetype", ki18n("Do not print the mimetype of the given file(s)"));
    options.add("ls"); // short option for --listsupported
    options.add("listsupported", ki18n("List all supported metadata keys." ));
    options.add("la"); // short option for --listavailable
    options.add("listavailable", ki18n("List all metadata keys which have a value in the given "
                "file(s)."));
    options.add("q"); // short option for --quiet
    options.add("quiet", ki18n("Do not print a warning when more than one file was given "
                "and they do not all have the same mimetype."));
    options.add("av"); // short option for --allValues
    options.add("allValues", ki18n("Prints all metadata values, available in the given "
                "file(s)."));
    options.add("dialog", ki18n("Opens a KDE properties dialog to allow viewing and "
                "modifying of metadata of the given file(s)"));
    options.add("getValue <key>", ki18n("Prints the value for 'key' of the given file(s). 'key' "
                "may also be a comma-separated list of keys"));
    options.add("setValue <key=value>", ki18n("Attempts to set the value 'value' for the metadata key "
                "'key' for the given file(s)"));
    options.add("+[files]", ki18n("The file (or a number of files) to operate on."));
    KCmdLineArgs::addCmdLineOptions( options );

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    bool useGUI = args->isSet( "dialog" );

    KApplication app( useGUI );

    QList<FileProps *> m_props;

    bool quiet = args->isSet( "quiet" );

    int files = args->count();
    if ( files == 0 ) {
        KCmdLineArgs::usageError( i18n("No files specified") ); // exit()s
    }

    if ( args->isSet( "dialog" ) ) {
        showPropertiesDialog( args );
        return 0;
    }

    QString mimeType;

    for ( int i = 0; i < files; i++ ) {
        //if ( args->isSet( "mimetype" ) )
            //printMimeTypes( args );

        FileProps *props = new FileProps( args->url(i).path());
        if ( props->isValid() ) {
            m_props.append( props );
        } else {
            if ( !quiet ) {
                kWarning() << args->arg(i) << ": " <<
                i18n("Cannot determine metadata").toLocal8Bit() << endl;
            }
            delete props;
        }
    }


    processMetaDataOptions( m_props, args );

    qDeleteAll(m_props); // force destruction/sync of props

    return 0;
}
