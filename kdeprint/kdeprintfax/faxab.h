/*
 *   kdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef FAXAB_H
#define FAXAB_H

#include <kdialog.h>
#include <QMap>
#include <kabc/phonenumber.h>

class K3ListView;
class AddressBook;
class QPushButton;

class FaxAB : public KDialog
{
	Q_OBJECT
public:
	FaxAB(QWidget *parent = 0, const char *name = 0);
	~FaxAB();
	bool isValid();

	static bool getEntry(QStringList& number, QStringList& name, QStringList& enterprise, QWidget *parent = 0);
	static bool getEntryByNumber(const QString& number, QString& name, QString& enterprise);

protected Q_SLOTS:
	void slotEditAb();
	void slotAbChanged(AddressBook*);

protected:
	void initialize();

private:
	struct FaxABEntry
	{
		QString           m_name;
		KABC::PhoneNumber m_number;
		QString           m_enterprise;
	};

	K3ListView*                  m_list;
	QMap<QString,FaxABEntry>    m_entries;
	QPushButton*                m_ok;
};

#endif
