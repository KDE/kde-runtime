/****************************************************************************
**
**
** KRDB - puts current KDE color scheme into preprocessor statements
** cats specially written application default files and uses xrdb -merge to
** write to RESOURCE_MANAGER. Thus it gives a  simple way to make non-KDE
** applications fit in with the desktop
**
** Copyright (C) 1998 by Mark Donohoe
** Copyright (C) 1999 by Dirk A. Mueller (reworked for KDE 2.0)
** Copyright (C) 2001 by Matthias Ettrich (add support for GTK applications )
** Copyright (C) 2001 by Waldo Bastian <bastian@kde.org>
** Copyright (C) 2002 by Karol Szwed <gallium@kde.org>
** This application is freely distributable under the GNU Public License.
**
*****************************************************************************/

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#undef Unsorted
#include <qbuffer.h>
#include <qdir.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qsettings.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <ktempfile.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstyle.h>

#include "krdb.h"

#include <X11/Xlib.h>


// -----------------------------------------------------------------------------
static void applyGtkStyles(bool active)
{
   QString gtkkde = QDir::homeDirPath()+"/.gtkrc-kde";
   QCString gtkrc = getenv("GTK_RC_FILES");
   QStringList list = QStringList::split(':', QFile::decodeName(gtkrc));
   if (list.count() == 0)
   {
      list.append(QString::fromLatin1("/etc/gtk/gtkrc"));
      list.append(QDir::homeDirPath()+"/.gtkrc");
   }
   list.remove(gtkkde);
   list.append(gtkkde);
   if (!active)
      ::unlink(QFile::encodeName(gtkkde));

   // Pass env. var to kdeinit.
   QCString name = "GTK_RC_FILES";
   QCString value = QFile::encodeName(list.join(":"));
   QByteArray params;
   QDataStream stream(params, IO_WriteOnly);
   stream << name << value;
   kapp->dcopClient()->send("klauncher", "klauncher", "setLaunchEnv(QCString,QCString)", params);
}

// -----------------------------------------------------------------------------

static void applyQtColors( KSimpleConfig& kglobals, QSettings& settings, QPalette& newPal )
{
  QStringList actcg, inactcg, discg;

  /* export kde color settings */
  int i;
  for (i = 0; i < QColorGroup::NColorRoles; i++)
     actcg   << newPal.color(QPalette::Active,
                (QColorGroup::ColorRole) i).name();
  for (i = 0; i < QColorGroup::NColorRoles; i++)
     inactcg << newPal.color(QPalette::Inactive,
                (QColorGroup::ColorRole) i).name();
  for (i = 0; i < QColorGroup::NColorRoles; i++)
     discg   << newPal.color(QPalette::Disabled,
                (QColorGroup::ColorRole) i).name();

  while (!settings.writeEntry("/qt/Palette/active", actcg)) ;
  settings.writeEntry("/qt/Palette/inactive", inactcg);
  settings.writeEntry("/qt/Palette/disabled", discg);

  // export kwin's colors to qtrc for kstyle to use
  kglobals.setGroup("WM");

  // active colors
  QColor clr = newPal.active().background();
  clr = kglobals.readColorEntry("activeBackground", &clr);
  settings.writeEntry("/qt/KWinPalette/activeBackground", clr.name());
  if (QPixmap::defaultDepth() > 8)
    clr = clr.dark(110);
  clr = kglobals.readColorEntry("activeBlend", &clr);
  settings.writeEntry("/qt/KWinPalette/activeBlend", clr.name());
  clr = newPal.active().highlightedText();
  clr = kglobals.readColorEntry("activeForeground", &clr);
  settings.writeEntry("/qt/KWinPalette/activeForeground", clr.name());
  clr = newPal.active().background();
  clr = kglobals.readColorEntry("frame", &clr);
  settings.writeEntry("/qt/KWinPalette/frame", clr.name());
  clr = kglobals.readColorEntry("activeTitleBtnBg", &clr);
  settings.writeEntry("/qt/KWinPalette/activeTitleBtnBg", clr.name());

  // inactive colors
  clr = newPal.inactive().background();
  clr = kglobals.readColorEntry("inactiveBackground", &clr);
  settings.writeEntry("/qt/KWinPalette/inactiveBackground", clr.name());
  if (QPixmap::defaultDepth() > 8)
    clr = clr.dark(110);
  clr = kglobals.readColorEntry("inactiveBlend", &clr);
  settings.writeEntry("/qt/KWinPalette/inactiveBlend", clr.name());
  clr = newPal.inactive().background().dark();
  clr = kglobals.readColorEntry("inactiveForeground", &clr);
  settings.writeEntry("/qt/KWinPalette/inactiveForeground", clr.name());
  clr = newPal.inactive().background();
  clr = kglobals.readColorEntry("inactiveFrame", &clr);
  settings.writeEntry("/qt/KWinPalette/inactiveFrame", clr.name());
  clr = kglobals.readColorEntry("inactiveTitleBtnBg", &clr);
  settings.writeEntry("/qt/KWinPalette/inactiveTitleBtnBg", clr.name());

  kglobals.setGroup("KDE");
  settings.writeEntry("/qt/KDE/contrast", kglobals.readNumEntry("contrast", 7));
}

// -----------------------------------------------------------------------------

static void applyQtSettings( KSimpleConfig& kglobals, QSettings& settings )
{
  /* export kde's plugin library path to qtrc */

  QMap <QString, bool> pathDb;
    // OK, this isn't fun at all.
    // KApp adds paths ending with /, QApp those without slash, and if
    // one gives it something that is other way around, it will complain and scare
    // users. So we need to know whether a path being added is from KApp, and in this case
    // end it with.. So keep a QMap to bool, specifying whether the path is KDE-specified..

  QStringList kdeAdded =
    settings.readListEntry("/qt/KDE/kdeAddedLibraryPaths");
  QString libPathKey =
    QString("/qt/%1.%2/libraryPath").arg( QT_VERSION >> 16 ).arg( (QT_VERSION & 0xff00 ) >> 8 );

  //Read qt library path..
  QStringList plugins = settings.readListEntry(libPathKey, ':');
  for (QStringList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
  {
    QString path = *it;
    if (path.endsWith("/"))
      path.truncate(path.length()-1);

    pathDb[path]=false;
  }

  //Get rid of old KDE-added ones...
  for (QStringList::ConstIterator it = kdeAdded.begin(); it != kdeAdded.end(); ++it)
  {
    //Normalize..
    QString path = *it;
    if (path.endsWith("/"))
      path.truncate(path.length()-1);

    //Remove..
    pathDb.remove(path);
  }

  kdeAdded.clear();

  //Merge in KDE ones..
  plugins = KGlobal::dirs()->resourceDirs( "qtplugins" );

  for (QStringList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
  {
    QString path = *it;
    if (path.endsWith("/"))
      path.truncate(path.length()-1);

    pathDb[path]=true;
  }

  QStringList paths;
  for (QMap <QString, bool>::ConstIterator it = pathDb.begin();
         it != pathDb.end(); it++)
  {
    QString path = it.key();
    bool fromKDE = it.data();

    char new_path[PATH_MAX+1];
    if (realpath(QFile::encodeName(path), new_path))
      path = QFile::decodeName(new_path);

    if (fromKDE)
    {
      if (!path.endsWith("/"))
        path += "/";
      kdeAdded.push_back(path); //Add for the new list -- do it here to have it in the right form..
    }

    paths.append(path);
  }

   //Write the list out..
  settings.writeEntry("/qt/KDE/kdeAddedLibraryPaths", kdeAdded);
  settings.writeEntry(libPathKey, paths, ':');

  /* export widget style */
  kglobals.setGroup("General");
  QString style = kglobals.readEntry("widgetStyle", KStyle::defaultStyle() );
  if (!style.isEmpty())
    settings.writeEntry("/qt/style", style);

  /* export font settings */
  settings.writeEntry("/qt/font", KGlobalSettings::generalFont().toString());

  /* ##### looks like kcmfonts skips this, so we don't do this here */
/*bool usexft = kglobals.readBoolEntry("AntiAliasing", false);
  kconfig.setGroup("General");
  settings.writeEntry("/qt/enableXft", usexft);
  settings.writeEntry("/qt/useXft", usexft); */

  /* export effects settings */
  kglobals.setGroup("KDE");
  bool effectsEnabled = kglobals.readBoolEntry("EffectsEnabled", false);
  bool fadeMenus = kglobals.readBoolEntry("EffectFadeMenu", false);
  bool fadeTooltips = kglobals.readBoolEntry("EffectFadeTooltip", false);
  bool animateCombobox = kglobals.readBoolEntry("EffectAnimateCombo", false);

  QStringList guieffects;
  if (effectsEnabled) {
    guieffects << QString("general");
    if (fadeMenus)
      guieffects << QString("fademenu");
    if (animateCombobox)
      guieffects << QString("animatecombo");
    if (fadeTooltips)
      guieffects << QString("fadetooltip");
  }
  else
    guieffects << QString("none");

  settings.writeEntry("/qt/GUIEffects", guieffects);
}

// -----------------------------------------------------------------------------

static void addColorDef(QString& s, const char* n, const QColor& col)
{
  QString tmp;

  tmp.sprintf("#define %s #%02x%02x%02x\n",
              n, col.red(), col.green(), col.blue());

  s += tmp;
}


// -----------------------------------------------------------------------------

static void copyFile(QFile& tmp, QString const& filename, bool )
{
  QFile f( filename );
  if ( f.open(IO_ReadOnly) ) {
      QCString buf( 8192 );
      while ( !f.atEnd() ) {
          int read = f.readBlock( buf.data(), buf.size() );
          if ( read > 0 )
              tmp.writeBlock( buf.data(), read );
      }
  }
}


// -----------------------------------------------------------------------------

static QString item( int i ) {
    return QString::number( i / 255.0, 'f', 3 );
}

static QString color( const QColor& col )
{
    return QString( "{ %1, %2, %3 }" ).arg( item( col.red() ) ).arg( item( col.green() ) ).arg( item( col.blue() ) );
}

static void createGtkrc( bool exportColors, const QColorGroup& cg )
{
    QCString filename = ::getenv("HOME");
    filename += "/.gtkrc-kde";
    QFile f( filename );

    if ( f.open( IO_WriteOnly) ) {
        QTextStream t( &f );
        t.setEncoding( QTextStream::Latin1 );

        t << i18n(
            "# created by KDE, %1\n"
            "#\n"
            "# If you do not want KDE to override your GTK settings, select\n"
            "# Appearance & Themes -> Colors in the Control Center and disable the checkbox\n"
            "# \"Apply colors to non-KDE applications\"\n"
            "#\n"
            "#\n").arg(QDateTime::currentDateTime().toString());

        t << "style \"default\"" << endl;

        t << "{" << endl;
        if (exportColors)
        {
          t << "  bg[NORMAL] = " << color( cg.background() ) << endl;
          t << "  bg[SELECTED] = " << color( cg.highlight() ) << endl;
          t << "  bg[INSENSITIVE] = " << color( cg.background() ) << endl;
          t << "  bg[ACTIVE] = " << color( cg.mid() ) << endl;
          t << "  bg[PRELIGHT] = " << color( cg.background() ) << endl;
          t << endl;
          t << "  base[NORMAL] = " << color( cg.base() ) << endl;
          t << "  base[SELECTED] = " << color( cg.highlight() ) << endl;
          t << "  base[INSENSITIVE] = " << color( cg.background() ) << endl;
          t << "  base[ACTIVE] = " << color( cg.base() ) << endl;
          t << "  base[PRELIGHT] = " << color( cg.base() ) << endl;
          t << endl;
          t << "  text[NORMAL] = " << color( cg.text() ) << endl;
          t << "  text[SELECTED] = " << color( cg.highlightedText() ) << endl;
          t << "  text[INSENSITIVE] = " << color( cg.mid() ) << endl;
          t << "  text[ACTIVE] = " << color( cg.text() ) << endl;
          t << "  text[PRELIGHT] = " << color( cg.text() ) << endl;
          t << endl;
          t << "  fg[NORMAL] = " << color( cg.foreground() ) << endl;
          t << "  fg[SELECTED] = " << color( cg.highlightedText() ) << endl;
          t << "  fg[INSENSITIVE] = " << color( cg.mid() ) << endl;
          t << "  fg[ACTIVE] = " << color( cg.foreground() ) << endl;
          t << "  fg[PRELIGHT] = " << color( cg.foreground() ) << endl;
        }
        t << "}" << endl;
        t << endl;
        t << "class \"*\" style \"default\"" << endl;
        t << endl;
    }
}

// -----------------------------------------------------------------------------

void runRdb( uint flags )
{
  // Obtain the application palette that is about to be set.
  QPalette newPal = KApplication::createApplicationPalette();
  bool exportColors     = flags & KRdbExportColors;
  bool exportQtColors   = flags & KRdbExportQtColors;
  bool exportQtSettings = flags & KRdbExportQtSettings;

  KSimpleConfig kglobals("kdeglobals", true);
  kglobals.setGroup("KDE");

  KTempFile tmpFile;

  if (tmpFile.status() != 0)
  {
    kdDebug() << "Couldn't open temp file" << endl;
    exit(0);
  }

  QFile &tmp = *(tmpFile.file());

  // Export colors to non-(KDE/Qt) apps (e.g. Motif, GTK+ apps)
  if (exportColors)
  {
    KGlobal::dirs()->addResourceType("appdefaults", KStandardDirs::kde_default("data") + "kdisplay/app-defaults/");
    QColorGroup cg = newPal.active();
    KGlobal::locale()->insertCatalogue("krdb");
    createGtkrc( true, cg );

    QString preproc;
    QColor backCol = cg.background();
    addColorDef(preproc, "FOREGROUND"         , cg.foreground());
    addColorDef(preproc, "BACKGROUND"         , backCol);
    addColorDef(preproc, "HIGHLIGHT"          , backCol.light(100+(2*KGlobalSettings::contrast()+4)*16/1));
    addColorDef(preproc, "LOWLIGHT"           , backCol.dark(100+(2*KGlobalSettings::contrast()+4)*10));
    addColorDef(preproc, "SELECT_BACKGROUND"  , cg.highlight());
    addColorDef(preproc, "SELECT_FOREGROUND"  , cg.highlightedText());
    addColorDef(preproc, "WINDOW_BACKGROUND"  , cg.base());
    addColorDef(preproc, "WINDOW_FOREGROUND"  , cg.foreground());
    addColorDef(preproc, "INACTIVE_BACKGROUND", KGlobalSettings::inactiveTitleColor());
    addColorDef(preproc, "INACTIVE_FOREGROUND", KGlobalSettings::inactiveTitleColor());
    addColorDef(preproc, "ACTIVE_BACKGROUND"  , KGlobalSettings::activeTitleColor());
    addColorDef(preproc, "ACTIVE_FOREGROUND"  , KGlobalSettings::activeTitleColor());
    //---------------------------------------------------------------

    tmp.writeBlock( preproc.latin1(), preproc.length() );

    QStringList list;

    QStringList adPaths = KGlobal::dirs()->findDirs("appdefaults", "");
    for (QStringList::ConstIterator it = adPaths.begin(); it != adPaths.end(); it++) {
      QDir dSys( *it );

      if ( dSys.exists() ) {
        dSys.setFilter( QDir::Files );
        dSys.setSorting( QDir::Name );
        dSys.setNameFilter("*.ad");
        list += dSys.entryList();
      }
    }

    for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++)
      copyFile(tmp, locate("appdefaults", *it ), true);
  }

  // Merge ~/.Xresources or fallback to ~/.Xdefaults
  QString homeDir = QDir::homeDirPath();
  QString xResources = homeDir + "~/.Xresources";

  // very primitive support for ~/.Xresources by appending it
  if ( QFile::exists( xResources ) )
    copyFile(tmp, xResources, true);
  else
    copyFile(tmp, homeDir + "/.Xdefaults", true);

  // Export the Xcursor theme & size settings
  QString theme = kglobals.readEntry("cursorTheme", QString());
  QString size  = kglobals.readEntry("cursorSize", QString());
  QString contents;

  if (!theme.isNull())
    contents = "Xcursor.theme: " + theme + '\n';

  if (!size.isNull())
    contents += "Xcursor.size: " + size + '\n';

  if (contents.length() > 0)
    tmp.writeBlock( contents.latin1(), contents.length() );

  tmpFile.close();

  KProcess proc;
#ifndef NDEBUG
  proc << "xrdb" << "-merge" << tmpFile.name();
#else
  proc << "xrdb" << "-quiet -merge" << tmpFile.name();
#endif
  proc.start( KProcess::Block, KProcess::Stdin );

  tmpFile.unlink();

  applyGtkStyles(exportColors);

  /* Qt exports */
  if ( exportQtColors || exportQtSettings )
  {
    QSettings* settings = new QSettings;

    if ( exportQtColors )
      applyQtColors( kglobals, *settings, newPal );    // For kcmcolors

    if ( exportQtSettings )
      applyQtSettings( kglobals, *settings );          // For kcmstyle

    delete settings;
    QApplication::flushX();

    // We let KIPC take care of ourselves, as we are in a KDE app with
    // QApp::setDesktopSettingsAware(false);
    // Instead of calling QApp::x11_apply_settings() directly, we instead
    // modify the timestamp which propagates the settings changes onto
    // Qt-only apps without adversely affecting ourselves.

    // Cheat and use the current timestamp, since we just saved to qtrc.
    QDateTime settingsstamp = QDateTime::currentDateTime();

    static Atom qt_settings_timestamp = 0;
    if (!qt_settings_timestamp) {
	 QString atomname("_QT_SETTINGS_TIMESTAMP_");
	 atomname += XDisplayName( 0 ); // Use the $DISPLAY envvar.
	 qt_settings_timestamp = XInternAtom( qt_xdisplay(), atomname.latin1(), False);
    }

    QBuffer stamp;
    QDataStream s(stamp.buffer(), IO_WriteOnly);
    s << settingsstamp;
    XChangeProperty( qt_xdisplay(), qt_xrootwin(), qt_settings_timestamp,
		     qt_settings_timestamp, 8, PropModeReplace,
		     (unsigned char*) stamp.buffer().data(),
		     stamp.buffer().size() );
    QApplication::flushX();
  }
}

