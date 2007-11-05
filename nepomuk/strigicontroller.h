/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _NEPOMUK_STRIGI_CONTROLLER_H_
#define _NEPOMUK_STRIGI_CONTROLLER_H_

#include <QtCore/QObject>
#include <KProcess>

namespace Nepomuk {
    class StrigiController : public QObject
    {
	Q_OBJECT;

    public:
	StrigiController( QObject* parent = 0 );
	~StrigiController();

	enum State {
	    Idle,
	    StartingUp,
	    Running,
	    ShuttingDown
	};

	State state() const;

    public Q_SLOTS:
	bool start( bool useNepomuk = true );
	void shutdown();

	static bool isRunning();

    private Q_SLOTS:
	void slotProcessFinished( int exitCode, QProcess::ExitStatus exitStatus );
	void slotRunning5Minutes();

    private:
	KProcess* m_strigiProcess;
	bool m_running5Minutes;
	State m_state;
    };
}

#endif
