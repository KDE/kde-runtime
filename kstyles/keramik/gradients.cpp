/* Keramik Style for KDE4, gradient routines..
   Copyright (c) 2002       Malte Starostik <malte@kde.org>
             (c) 2002, 2005 Maksim Orlovich <maksim@kde.org>

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

#include "gradients.h"
#include "colorutil.h"

#include <QtGui/QPainter>
#include <QtCore/QRect>
#include <QtGui/QColor>

#include <QtGui/QImage>
#include <QtCore/QCache>
#include <qimageblitz.h>

namespace
{
	struct GradientCacheEntry
	{
		QPixmap* m_pixmap;
		QRgb     m_color;
		bool     m_menu;
		int      m_width;
		int      m_height;
		
		GradientCacheEntry(int width, int height, const QColor& color, bool menu):
			m_pixmap(0), m_color(color.rgb()), m_menu(menu), m_width(width), m_height(height)
		{}
		
		int key()
		{
			return (int)m_menu ^ m_width ^ (m_height << 16) ^ ((m_color)<<8);
		}
		
		bool operator == (const GradientCacheEntry& other)
		{
			return ((m_width == other.m_width) &&
				    (m_height == other.m_height) &&
				    (m_menu == other.m_menu) &&
				    (m_color == other.m_color));
		}
		
		~GradientCacheEntry()
		{
			delete m_pixmap;
		}
		
	};
	
	
	static QCache<int, GradientCacheEntry> cache(65636);
	
}

using namespace Keramik;

void GradientPainter::releaseCache()
{
	cache.clear();
}

void GradientPainter::renderGradient( QPainter* p, const QRect& r, const QColor& c,
										bool horizontal, bool menu, int px, int py,
										int pwidth, int pheight)
{
	int width = r.width(), height = r.height();
	if (pwidth != -1) width = pwidth;
	if (pheight != -1) height = pheight;
	
	if (horizontal)
		width  = 18;
	else
		height = 18;
	
	GradientCacheEntry entry (width, height, c, menu);
	GradientCacheEntry* cacheEntry = 0;
	
	int key = entry.key();
	
	if ((cacheEntry = cache.take(key)))
	{
		if (entry == *cacheEntry)
		{
			p->drawTiledPixmap(r, *cacheEntry->m_pixmap, horizontal? QPoint(0,py): QPoint(px,0));
			return;
		}
	}
	
	
	if (horizontal)
	{
		QPixmap* pix = new QPixmap(18, height);
		
		if (menu)
		{
			QImage gr = Blitz::gradient(QSize(4,height), c.light(93), ColorUtil::lighten(c,109), Blitz::VerticalGradient );
			QPixmap grT(QPixmap::fromImage(gr));
			QPainter p2(pix);
			p2.drawTiledPixmap(0,0, 18, height, grT);
			p2.end();
		}
		else
		{
			int h1 = 3 * height/4;
			int h2 = height - h1;

			QImage top = Blitz::gradient(QSize(4,h1),  ColorUtil::lighten(c,110), c.light(94), Blitz::VerticalGradient );
			QImage bot = Blitz::gradient(QSize(4,h2), c.light(94), ColorUtil::lighten(c,109), Blitz::VerticalGradient );
			
			QPixmap topT(QPixmap::fromImage(top));
			QPixmap botT(QPixmap::fromImage(bot));
			
			QPainter p2(pix);
			p2.drawTiledPixmap(0, 0, 18, h1, topT);
			p2.drawTiledPixmap(0, h1, 18, h2, botT);
			p2.end();
		}
		
		entry.m_pixmap = pix;
	}
	else
	{
		QPixmap* pix = new QPixmap(width, 18);
		
		int h1 = 3 * width/4;
		int h2 = width - h1;
		
		QImage top = Blitz::gradient(QSize(h1,4), ColorUtil::lighten(c,110), c.light(94), Blitz::HorizontalGradient );
		QImage bot = Blitz::gradient(QSize(h2,4), c.light(94), ColorUtil::lighten(c,109), Blitz::HorizontalGradient );
		
		QPixmap topT(QPixmap::fromImage(top));
		QPixmap botT(QPixmap::fromImage(bot));

		QPainter p2(pix);
		p2.drawTiledPixmap(0,  0, h1, 18, topT);
		p2.drawTiledPixmap(h1, 0, h2, 18, botT);
		p2.end();
		
		entry.m_pixmap = pix;

	}
		
	
	GradientCacheEntry* imgToAdd = new GradientCacheEntry(entry);
	cache.insert(imgToAdd->key(), imgToAdd,
		imgToAdd->m_pixmap->width() * imgToAdd->m_pixmap->height()*
		imgToAdd->m_pixmap->depth()/8);
		
	p->drawTiledPixmap(r, *imgToAdd->m_pixmap, horizontal? QPoint(0,py): QPoint(px,0));
	
	entry.m_pixmap = 0;//Don't free too early..
}

// vim: ts=4 sw=4 noet
