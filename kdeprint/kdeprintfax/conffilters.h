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

#ifndef CONFFILTERS_H
#define CONFFILTERS_H

#include <QWidget>
#include <QPushButton>
class K3ListView;

class ConfFilters : public QWidget
{
	Q_OBJECT
public:
	explicit ConfFilters(QWidget *parent = 0, const char *name = 0);

	void load();
	void save();
protected Q_SLOTS:
	void slotAdd();
	void slotRemove();
	void slotChange();
	void slotUp();
	void slotDown();
	void updateButton();
private:
	K3ListView	*m_filters;
	QPushButton	*m_add,*m_remove,*m_up,*m_down,*m_change;
};

#endif
