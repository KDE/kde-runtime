/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <unistd.h> // for getuid()

#include <kpushbutton.h>
#include <qlayout.h>
#include <klocale.h>
#include <kapplication.h>
#include <kcmodule.h>
#include <kseparator.h>
#include <kdialog.h>
#include <kstdguiitem.h>
#include <dcopclient.h>

#include <qwhatsthis.h>
#include <qlabel.h>

#include "global.h"
#include "proxywidget.h"
#include "proxywidget.moc"

#include <kdebug.h>

class WhatsThis : public QWhatsThis
{
public:
    WhatsThis( ProxyWidget* parent )
    : QWhatsThis( parent ), proxy( parent ) {}
    ~WhatsThis(){};


    QString text( const QPoint &  ) {
    if ( !proxy->quickHelp().isEmpty() )
        return proxy->quickHelp();
    else
        return i18n("The currently loaded configuration module.");
    }

private:
    ProxyWidget* proxy;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

static void setVisible(QPushButton *btn, bool vis)
{
  if (vis)
    btn->show();
  else
    btn->hide();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////


class RootInfoWidget : public QLabel
{
public:
    RootInfoWidget(QWidget *parent, const char *name);
    void setRootMsg(const QString& s) { setText(s); }
};

RootInfoWidget::RootInfoWidget(QWidget *parent, const char *name = 0)
    : QLabel(parent, name)
{
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Raised);

    setText(i18n("<b>Changes in this module require root access.</b><br>"
                      "Click the \"Administrator Mode\" button to "
                      "allow modifications in this module."));

	QWhatsThis::add(this, i18n("This module requires special permissions, probably "
                              "for system-wide modifications. Therefore it is "
                              "required that you provide the root password to be "
                              "able to change the modules properties. As long as "
                              "you don't provide the password, the module will be "
                              "disabled."));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

class ProxyView : public QScrollView
{
public:
    ProxyView(KCModule *client, const QString& title, QWidget *parent, bool run_as_root, const char *name);

private:
    virtual void resizeEvent(QResizeEvent *);

    QWidget *contentWidget;
    KCModule    *client;
    bool scroll;
};

class ProxyContentWidget : public QWidget
{
public:
    ProxyContentWidget( QWidget* parent ) : QWidget( parent ) {}
    ~ProxyContentWidget(){}

    // this should be really done by qscrollview in AutoOneFit mode!
    QSize sizeHint() const { return minimumSizeHint(); }
};


ProxyView::ProxyView(KCModule *_client, const QString&, QWidget *parent, bool run_as_root, const char *name)
    : QScrollView(parent, name), client(_client)
{
  setResizePolicy(QScrollView::AutoOneFit);
  setFrameStyle( NoFrame );
  contentWidget = new ProxyContentWidget( viewport() );

  QVBoxLayout* vbox = new QVBoxLayout( contentWidget );

  if (run_as_root && _client->useRootOnlyMsg()) // notify the user
  {
      RootInfoWidget *infoBox = new RootInfoWidget(contentWidget);
      vbox->addWidget( infoBox );
      QString msg = _client->rootOnlyMsg();
      if (!msg.isEmpty())
	      infoBox->setRootMsg(msg);
      vbox->setSpacing(KDialog::spacingHint());
  }
  client->reparent(contentWidget,0,QPoint(0,0),true);
  vbox->addWidget( client );
  vbox->activate(); // make sure we have a proper minimumSizeHint
  addChild(contentWidget);
}

void ProxyView::resizeEvent(QResizeEvent *e)
{
    QScrollView::resizeEvent(e);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////


ProxyWidget::ProxyWidget(KCModule *client, QString title, const char *name,
             bool run_as_root)
  : QWidget(0, name)
  , _client(client)
{
 setCaption(title);

 if (getuid()==0 ) {
	 // Make root modules look as similar as possible...
	 QCString replyType;
	 QByteArray replyData;
	 
	 if (kapp->dcopClient()->call("kcontrol", "moduleIface", "getPalette()", QByteArray(),
				 replyType, replyData))
		 if ( replyType == "QPalette") {
			 QDataStream reply( replyData, IO_ReadOnly );
			 QPalette pal;
			 reply >> pal;
			 setPalette(pal);
		 }
/* // Doesn't work ...
	 if (kapp->dcopClient()->call("kcontrol", "moduleIface", "getStyle()", QByteArray(),
				 replyType, replyData))
		 if ( replyType == "QString") {
			 QDataStream reply( replyData, IO_ReadOnly );
			 QString style; 
			 reply >> style;
			 setStyle(style);
		 }
*/	 
	 if (kapp->dcopClient()->call("kcontrol", "moduleIface", "getFont()", QByteArray(),
				 replyType, replyData))
		 if ( replyType == "QFont") {
			 QDataStream reply( replyData, IO_ReadOnly );
			 QFont font;
			 reply >> font;
			 setFont(font);
		 }
 }
	 
  view = new ProxyView(client, title, this, run_as_root, "proxyview");
  (void) new WhatsThis( this );

  connect(_client, SIGNAL(changed(bool)), SLOT(clientChanged(bool)));
  connect(_client, SIGNAL(quickHelpChanged()), SIGNAL(quickHelpChanged()));

  _sep = new KSeparator(KSeparator::HLine, this);

  _help =    new KPushButton( KStdGuiItem::help(), this );
  _default = new KPushButton( KStdGuiItem::defaults(), this );
  _apply =   new KPushButton( KStdGuiItem::apply(), this );
  _reset =   new KPushButton( KGuiItem( i18n( "&Reset" ), "undo" ), this );
  _root =    new KPushButton( KGuiItem(i18n( "&Administrator Mode" )), this );

  bool mayModify = (!run_as_root || !_client->useRootOnlyMsg()) && !KCGlobal::isInfoCenter();

  // only enable the requested buttons
  int b = _client->buttons();
  setVisible(_help, false & (b & KCModule::Help));
  setVisible(_default, mayModify && (b & KCModule::Default));
  setVisible(_apply, mayModify && (b & KCModule::Apply));
  setVisible(_reset, mayModify && (b & KCModule::Apply));
  setVisible(_root, run_as_root);

  // disable initial buttons
  _apply->setEnabled( client->changed() );
  _reset->setEnabled( client->changed() );

  connect(_help, SIGNAL(clicked()), SLOT(helpClicked()));
  connect(_default, SIGNAL(clicked()), SLOT(defaultClicked()));
  connect(_apply, SIGNAL(clicked()), SLOT(applyClicked()));
  connect(_reset, SIGNAL(clicked()), SLOT(resetClicked()));
  connect(_root, SIGNAL(clicked()), SLOT(rootClicked()));

  QVBoxLayout *top = new QVBoxLayout(this, KDialog::marginHint(), 
      KDialog::spacingHint());
  top->addWidget(view);
  top->addWidget(_sep);

  QHBoxLayout *buttons = new QHBoxLayout(top, 4);
  buttons->addWidget(_help);
  buttons->addWidget(_default);
  if (run_as_root) 
  {
    buttons->addWidget(_root);
  }

  buttons->addStretch(1);
  if (mayModify)
  {
    buttons->addWidget(_apply);
    buttons->addWidget(_reset);
  }

  top->activate();
}

ProxyWidget::~ProxyWidget()
{
  delete _client;
}

QString ProxyWidget::quickHelp() const
{
  if (_client)
    return _client->quickHelp();
  else
    return "";
}

void ProxyWidget::helpClicked()
{
  if (getuid()!=0)
	  emit helpRequest();
  else
     kapp->dcopClient()->send("kcontrol", "moduleIface", "invokeHelp()", QByteArray());
}

void ProxyWidget::defaultClicked()
{
  _client->defaults();
  clientChanged(true);
}

void ProxyWidget::applyClicked()
{
  _client->save();
  clientChanged(false);
  _apply->setEnabled(false);
}

void ProxyWidget::resetClicked()
{
  _client->load();
  clientChanged(false);
}

void ProxyWidget::rootClicked()
{
  emit runAsRoot();
}


void ProxyWidget::clientChanged(bool state)
{
  // enable/disable buttons
    // int b = _client->buttons();
//  _apply->setEnabled(state);
  _reset->setEnabled(state);
  _apply->setEnabled(true);

  // forward the signal
  emit changed(state);
}

const KAboutData *ProxyWidget::aboutData() const
{
  return _client->aboutData();
}
