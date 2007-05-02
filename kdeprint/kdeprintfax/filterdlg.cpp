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

#include "filterdlg.h"
#include "filterdlg.moc"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>

#include <klocale.h>

FilterDlg::FilterDlg(QWidget *parent, const char *name)
  : KDialog( parent )
{
  setObjectName( name );
  setModal( true );
  setCaption( i18n("Filter Parameters") );
  setButtons( Ok|Cancel );

	QWidget	*w = new QWidget(this);

	m_mime = new QLineEdit(w);
	m_cmd = new QLineEdit(w);
	QLabel	*m_mimelabel = new QLabel(i18n("MIME type:"), w);
	QLabel	*m_cmdlabel = new QLabel(i18n("Command:"), w);

	QGridLayout	*l0 = new QGridLayout(w);
  l0->setMargin( 10 );
  l0->setSpacing( 5 );
	l0->setColumnStretch(1, 1);
	l0->addWidget(m_mimelabel, 0, 0);
	l0->addWidget(m_cmdlabel, 1, 0);
	l0->addWidget(m_mime, 0, 1);
	l0->addWidget(m_cmd, 1, 1);

	setMainWidget(w);
	m_mime->setFocus();
	resize(300, 100);
	connect(m_mime, SIGNAL(textChanged ( const QString & )),this, SLOT(slotTextFilterChanged()));
        connect(m_cmd, SIGNAL(textChanged ( const QString & )),this, SLOT(slotTextFilterChanged()));
        slotTextFilterChanged();
}

void FilterDlg::slotTextFilterChanged( )
{
    enableButtonOk(!m_mime->text().isEmpty() && !m_cmd->text().isEmpty());
}

bool FilterDlg::doIt(QWidget *parent, QString *mime, QString *cmd)
{
	FilterDlg	dlg(parent);
	if (mime) dlg.m_mime->setText(*mime);
	if (cmd) dlg.m_cmd->setText(*cmd);
	if (dlg.exec())
	{
		if (mime) *mime = dlg.m_mime->text();
		if (cmd) *cmd = dlg.m_cmd->text();
		return true;
	}
	return false;
}
