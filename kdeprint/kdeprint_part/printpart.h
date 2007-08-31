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

#ifndef PRINTPART_H
#define PRINTPART_H

#include <kparts/part.h>
#include <kparts/browserextension.h>

class PrintPartExtension;
class KMMainView;
class KAboutData;
class KIconLoader;

class PrintPart : public KParts::ReadOnlyPart
{
	Q_OBJECT
public:
	PrintPart(QWidget *parentWidget,
		  QObject *parent,
		  const QVariantList & );
	virtual ~PrintPart();

	static KAboutData *createAboutData();

protected:
	bool openFile();
	void initActions();

private:
	PrintPartExtension	*m_extension;
	KMMainView		*m_view;
	KIconLoader		*m_iconLoader;
};

class PrintPartExtension : public KParts::BrowserExtension
{
	Q_OBJECT
	friend class PrintPart;
public:
	PrintPartExtension(PrintPart *parent);
	virtual ~PrintPartExtension();
};

#endif
