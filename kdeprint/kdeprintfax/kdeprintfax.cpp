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

#include "kdeprintfax.h"
#include "faxab.h"
#include "faxctrl.h"
#include "configdlg.h"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QtGui/QDateEdit>
#include <QComboBox>
#include <QtGui/QDesktopWidget>
#include <QDateTimeEdit>

#include <kapplication.h>
#include <kstandardaction.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klistwidget.h>
#include <k3listview.h>
#include <Qt3Support/Q3Header>
#include <klocale.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmimetype.h>
#include <kseparator.h>
#include <ksystemtrayicon.h>
#include <kstatusbar.h>
#include <ksqueezedtextlabel.h>
#include <krun.h>
#include <kpushbutton.h>
#include <kurl.h>
#include <kdebug.h>
#include <kxmlguifactory.h>

KdeprintFax::KdeprintFax(QWidget *parent, const char *name)
: KXmlGuiWindow(parent)
{
  setObjectName( name );

	m_faxctrl = new FaxCtrl(this);
	m_quitAfterSend = false;
	connect(m_faxctrl, SIGNAL(message(const QString&)), SLOT(slotMessage(const QString&)));
	connect(m_faxctrl, SIGNAL(faxSent(bool)), SLOT(slotFaxSent(bool)));

	QWidget	*mainw = new QWidget(this);
	setCentralWidget(mainw);
	m_files = new KListWidget(mainw);
	connect( m_files, SIGNAL(currentItemChanged (QListWidgetItem*,QListWidgetItem*)), SLOT( slotCurrentChanged() ) );
	m_upbtn = new KPushButton( mainw );
	m_upbtn->setIcon( KIcon( "go-up" ) );
	m_upbtn->setToolTip( i18n( "Move up" ) );
	connect( m_upbtn, SIGNAL( clicked() ), SLOT( slotMoveUp() ) );
	m_upbtn->setEnabled( false );
	m_downbtn = new KPushButton( mainw );
	m_downbtn->setIcon( KIcon( "go-down" ) );
	m_downbtn->setToolTip( i18n( "Move down" ) );
	connect( m_downbtn, SIGNAL( clicked() ), SLOT( slotMoveDown() ) );
	m_downbtn->setEnabled( false );
	QLabel	*m_filelabel = new QLabel(i18n("F&iles:"), mainw);
        m_filelabel->setBuddy(m_files);
        KSeparator*m_line = new KSeparator( Qt::Horizontal, mainw);
		KSeparator *m_line2 = new KSeparator( Qt::Horizontal, mainw );
	m_numbers = new K3ListView( mainw );
	m_numbers->addColumn( i18n("Fax Number") );
	m_numbers->addColumn( i18n("Name") );
	m_numbers->addColumn( i18n("Enterprise") );
	m_numbers->header()->setStretchEnabled( true );
	m_numbers->setSelectionMode( Q3ListView::Extended );
	connect( m_numbers, SIGNAL( selectionChanged() ), SLOT( slotFaxSelectionChanged() ) );
	connect( m_numbers, SIGNAL( executed( Q3ListViewItem* ) ), SLOT( slotFaxExecuted( Q3ListViewItem* ) ) );
	m_newbtn = new KPushButton( mainw );
	m_newbtn->setIcon( SmallIcon( "edit" ) );
	m_newbtn->setToolTip( i18n( "Add fax number" ) );
	connect( m_newbtn, SIGNAL( clicked() ), SLOT( slotFaxAdd() ) );
	m_abbtn = new KPushButton( mainw );
	m_abbtn->setIcon( SmallIcon( "kaddressbook" ) );
	m_abbtn->setToolTip( i18n( "Add fax number from addressbook" ) );
	connect( m_abbtn, SIGNAL( clicked() ), SLOT( slotKab() ) );
	m_delbtn = new KPushButton( mainw );
	m_delbtn->setIcon( KIcon( "user-trash" ) );
	m_delbtn->setToolTip( i18n( "Remove fax number" ) );
	m_delbtn->setEnabled( false );
	connect( m_delbtn, SIGNAL( clicked() ), SLOT( slotFaxRemove() ) );
	QLabel	*m_commentlabel = new QLabel(i18n("&Comment:"), mainw);
	KSystemTrayIcon	*m_tray = new KSystemTrayIcon(this);
	m_tray->setIcon(SmallIcon("kdeprintfax"));
	m_tray->show();
	m_comment = new QTextEdit(mainw);
	m_comment->setWordWrapMode (QTextOption::NoWrap);
	m_comment->setLineWidth(1);
	m_commentlabel->setBuddy(m_comment);
	QLabel	*m_timelabel = new QLabel(i18n("Sched&ule:"), mainw);
	m_timecombo = new QComboBox(mainw);
	m_timecombo->addItem(i18n("Now"));
	m_timecombo->addItem(i18n("At Specified Time"));
	m_timecombo->setCurrentIndex(0);
	m_timelabel->setBuddy(m_timecombo);
	m_time = new QDateTimeEdit(mainw);
	m_time->setTime(QTime::currentTime());
	m_time->setEnabled(false);
	connect(m_timecombo, SIGNAL(activated(int)), SLOT(slotTimeComboActivated(int)));
	m_subject = new QLineEdit( mainw );
	QLabel *m_subjectlabel = new QLabel( i18n( "Su&bject:" ), mainw );
	m_subjectlabel->setBuddy( m_subject );




	QGridLayout	*l0 = new QGridLayout(mainw);
  l0->setMargin( 10 );
  l0->setSpacing( 5 );
	l0->setColumnStretch(1,1);
	l0->addWidget(m_filelabel, 0, 0, Qt::AlignLeft|Qt::AlignTop);

	QHBoxLayout *l2 = new QHBoxLayout();
  l2->setMargin( 0 );
  l2->setSpacing( 10 );
	QVBoxLayout *l3 = new QVBoxLayout();
  l3->setMargin( 0 );
  l3->setSpacing( 5 );
	l0->addLayout( l2, 0, 1 );
	l2->addWidget( m_files );
	l2->addLayout( l3 );
	//l3->addStretch( 1 );
	l3->addWidget( m_upbtn );
	l3->addWidget( m_downbtn );
	l3->addStretch( 1 );
	l0->addWidget(m_line, 1, 0, 1, 2);
  l0->addItem( new QSpacerItem( 0, 10 ), 1, 0 );
	QHBoxLayout *l5 = new QHBoxLayout();
  l5->setMargin( 0 );
  l5->setSpacing( 10 );
	QVBoxLayout *l6 = new QVBoxLayout();
  l6->setMargin( 0 );
  l6->setSpacing( 5 );
	l0->addLayout( l5, 2, 0, 4, 2 );
	l5->addWidget( m_numbers );
	l5->addLayout( l6 );
	l6->addWidget( m_newbtn );
	l6->addWidget( m_delbtn );
	l6->addWidget( m_abbtn );
	l6->addStretch( 1 );
	l0->addWidget( m_line2, 5, 0, 5, 2 );
  l0->addItem( new QSpacerItem( 0, 10 ), 5, 0 );
	l0->addWidget( m_subjectlabel, 6, 0 );
	l0->addWidget( m_subject, 6, 1 );
	l0->addWidget(m_commentlabel, 7, 0, Qt::AlignTop|Qt::AlignLeft);
	l0->addWidget(m_comment, 7, 1);
	l0->addWidget(m_timelabel, 8, 0);
	QHBoxLayout	*l1 = new QHBoxLayout();
  l1->setMargin( 0 );
  l1->setSpacing( 5 );
	l0->addLayout(l1, 8, 1);
	l1->addWidget(m_timecombo, 1);
	l1->addWidget(m_time, 0);






	m_msglabel = new KSqueezedTextLabel(statusBar());
	statusBar()->addWidget(m_msglabel, 1);
	statusBar()->insertFixedItem(i18n("Processing..."), 1);
	statusBar()->changeItem(i18n("Idle"), 1);
	statusBar()->insertFixedItem("hylafax/efax", 2);
	initActions();
	setAcceptDrops(true);
	setCaption(i18n("Send to Fax"));
	updateState();

	resize(550,500);
	QDesktopWidget *d = kapp->desktop();
	move((d->width()-width())/2, (d->height()-height())/2);
}

KdeprintFax::~KdeprintFax()
{
}

void KdeprintFax::initActions()
{
  QAction *action = 0;

  action = actionCollection()->addAction("file_add");
  action->setText( i18n("&Add File...") );
  action->setIcon( KIcon( "document-new" ) );
  action->setShortcut( Qt::Key_Insert );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotAdd()) );

  action = actionCollection()->addAction("file_remove");
  action->setText( i18n("&Remove File") );
  action->setIcon( KIcon( "list-remove" ) );
  action->setShortcut( Qt::Key_Delete );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotRemove()) );

  action = actionCollection()->addAction("fax_send");
  action->setText( i18n("&Send Fax") );
  action->setIcon( KIcon( "connection-established" ) );
  action->setShortcut( Qt::Key_Return );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotFax()) );

  action = actionCollection()->addAction("fax_stop");
  action->setText( i18n("A&bort") );
  action->setIcon( KIcon( "process-stop" ) );
  action->setShortcut( Qt::Key_Escape );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotAbort()) );

  action = actionCollection()->addAction("fax_ab");
  action->setText( i18n("A&ddress Book") );
  action->setIcon( KIcon( "kaddressbook" ) );
  action->setShortcut( Qt::CTRL+Qt::Key_A );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotKab()) );

  action = actionCollection()->addAction("fax_log");
  action->setText( i18n("V&iew Log") );
  action->setIcon( KIcon( "help-contents" ) );
  action->setShortcut( Qt::CTRL+Qt::Key_L );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotViewLog()) );

  action = actionCollection()->addAction("file_view");
  action->setText( i18n("Vi&ew File") );
  action->setIcon( KIcon( "file-find" ) );
  action->setShortcut( Qt::CTRL+Qt::Key_O );
  connect( action, SIGNAL( triggered() ), this, SLOT(slotView()) );

  action = actionCollection()->addAction("fax_add" );
  action->setText(  i18n( "&New Fax Recipient..." ) );
  action->setIcon( KIcon( "edit" ) );
  action->setShortcut( Qt::CTRL+Qt::Key_N );
  connect( action, SIGNAL( triggered() ), this, SLOT( slotFaxAdd() ) );

  KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
  setStandardToolBarMenuEnabled(true);
  KStandardAction::showMenubar(this, SLOT(slotToggleMenuBar()), actionCollection());
  KStandardAction::preferences(this, SLOT(slotConfigure()), actionCollection());
  KStandardAction::keyBindings(guiFactory(), SLOT(configureShortcuts()),
                               actionCollection());
  actionCollection()->action("fax_stop")->setEnabled(false);
  connect(this,SIGNAL(changeStateFileViewAction(bool)), SLOT(slotChangeStateFileViewAction(bool)));
  actionCollection()->action("file_remove")->setEnabled(false);
  emit changeStateFileViewAction(false);
  createGUI();
}

void KdeprintFax::slotChangeStateFileViewAction(bool _b)
{
  actionCollection()->action("file_view")->setEnabled(_b);
}


void KdeprintFax::slotToggleMenuBar()
{
	if (menuBar()->isVisible()) menuBar()->hide();
	else menuBar()->show();
}

void KdeprintFax::slotAdd()
{
	KUrl	url = KFileDialog::getOpenUrl(QString(), QString(), this);
	if (!url.isEmpty())
		addURL(url);
}

void KdeprintFax::slotRemove()
{
	if (m_files->currentItem())
		delete m_files->takeItem(m_files->currentRow());
	if (m_files->count() == 0)
	{
		actionCollection()->action("file_remove")->setEnabled(false);
		emit changeStateFileViewAction(false);
	}
}

void KdeprintFax::slotView()
{
	if (m_files->currentItem())
	{
		new KRun(KUrl( m_files->currentItem()->text() ),this);
	}
}

void KdeprintFax::slotFax()
{
	if (m_files->count() == 0)
		KMessageBox::error(this, i18n("No file to fax."));
	else if ( m_numbers->childCount() == 0 )
		KMessageBox::error(this, i18n("No fax number specified."));
	else if (m_faxctrl->send(this))
	{
		actionCollection()->action("fax_send")->setEnabled(false);
		actionCollection()->action("fax_stop")->setEnabled(true);
		statusBar()->changeItem(i18n("Processing..."), 1);
	}
	else
		KMessageBox::error(this, i18n("Unable to start Fax process."));
}

void KdeprintFax::slotAbort()
{
	m_faxctrl->abort();
}

void KdeprintFax::slotKab()
{
	QStringList	number, name, enterprise;
	if (FaxAB::getEntry(number, name, enterprise, this))
	{
		for ( int i = 0; i<number.count(); i++ )
			new Q3ListViewItem( m_numbers, number[ i ], name[ i ], enterprise[ i ] );
	}
}

void KdeprintFax::addURL(KUrl url)
{
	QString	target;
	if (KIO::NetAccess::download(url,target,this))
	{
	        QPixmap pm = KIcon( KMimeType::iconNameForUrl(url) ).pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize) );
		new QListWidgetItem(pm,target,m_files);
		actionCollection()->action("file_remove")->setEnabled(true);
		emit changeStateFileViewAction(true);
		slotCurrentChanged();
	}
	else
		KMessageBox::error(this, i18n("Unable to retrieve %1.", url.prettyUrl()));
}

void KdeprintFax::setPhone(QString phone)
{
	QString name, enterprise;
	FaxAB::getEntryByNumber(phone, name, enterprise);
	new Q3ListViewItem( m_numbers, phone, name, enterprise );
}

void KdeprintFax::sendFax( bool quitAfterSend )
{
	slotFax();
	m_quitAfterSend = quitAfterSend;
}

void KdeprintFax::dragEnterEvent(QDragEnterEvent *e)
{
  if ( !KUrl::List::fromMimeData( e->mimeData() ).isEmpty() )
    e->accept();
  else
    e->ignore();
}

void KdeprintFax::dropEvent(QDropEvent *e)
{
	KUrl::List l = KUrl::List::fromMimeData( e->mimeData() );
	if( !l.isEmpty() ) {
		for (KUrl::List::ConstIterator it = l.begin(); it != l.end(); ++it)
			addURL(*it);
	}
}

QStringList KdeprintFax::files()
{
	QStringList	l;
	for (int i=0; i<m_files->count(); i++)
		l.append(m_files->item(i)->text());
	return l;
}


int KdeprintFax::faxCount() const
{
	return m_numbers->childCount();
}

/*
Q3ListViewItem* KdeprintFax::faxItem( int i ) const
{
	Q3ListViewItem *item = m_numbers->firstChild();
	while ( i && item && item->nextSibling() )
	{
		item = item->nextSibling();
		i--;
	}
	if ( i || !item )
		kError() << "KdeprintFax::faxItem(" << i << ") => fax item index out of bound" << endl;
	return item;
}

QString KdeprintFax::number( int i ) const
{
	Q3ListViewItem *item = faxItem( i );
	return ( item ? item->text( 0 ) : QString() );
}

QString KdeprintFax::name( int i ) const
{
	Q3ListViewItem *item = faxItem( i );
	return ( item ? item->text( 1 ) : QString() );
}

QString KdeprintFax::enterprise( int i ) const
{
	Q3ListViewItem *item = faxItem( i );
	return ( item ? item->text( 2 ) : QString() );
}
*/

KdeprintFax::FaxItemList KdeprintFax::faxList() const
{
	FaxItemList list;
	Q3ListViewItemIterator it( m_numbers );
	while ( it.current() )
	{
		FaxItem item;
		item.number = it.current()->text( 0 );
		item.name = it.current()->text( 1 );
		item.enterprise = it.current()->text( 2 );
		list << item;
		++it;
	}
	return list;
}

QString KdeprintFax::comment() const
{
	return m_comment->text();
}

QString KdeprintFax::subject() const
{
	return m_subject->text();
}

void KdeprintFax::slotMessage(const QString& msg)
{
	m_msglabel->setText(msg);
}

void KdeprintFax::slotFaxSent(bool status)
{
	actionCollection()->action("fax_send")->setEnabled(true);
	actionCollection()->action("fax_stop")->setEnabled(false);
	statusBar()->changeItem(i18n("Idle"), 1);

 	if( m_quitAfterSend ) {
		slotQuit();
	}
	else {
		if (!status)
			KMessageBox::error(this, i18n("Fax error: see log message for more information."));
		slotMessage(QString());
	}
}

void KdeprintFax::slotViewLog()
{
	m_faxctrl->viewLog(this);
}

void KdeprintFax::slotConfigure()
{
	if (ConfigDlg::configure(this))
		updateState();
}

void KdeprintFax::updateState()
{
	QString	cmd = m_faxctrl->faxCommand();
	m_comment->setEnabled(cmd.indexOf("%comment") != -1);
	//m_comment->setPaper(m_comment->isEnabled() ? colorGroup().brush(QPalette::Base) : colorGroup().brush(QPalette::Background));
	if (!m_comment->isEnabled())
	{
		m_comment->setText("");
		//m_comment->setPaper( palette().color(QPalette::Background) );
	}
	//else
		//m_comment->setPaper( palette().color( QPalette::Base ) );
	/*
	m_enterprise->setEnabled(cmd.indexOf("%enterprise") != -1);
	if (!m_enterprise->isEnabled())
		m_enterprise->setText("");
	*/
	if (cmd.indexOf("%time") == -1)
	{
		m_timecombo->setCurrentIndex(0);
		m_timecombo->setEnabled(false);
		slotTimeComboActivated(0);
	}
	else
		m_timecombo->setEnabled( true );
	/*m_name->setEnabled( cmd.indexOf( "%name" ) != -1 );*/
	m_subject->setEnabled( cmd.indexOf( "%subject" ) != -1 );
	statusBar()->changeItem(m_faxctrl->faxSystem(), 2);
}

void KdeprintFax::slotQuit()
{
	close();
}

void KdeprintFax::slotTimeComboActivated(int ID)
{
	m_time->setEnabled(ID == 1);
}

QString KdeprintFax::time() const
{
	if (!m_time->isEnabled())
		return QString();
	return m_time->time().toString("hh:mm");
}

void KdeprintFax::slotMoveUp()
{
	int index = m_files->currentRow();
	if ( index > 0 )
	{
		QListWidgetItem *item = m_files->item( index );
		m_files->takeItem( index );
		m_files->insertItem( index-1 , item );
		m_files->setCurrentRow( index-1 );
	}
}

void KdeprintFax::slotMoveDown()
{
	int index = m_files->currentRow();
	if ( index >= 0 && index < ( int )m_files->count()-1 )
	{
		QListWidgetItem *item = m_files->item( index );
		m_files->takeItem( index );
		m_files->insertItem( index + 1 , item );
		m_files->setCurrentRow( index+1 );
	}
}

void KdeprintFax::slotCurrentChanged()
{
	int index = m_files->currentRow();
	m_upbtn->setEnabled( index > 0 );
	m_downbtn->setEnabled( index >=0 && index < ( int )m_files->count()-1 );
}

void KdeprintFax::slotFaxSelectionChanged()
{
	Q3ListViewItemIterator it( m_numbers, Q3ListViewItemIterator::Selected );
	m_delbtn->setEnabled( it.current() != NULL );
}

void KdeprintFax::slotFaxRemove()
{
	Q3ListViewItemIterator it( m_numbers, Q3ListViewItemIterator::Selected );
	Q3PtrList<Q3ListViewItem> items;
	items.setAutoDelete( true );
	while ( it.current() )
	{
		items.append( it.current() );
		++it;
	}
	items.clear();
	/* force this slot to be called, to update buttons state */
	slotFaxSelectionChanged();
}

void KdeprintFax::slotFaxAdd()
{
	QString number, name, enterprise;
	if ( manualFaxDialog( number, name, enterprise ) )
	{
		new Q3ListViewItem( m_numbers, number, name, enterprise );
	}
}

void KdeprintFax::slotFaxExecuted( Q3ListViewItem *item )
{
	if ( item )
	{
		QString number = item->text( 0 ), name = item->text( 1 ), enterprise = item->text( 2 );
		if ( manualFaxDialog( number, name, enterprise ) )
		{
			item->setText( 0, number );
			item->setText( 1, name );
			item->setText( 2, enterprise );
		}
	}
}

bool KdeprintFax::manualFaxDialog( QString& number, QString& name, QString& enterprise )
{
	/* dialog construction */
	KDialog dlg( this );
  dlg.setObjectName( "manualFaxDialog" );
  dlg.setModal( true );
  dlg.setCaption( i18n( "Fax Number" ) );
  dlg.setButtons( KDialog::Ok | KDialog::Cancel );
  dlg.showButtonSeparator( true );

	QWidget *mainw = new QWidget( &dlg );
	QLabel *lab0 = new QLabel( i18n( "Enter recipient fax properties." ), mainw );
	QLabel *lab1 = new QLabel( i18n( "&Number:" ), mainw );
	QLabel *lab2 = new QLabel( i18n( "N&ame:" ), mainw );
	QLabel *lab3 = new QLabel( i18n( "&Enterprise:" ), mainw );
	QLineEdit *edit_number = new QLineEdit( number, mainw );
	QLineEdit *edit_name = new QLineEdit( name, mainw );
	QLineEdit *edit_enterprise = new QLineEdit( enterprise, mainw );
	lab1->setBuddy( edit_number );
	lab2->setBuddy( edit_name );
	lab3->setBuddy( edit_enterprise );
	QGridLayout *l0 = new QGridLayout( mainw );
  l0->setMargin( 0 );
  l0->setSpacing( 5 );
	l0->setColumnStretch( 1, 1 );
	l0->addWidget( lab0, 0, 0, 1, 0);
  l0->addItem( new QSpacerItem( 0, 10 ), 1, 0 );
	l0->addWidget( lab1, 2, 0 );
	l0->addWidget( lab2, 3, 0 );
	l0->addWidget( lab3, 4, 0 );
	l0->addWidget( edit_number, 2, 1 );
	l0->addWidget( edit_name, 3, 1 );
	l0->addWidget( edit_enterprise, 4, 1 );
	dlg.setMainWidget( mainw );
	dlg.resize( 300, 10 );

	/* dialog execution */
	while ( 1 )
		if ( dlg.exec() )
		{
			if ( edit_number->text().isEmpty() )
			{
				KMessageBox::error( this, i18n( "Invalid fax number." ) );
			}
			else
			{
				number = edit_number->text();
				name = edit_name->text();
				enterprise = edit_enterprise->text();
				return true;
			}
		}
		else
			return false;
}

#include "kdeprintfax.moc"
