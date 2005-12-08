/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

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

#ifndef _MEDIUM_H_
#define _MEDIUM_H_

#include <qstring.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <QList>
#include <kurl.h>

class Medium
{
public:
	typedef QList<Medium> List;

	static const uint ID = 0;
	static const uint NAME = 1;
	static const uint LABEL = 2;
	static const uint USER_LABEL = 3;
	static const uint MOUNTABLE = 4;
	static const uint DEVICE_NODE = 5;
	static const uint MOUNT_POINT = 6;
	static const uint FS_TYPE = 7;
	static const uint MOUNTED = 8;
	static const uint BASE_URL = 9;
	static const uint MIME_TYPE = 10;
	static const uint ICON_NAME = 11;
	static const uint PROPERTIES_COUNT = 12;
	static const QString SEPARATOR;

	Medium(const QString &id, const QString &name);
	static const Medium create(const QStringList &properties);
	static List createList(const QStringList &properties);

	const QStringList &properties() const { return m_properties; };

	QString id() const { return m_properties[ID]; };
	QString name() const { return m_properties[NAME]; };
	QString label() const { return m_properties[LABEL]; };
	QString userLabel() const { return m_properties[USER_LABEL]; };
	bool isMountable() const { return m_properties[MOUNTABLE]=="true"; };
	QString deviceNode() const { return m_properties[DEVICE_NODE]; };
	QString mountPoint() const { return m_properties[MOUNT_POINT]; };
	QString fsType() const { return m_properties[FS_TYPE]; };
	bool isMounted() const { return m_properties[MOUNTED]=="true"; };
	QString baseURL() const { return m_properties[BASE_URL]; };
	QString mimeType() const { return m_properties[MIME_TYPE]; };
	QString iconName() const { return m_properties[ICON_NAME]; };

	bool needMounting() const;
	KURL prettyBaseURL() const;
	QString prettyLabel() const;

	void setName(const QString &name);
	void setLabel(const QString &label);
	void setUserLabel(const QString &label);

	bool mountableState(bool mounted);
	void mountableState(const QString &deviceNode,
	                    const QString &mountPoint,
	                    const QString &fsType, bool mounted);
	void unmountableState(const QString &baseURL = QString::null);

	void setMimeType(const QString &mimeType);
	void setIconName(const QString &iconName);

private:
	Medium();
	void loadUserLabel();

	QStringList m_properties;
};

#endif
