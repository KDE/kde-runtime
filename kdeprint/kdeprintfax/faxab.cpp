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

#include "faxab.h"

#include <QLabel>
#include <QLayout>
#include <kpushbutton.h>
#include <k3listview.h>
#include <Qt3Support/Q3Header>
#include <klocale.h>
#include <kabc/stdaddressbook.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <krun.h>
#include <kstandardguiitem.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstandarddirs.h>

FaxAB::FaxAB(QWidget *parent, const char *name)
: KDialog(parent)
{
  setObjectName( name );
  QWidget *widget = new QWidget(this);
  setMainWidget(widget); 
  setButtons(None);
	m_list = new K3ListView( widget );
	m_list->addColumn( i18n( "Name" ) );
	m_list->addColumn( i18n( "Fax Number" ) );
	m_list->header()->setStretchEnabled( true, 0 );
	QLabel	*m_listlabel = new QLabel(i18n("Entries:"), widget);
	m_ok = new KPushButton(KStandardGuiItem::ok(), widget);
	QPushButton	*m_cancel = new KPushButton(KStandardGuiItem::cancel(), widget);
	QPushButton	*m_ab = new KPushButton(KGuiItem(i18n("&Edit Addressbook"), "help-contents"), widget);
	connect(m_ok, SIGNAL(clicked()), SLOT(accept()));
	connect(m_cancel, SIGNAL(clicked()), SLOT(reject()));
	connect(m_ab, SIGNAL(clicked()), SLOT(slotEditAb()));
	if(KStandardDirs::findExe("kaddressbook").isEmpty())
	   m_ab->setEnabled(false);
	m_ok->setDefault(true);

	QVBoxLayout	*l0 = new QVBoxLayout(widget);
  l0->setMargin( 10 );
  l0->setSpacing( 10 );
	l0->addWidget( m_listlabel );
	l0->addWidget( m_list );
	QHBoxLayout	*l2 = new QHBoxLayout();
  l2->setMargin( 0 );
  l2->setSpacing( 10 );
	l0->addLayout(l2, 0);
	l2->addWidget(m_ab, 0);
	l2->addStretch(1);
	l2->addWidget(m_ok, 0);
	l2->addWidget(m_cancel, 0);

	KConfigGroup conf(KGlobal::config(), "General");
	QSize defsize( 400, 200 );
	resize( conf.readEntry( "ABSize", defsize ) );

	initialize();
	connect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), SLOT(slotAbChanged(AddressBook*)));
}

FaxAB::~FaxAB()
{
	KConfigGroup conf(KGlobal::config(), "General");
	conf.writeEntry( "ABSize", size() );
}

void FaxAB::initialize()
{
	m_entries.clear();
	m_list->clear();

	KABC::AddressBook	*bk = KABC::StdAddressBook::self();
	for (KABC::AddressBook::Iterator it=bk->begin(); it!=bk->end(); ++it)
	{
		KABC::PhoneNumber::List	numbers = (*it).phoneNumbers();
		KABC::PhoneNumber::List faxNumbers;
		for (KABC::PhoneNumber::List::Iterator nit=numbers.begin(); nit!=numbers.end(); ++nit)
		{
			if (((*nit).type() & KABC::PhoneNumber::Fax) && !(*nit).number().isEmpty())
				faxNumbers << ( *nit );
		}
		if (faxNumbers.count() > 0)
		{
			for ( KABC::PhoneNumber::List::ConstIterator nit = faxNumbers.begin(); nit != faxNumbers.end(); ++nit )
			{
				FaxABEntry entry;
				entry.m_number = ( *nit );
				entry.m_enterprise = ( *it ).organization();
				if ( !( *it ).formattedName().isEmpty() )
					entry.m_name = ( *it ).formattedName();
				else
				{
					QString key = ( *it ).familyName();
					if ( !( *it ).givenName().isEmpty() )
					{
						if ( !key.isEmpty() )
							key.append( " " );
						key.append( ( *it ).givenName() );
					}
					entry.m_name = key;
				}
				entry.m_name += ( " (" + ( *nit ).typeLabel() + ")" );
				m_entries[ entry.m_name ] = entry;
			}
		}
	}

	if (m_entries.count() > 0)
	{
		for (QMap<QString,FaxABEntry>::ConstIterator it=m_entries.begin(); it!=m_entries.end(); ++it)
		{
			Q3CheckListItem *item = new Q3CheckListItem( m_list, it.key(), Q3CheckListItem::CheckBox );
			item->setText( 1, ( *it ).m_number.number() );
			item->setText( 2, ( *it ).m_enterprise );
		}
		m_list->sort();
		m_ok->setEnabled(true);
	}
	else
		m_ok->setDisabled(true);
}

void FaxAB::slotEditAb()
{
	KRun::runCommand("kaddressbook",topLevelWidget());
}

void FaxAB::slotAbChanged(AddressBook*)
{
	initialize();
}

bool FaxAB::getEntry(QStringList& number, QStringList& name, QStringList& enterprise, QWidget *parent)
{
	FaxAB	kab(parent);
	if (!kab.isValid())
	{
		KMessageBox::error(parent, i18n("No fax number found in your address book."));
		return false;
	}
	if (kab.exec())
	{
		Q3ListViewItemIterator it( kab.m_list, Q3ListViewItemIterator::Checked );
		while ( it.current() )
		{
			number << it.current()->text( 1 );
			name << it.current()->text( 0 );
			enterprise << it.current()->text( 2 );
			++it;
		}
		/*
		number = kab.m_fax->currentText();
		name = kab.m_name->currentText();
		if (kab.m_entries.contains(name))
		{
			enterprise = kab.m_entries[name][0];
		}
		*/
		return true;
	}

	return false;
}

bool FaxAB::getEntryByNumber(const QString& number, QString& name, QString& enterprise)
{
	KABC::AddressBook *bk = KABC::StdAddressBook::self();
	for (KABC::AddressBook::Iterator it=bk->begin(); it!=bk->end(); ++it)
	{
		KABC::PhoneNumber::List	numbers = (*it).phoneNumbers();
		QStringList	filteredNumbers;
		for (KABC::PhoneNumber::List::Iterator nit=numbers.begin(); nit!=numbers.end(); ++nit)
		{
			if (((*nit).type() & KABC::PhoneNumber::Fax) )
			{
				QString strippedNumber;
				for (int i = 0; i < (*nit).number().length(); ++i)
					if ((*nit).number()[i].isDigit() || ( *nit ).number()[ i ] == '+')
						strippedNumber.append((*nit).number()[i]);

				if ( strippedNumber == number)
				{
					enterprise = (*it).organization();
					name = (*it).formattedName();
					return true;
				}
			}
		}
	}

	return false;
}

bool FaxAB::isValid()
{
	return true;
	//return (m_name->count() > 0);
}

#include "faxab.moc"
