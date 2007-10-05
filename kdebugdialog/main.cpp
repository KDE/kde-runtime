/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kdebugdialog.h"
#include "klistdebugdialog.h"
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <QTextStream>
#include <klocale.h>
#include <kdebug.h>
#include <kuniqueapplication.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QFile>

QStringList readAreaList()
{
  QStringList lst;
  lst.append( "0 (generic)" );

  QString confAreasFile = KStandardDirs::locate( "config", "kdebug.areas" );
  QFile file( confAreasFile );
  if (!file.open(QIODevice::ReadOnly)) {
    kWarning() << "Couldn't open " << confAreasFile ;
    file.close();
  }
  else
  {
    QString data;

    QTextStream *ts = new QTextStream(&file);
    ts->setCodec( "ISO-8859-1" );
    while (!ts->atEnd()) {
      data = ts->readLine().simplified();

      int pos = data.indexOf("#");
      if ( pos != -1 )
        data.truncate( pos );

      if (data.isEmpty())
        continue;

      lst.append( data );
    }

    delete ts;
    file.close();
  }

  return lst;
}

int main(int argc, char ** argv)
{
  KAboutData data( "kdebugdialog", 0, ki18n( "KDebugDialog"),
    "1.0", ki18n("A dialog box for setting preferences for debug output"),
    KAboutData::License_GPL, ki18n("Copyright 1999-2000, David Faure <email>faure@kde.org</email>"));
  data.addAuthor(ki18n("David Faure"), ki18n("Maintainer"), "faure@kde.org");
  KCmdLineArgs::init( argc, argv, &data );

  KCmdLineOptions options;
  options.add("fullmode", ki18n("Show the fully-fledged dialog instead of the default list dialog"));
  options.add("on <area>", ki18n(/*I18N_NOOP TODO*/ "Turn area on"));
  options.add("off <area>", ki18n(/*I18N_NOOP TODO*/ "Turn area off"));
  KCmdLineArgs::addCmdLineOptions( options );
  KUniqueApplication::addCmdLineOptions();
  KUniqueApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  QStringList areaList ( readAreaList() );
  KAbstractDebugDialog * dialog;
  if (args->isSet("fullmode"))
      dialog = new KDebugDialog(areaList, 0L);
  else
  {
      KListDebugDialog * listdialog = new KListDebugDialog(areaList, 0L);
      if (args->isSet("on"))
      {
          listdialog->activateArea( args->getOption("on").toUtf8(), true );
          /*listdialog->save();
          listdialog->config()->sync();
          return 0;*/
      } else if ( args->isSet("off") )
      {
          listdialog->activateArea( args->getOption("off").toUtf8(), false );
          /*listdialog->save();
          listdialog->config()->sync();
          return 0;*/
      }
      dialog = listdialog;
  }

  /* Show dialog */
  int nRet = dialog->exec();
  if( nRet == QDialog::Accepted )
  {
      dialog->save();
      dialog->config()->sync();
  }
  else
    dialog->config()->clean();

  return 0;
}
