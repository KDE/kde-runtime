/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

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

#ifndef PHONON_XINE_MEDIAQUEUE_H
#define PHONON_XINE_MEDIAQUEUE_H

#include "mediaobject.h"
#include <kurl.h>

namespace Phonon
{
namespace Xine
{
	class MediaQueue : public MediaObject
	{
		Q_OBJECT
		public:
			MediaQueue( QObject *parent = 0 );
			~MediaQueue();

			Q_INVOKABLE KUrl nextUrl() const { return m_nextUrl; }
			Q_INVOKABLE void setNextUrl( const KUrl& url );

			Q_INVOKABLE bool doCrossfade() const { return m_doCrossfade; }
			Q_INVOKABLE void setDoCrossfade( bool );

			Q_INVOKABLE qint32 timeBetweenMedia() const { return m_timeBetweenMedia; }
			Q_INVOKABLE void setTimeBetweenMedia( qint32 );

		signals:
			void needNextUrl();

        private slots:
            void streamNeedsUrl();

		private:
			KUrl m_nextUrl;
			bool m_doCrossfade;
			qint32 m_timeBetweenMedia;
	};
} // namespace Xine
} // namespace Phonon
#endif // PHONON_XINE_MEDIAQUEUE_H

// vim: sw=4 ts=4 sts=4 et tw=100
