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

#include "medium.h"

#include <kconfig.h>
#include <klocale.h>

const QString Medium::SEPARATOR = "---";

Medium::Medium(const QString &id, const QString &name)
{
	m_properties+= id; /* ID */
	m_properties+= name; /* NAME */
	m_properties+= name; /* LABEL */
	m_properties+= QString(); /* USER_LABEL */

	m_properties+= "false"; /* MOUNTABLE */
	m_properties+= QString(); /* DEVICE_NODE */
	m_properties+= QString(); /* MOUNT_POINT */
	m_properties+= QString(); /* FS_TYPE */
	m_properties+= "false"; /* MOUNTED */
	m_properties+= QString(); /* BASE_URL */
	m_properties+= QString(); /* MIME_TYPE */
	m_properties+= QString(); /* ICON_NAME */

	loadUserLabel();
}

Medium::Medium()
{
	m_properties+= QString(); /* ID */
	m_properties+= QString(); /* NAME */
	m_properties+= QString(); /* LABEL */
	m_properties+= QString(); /* USER_LABEL */

	m_properties+= QString(); /* MOUNTABLE */
	m_properties+= QString(); /* DEVICE_NODE */
	m_properties+= QString(); /* MOUNT_POINT */
	m_properties+= QString(); /* FS_TYPE */
	m_properties+= QString(); /* MOUNTED */
	m_properties+= QString(); /* BASE_URL */
	m_properties+= QString(); /* MIME_TYPE */
	m_properties+= QString(); /* ICON_NAME */
}

const Medium Medium::create(const QStringList &properties)
{
	Medium m;

	if ( (uint)properties.size() >= PROPERTIES_COUNT )
	{
		m.m_properties[ID] = properties[ID];
		m.m_properties[NAME] = properties[NAME];
		m.m_properties[LABEL] = properties[LABEL];
		m.m_properties[USER_LABEL] = properties[USER_LABEL];

		m.m_properties[MOUNTABLE] = properties[MOUNTABLE];
		m.m_properties[DEVICE_NODE] = properties[DEVICE_NODE];
		m.m_properties[MOUNT_POINT] = properties[MOUNT_POINT];
		m.m_properties[FS_TYPE] = properties[FS_TYPE];
		m.m_properties[MOUNTED] = properties[MOUNTED];
		m.m_properties[BASE_URL] = properties[BASE_URL];
		m.m_properties[MIME_TYPE] = properties[MIME_TYPE];
		m.m_properties[ICON_NAME] = properties[ICON_NAME];
	}

	return m;
}

Medium::List Medium::createList(const QStringList &properties)
{
	List l;

	if ( properties.size() % (PROPERTIES_COUNT+1) == 0)
	{
		int media_count = properties.size()/(PROPERTIES_COUNT+1);

		QStringList props = properties;

		for(int i=0; i<media_count; i++)
		{
			const Medium m = create(props);
			l.append(m);

			QStringList::iterator first = props.begin();
			QStringList::iterator last = props.find(SEPARATOR);
			++last;
			props.erase(first, last);
		}
	}

	return l;
}


void Medium::setName(const QString &name)
{
	m_properties[NAME] = name;
}

void Medium::setLabel(const QString &label)
{
	m_properties[LABEL] = label;
}

void Medium::setUserLabel(const QString &label)
{
	KConfig cfg("mediamanagerrc");
	cfg.setGroup("UserLabels");

	QString entry_name = m_properties[ID];

	if ( label.isNull() )
	{
		cfg.deleteEntry(entry_name);
	}
	else
	{
		cfg.writeEntry(entry_name, label);
	}

	m_properties[USER_LABEL] = label;
}

void Medium::loadUserLabel()
{
	KConfig cfg("mediamanagerrc");
	cfg.setGroup("UserLabels");

	QString entry_name = m_properties[ID];

	if ( cfg.hasKey(entry_name) )
	{
		m_properties[USER_LABEL] = cfg.readEntry(entry_name, QString());
	}
	else
	{
		m_properties[USER_LABEL].clear();
	}
}


bool Medium::mountableState(bool mounted)
{
	if ( m_properties[DEVICE_NODE].isEmpty()
	  || m_properties[MOUNT_POINT].isEmpty() )
	{
		return false;
	}

	m_properties[MOUNTABLE] = "true";
	m_properties[MOUNTED] = ( mounted ? "true" : "false" );

	return true;
}

void Medium::mountableState(const QString &deviceNode,
                            const QString &mountPoint,
                            const QString &fsType, bool mounted)
{
	m_properties[MOUNTABLE] = "true";
	m_properties[DEVICE_NODE] = deviceNode;
	m_properties[MOUNT_POINT] = mountPoint;
	m_properties[FS_TYPE] = fsType;
	m_properties[MOUNTED] = ( mounted ? "true" : "false" );
}

void Medium::unmountableState(const QString &baseURL)
{
	m_properties[MOUNTABLE] = "false";
	m_properties[BASE_URL] = baseURL;
}

void Medium::setMimeType(const QString &mimeType)
{
	m_properties[MIME_TYPE] = mimeType;
}

void Medium::setIconName(const QString &iconName)
{
	m_properties[ICON_NAME] = iconName;
}

bool Medium::needMounting() const
{
	return isMountable() && !isMounted();
}

KURL Medium::prettyBaseURL() const
{
	if ( isMountable() )
	{
		return KURL( mountPoint() );
	}
	else
	{
		return KURL( baseURL() );
	}
}

QString Medium::prettyLabel() const
{
	if ( !userLabel().isEmpty() )
	{
		return userLabel();
	}
	else
	{
		return label();
	}
}

