/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#ifndef KIO_PRINT_H
#define KIO_PRINT_H

#include <kio/slavebase.h>
#include <kio/global.h>

#include <QBuffer>

class KMPrinter;
class KJob;
namespace KIO {
	class Job;
}

class KIO_Print : public QObject, public KIO::SlaveBase
{
	Q_OBJECT
public:
	KIO_Print(const QByteArray& pool, const QByteArray& app);

	void listDir(const KUrl& url);
	void get(const KUrl& url);
	void stat(const KUrl& url);

Q_SIGNALS:
  void leaveModality();

protected Q_SLOTS:
	void slotResult( KJob* );
	void slotData( KIO::Job*, const QByteArray& );
	void slotTotalSize( KJob*, qulonglong );
	void slotProcessedSize( KJob*, qulonglong );

private:
	void listRoot();
	void listDirDB( const KUrl& );
	void statDB( const KUrl& );
	bool getDBFile( const KUrl& );
	void getDB( const KUrl& );
	void showClassInfo(KMPrinter*);
	void showPrinterInfo(KMPrinter*);
	void showSpecialInfo(KMPrinter*);
	void showData(const QString&);
	QString locateData(const QString&);
	void showJobs(KMPrinter *p = 0, bool completed = false);
	void showDriver(KMPrinter*);

	bool loadTemplate(const QString& filename, QString& buffer);

	QBuffer m_httpBuffer;
	int     m_httpError;
	QString m_httpErrorTxt;
};

#endif
