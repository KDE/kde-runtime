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

#include "conffilters.h"
#include "filterdlg.h"

#include <QPushButton>
#include <QLayout>

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <Qt3Support/Q3Header>

#include <klocale.h>
#include <k3listview.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>

ConfFilters::ConfFilters(QWidget *parent, const char *name)
: QWidget(parent)
{
  setObjectName( name );

	m_filters = new K3ListView(this);
	m_filters->addColumn(i18n("Mime Type"));
	m_filters->addColumn(i18n("Command"));
	m_filters->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
	m_filters->setLineWidth(1);
	m_filters->setSorting(-1);
	m_filters->header()->setStretchEnabled(true, 1);
	connect(m_filters, SIGNAL(doubleClicked(Q3ListViewItem*)), SLOT(slotChange()));

	m_add = new QPushButton(this);
	m_add->setIcon(BarIcon("document-new"));
	m_remove = new QPushButton(this);
	m_remove->setIcon(BarIconSet("list-remove"));
	m_change = new QPushButton(this);
	m_change->setIcon(BarIconSet("search-filter"));
	m_up = new QPushButton(this);
	m_up->setIcon(BarIconSet("go-up"));
	m_down = new QPushButton(this);
	m_down->setIcon(BarIconSet("go-down"));
	connect(m_add, SIGNAL(clicked()), SLOT(slotAdd()));
	connect(m_change, SIGNAL(clicked()), SLOT(slotChange()));
	connect(m_remove, SIGNAL(clicked()), SLOT(slotRemove()));
	connect(m_up, SIGNAL(clicked()), SLOT(slotUp()));
	connect(m_down, SIGNAL(clicked()), SLOT(slotDown()));
	m_add->setToolTip( i18n("Add filter"));
	m_change->setToolTip( i18n("Modify filter"));
	m_remove->setToolTip( i18n("Remove filter"));
	m_up->setToolTip( i18n("Move filter up"));
	m_down->setToolTip( i18n("Move filter down"));

	QHBoxLayout	*l0 = new QHBoxLayout(this);
  l0->setMargin( 10 );
  l0->setMargin( 10 );
	QVBoxLayout	*l1 = new QVBoxLayout();
  l1->setMargin( 0 );
  l1->setMargin( 0 );
	l0->addWidget(m_filters, 1);
	l0->addLayout(l1, 0);
	l1->addWidget(m_add);
	l1->addWidget(m_change);
	l1->addWidget(m_remove);
	l1->addSpacing(10);
	l1->addWidget(m_up);
	l1->addWidget(m_down);
	l1->addStretch(1);
	updateButton();
	connect(m_filters, SIGNAL(selectionChanged ()),SLOT(updateButton()));
}

void ConfFilters::load()
{
	QFile	f(KStandardDirs::locate("data","kdeprintfax/faxfilters"));
	if (f.exists() && f.open(QIODevice::ReadOnly))
	{
		QTextStream	t(&f);
		QString		line;
		int		p(-1);
		Q3ListViewItem	*item(0);
		while (!t.atEnd())
		{
			line = t.readLine().trimmed();
			if ((p=line.indexOf(QRegExp("\\s"))) != -1)
			{
				QString	mime(line.left(p)), cmd(line.right(line.length()-p-1).trimmed());
				if (!mime.isEmpty() && !cmd.isEmpty())
					item = new Q3ListViewItem(m_filters, item, mime, cmd);
			}
		}
	}
}

void ConfFilters::save()
{
	Q3ListViewItem	*item = m_filters->firstChild();
	QFile	f(KStandardDirs::locateLocal("data","kdeprintfax/faxfilters"));
	if (f.open(QIODevice::WriteOnly))
	{
		QTextStream	t(&f);
		while (item)
		{
			t << item->text(0) << ' ' << item->text(1) << endl;
			item = item->nextSibling();
		}
	}
}

void ConfFilters::slotAdd()
{
	QString	mime, cmd;
	if (FilterDlg::doIt(this, &mime, &cmd))
		if (!mime.isEmpty() && !cmd.isEmpty())
		  {
		    new Q3ListViewItem(m_filters, m_filters->currentItem(), mime, cmd);
		    updateButton();
		  }
		else
			KMessageBox::error(this, i18n("Empty parameters."));
}

void ConfFilters::slotRemove()
{
	Q3ListViewItem	*item = m_filters->currentItem();
	if (item)
		delete item;
	updateButton();
}

void ConfFilters::slotChange()
{
	Q3ListViewItem	*item = m_filters->currentItem();
	if (item)
	{
		QString	mime(item->text(0)), cmd(item->text(1));
		if (FilterDlg::doIt(this, &mime, &cmd))
		{
			item->setText(0, mime);
			item->setText(1, cmd);
		}
	}
}

void ConfFilters::slotUp()
{
	Q3ListViewItem	*item = m_filters->currentItem();
	if (item && item->itemAbove())
	{
		m_filters->moveItem(item, 0, item->itemAbove()->itemAbove());
		m_filters->setCurrentItem(item);
		updateButton();
	}
}

void ConfFilters::slotDown()
{
	Q3ListViewItem	*item = m_filters->currentItem();
	if (item && item->itemBelow())
	{
		m_filters->moveItem(item, 0, item->itemBelow());
		m_filters->setCurrentItem(item);
		updateButton();
	}
}

void ConfFilters::updateButton()
{
  Q3ListViewItem	*item = m_filters->currentItem();

  bool state=item && item->itemBelow();
  m_remove->setEnabled(item);
  m_down->setEnabled(state);
  state=item && item->itemAbove();
  m_up->setEnabled(state);
  m_change->setEnabled(item);
}

#include "conffilters.moc"
