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

#include "confgeneral.h"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QCheckBox>

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kseparator.h>

#include <stdlib.h>
#include <kdebug.h>
ConfGeneral::ConfGeneral(QWidget *parent, const char *name)
: QWidget(parent)
{
  setObjectName( name );

	m_name = new QLineEdit(this);
	m_company = new QLineEdit(this);
	m_number = new QLineEdit(this);
	QLabel	*m_namelabel = new QLabel(i18n("&Name:"), this);
	m_namelabel->setBuddy(m_name);
	QLabel	*m_companylabel = new QLabel(i18n("&Company:"), this);
	m_companylabel->setBuddy(m_company);
	QLabel	*m_numberlabel = new QLabel(i18n("N&umber:"), this);
        m_numberlabel->setBuddy(m_number);
	KSeparator *sep = new KSeparator( this );
	m_replace_int_char = new QCheckBox( i18n( "Replace international prefix '+' with:" ), this );
	m_replace_int_char_val = new QLineEdit( this );
	m_replace_int_char_val->setEnabled( false );

	connect( m_replace_int_char, SIGNAL( toggled( bool ) ), m_replace_int_char_val, SLOT( setEnabled( bool ) ) );

	QGridLayout	*l0 = new QGridLayout(this);
  l0->setMargin(10);
  l0->setSpacing(10);
	l0->setColumnStretch(1, 1);
	l0->setRowStretch(5, 1);
	l0->addWidget(m_namelabel, 0, 0);
	l0->addWidget(m_companylabel, 1, 0);
	l0->addWidget(m_numberlabel, 2, 0);
	l0->addWidget(m_name, 0, 1);
	l0->addWidget(m_company, 1, 1);

	l0->addWidget(m_number, 2, 1);
	l0->addWidget( sep, 3, 3, 0, 1 );


	QHBoxLayout *l1 = new QHBoxLayout;
        l1->addSpacing( 10 );
        l1->setMargin( 0 );
	l0->addLayout( l1, 4, 4, 0, 1 );

	l1->addWidget( m_replace_int_char );

	l1->addWidget( m_replace_int_char_val );

}

void ConfGeneral::load()
{
	KConfigGroup conf(KGlobal::config(), "Personal");
	m_name->setText(conf.readEntry("Name", QString::fromLocal8Bit(getenv("USER"))));
	m_number->setText(conf.readEntry("Number"));
	m_company->setText(conf.readEntry("Company"));
	m_replace_int_char->setChecked( conf.readEntry( "ReplaceIntChar", false) );
	m_replace_int_char_val->setText( conf.readEntry( "ReplaceIntCharVal" ) );
}

void ConfGeneral::save()
{
	KConfigGroup conf(KGlobal::config(), "Personal");
	conf.writeEntry("Name", m_name->text());
	conf.writeEntry("Number", m_number->text());
	conf.writeEntry("Company", m_company->text());
	conf.writeEntry( "ReplaceIntChar", m_replace_int_char->isChecked() );
	conf.writeEntry( "ReplaceIntCharVal", m_replace_int_char_val->text() );
}
