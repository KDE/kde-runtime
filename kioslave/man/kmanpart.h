/*  This file is part of the KDE project
    Copyright (C) 2002 Alexander Neundorf <neundorf@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/


#ifndef KMANPART_H
#define KMANPART_H

#include <kparts/factory.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <khtml_part.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

#include <qcstring.h>

class KInstance;
class KAboutData;

class KManPartFactory: public KParts::Factory
{
   Q_OBJECT
   public:
      KManPartFactory( QObject * parent = 0, const char * name = 0 );
      virtual ~KManPartFactory();

      virtual KParts::Part* createPartObject( QWidget * parentWidget, const char * widgetName ,
                                QObject* parent, const char* name, const char * classname,
                                const QStringList &args);

      static KInstance * instance();

   private:
      static KInstance * s_instance;
      static KAboutData * s_about;

};

class KManPart : public KHTMLPart
{
   Q_OBJECT
   public:
      KManPart( QWidget * parent, const char * name = 0L );
      KParts::BrowserExtension * extension() {return m_extension;};

   public slots:
      virtual bool openURL( const KURL &url );
   protected slots:
      void readData(KIO::Job * , const QByteArray & data);
      void jobDone( KIO::Job *);
   protected:
      virtual bool openFile();
      KInstance *m_instance;
      KParts::BrowserExtension * m_extension;
      KIO::TransferJob *m_job;
      QCString m_htmlData;
};

#endif

