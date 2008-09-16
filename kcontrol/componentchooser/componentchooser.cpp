/***************************************************************************
                          componentchooser.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License verstion 2 as    *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/

#include "componentchooser.h"
#include "componentchooser.moc"

#include "componentchooserbrowser.h"
#include "componentchooseremail.h"
#ifdef Q_OS_UNIX
#include "componentchooserterminal.h"
#endif
#ifdef Q_WS_X11
#include "componentchooserwm.h"
#endif

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>

#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kmimetypetrader.h>
#include <kurlrequester.h>
#include <ktoolinvocation.h>
#include <klauncher_iface.h>
#include <kconfiggroup.h>
#include <KServiceTypeTrader>

class MyListBoxItem: public QListWidgetItem
{
public:
	MyListBoxItem(const QString& text, const QString &file):QListWidgetItem(text),mFile(file){}
	virtual ~MyListBoxItem(){}
	QString mFile;
};


//BEGIN  General kpart based Component selection

CfgComponent::CfgComponent(QWidget *parent)
    : QWidget(parent), Ui::ComponentConfig_UI(), CfgPlugin()
{
    setupUi( this );
    connect(ComponentSelector,SIGNAL(activated(const QString&)),this,SLOT(slotComponentChanged(const QString&)));
}

CfgComponent::~CfgComponent()
{
    qDeleteAll(m_lookupDict);
    m_lookupDict.clear();
    qDeleteAll(m_revLookupDict);
    m_revLookupDict.clear();
}

void CfgComponent::slotComponentChanged(const QString&) {
	emit changed(true);
}

void CfgComponent::save(KConfig *cfg) {
		// yes, this can happen if there are NO KTrader offers for this component
		if (!m_lookupDict[ComponentSelector->currentText()])
			return;

		QString ServiceTypeToConfigure=cfg->group("").readEntry("ServiceTypeToConfigure");
		KConfig *store = new KConfig(cfg->group("").readPathEntry("storeInFile","null"));
		KConfigGroup cg(store,cfg->group("").readEntry("valueSection"));
//		store->setGroup(cfg->group("").readEntry("valueSection"));
		cg.writePathEntry(cfg->group("").readEntry("valueName","kcm_componenchooser_null"),*m_lookupDict[ComponentSelector->currentText()]);
		store->sync();
		delete store;
		emit changed(false);
}

void CfgComponent::load(KConfig *cfg) {

	ComponentSelector->clear();
	m_lookupDict.clear();
	m_revLookupDict.clear();

	QString ServiceTypeToConfigure=cfg->group("").readEntry("ServiceTypeToConfigure");

	KService::List offers = KServiceTypeTrader::self()->query(ServiceTypeToConfigure);

	for (KService::List::Iterator tit = offers.begin(); tit != offers.end(); ++tit)
        {
		ComponentSelector->addItem((*tit)->name());
		m_lookupDict.insert((*tit)->name(),new QString((*tit)->desktopEntryName()));
		m_revLookupDict.insert((*tit)->desktopEntryName(),new QString((*tit)->name()));
	}

	KConfig *store = new KConfig(cfg->group("").readPathEntry("storeInFile","null"));
        KConfigGroup group(store, cfg->group("").readEntry("valueSection"));
	QString setting=group.readEntry(cfg->group("").readEntry("valueName","kcm_componenchooser_null"), QString());
        delete store;
	if (setting.isEmpty()) setting=cfg->group("").readEntry("defaultImplementation", QString());
	QString *tmp=m_revLookupDict[setting];
	if (tmp)
		for (int i=0;i<ComponentSelector->count();i++)
			if ((*tmp)==ComponentSelector->itemText(i))
			{
				ComponentSelector->setCurrentIndex(i);
				break;
			}
	emit changed(false);
}

void CfgComponent::defaults()
{
    //todo
}

//END  General kpart based Component selection






ComponentChooser::ComponentChooser(QWidget *parent):
    QWidget(parent), Ui::ComponentChooser_UI(), configWidget(0)
{
	setupUi(this);
	static_cast<QGridLayout*>(layout())->setRowStretch(1, 1);
	somethingChanged=false;
	latestEditedService="";

	QStringList dummy;
	QStringList services=KGlobal::dirs()->findAllResources( "data","kcm_componentchooser/*.desktop",
															KStandardDirs::NoDuplicates, dummy );
	for (QStringList::Iterator it=services.begin();it!=services.end();++it)
	{
		KConfig cfg(*it, KConfig::SimpleConfig);
		ServiceChooser->addItem(new MyListBoxItem(cfg.group("").readEntry("Name",i18n("Unknown")),(*it)));

	}
	ServiceChooser->setFixedWidth(ServiceChooser->sizeHint().width());
	ServiceChooser->model()->sort(0);
	connect(ServiceChooser,SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),this,SLOT(slotServiceSelected(QListWidgetItem*)));
	ServiceChooser->item(0)->setSelected(true);
	slotServiceSelected(ServiceChooser->item(0));

}

void ComponentChooser::slotServiceSelected(QListWidgetItem* it) {
	if (!it) return;

	if (somethingChanged) {
		if (KMessageBox::questionYesNo(this,i18n("<qt>You changed the default component of your choice, do want to save that change now ?</qt>"),QString(),KStandardGuiItem::save(),KStandardGuiItem::discard())==KMessageBox::Yes) save();
	}
	KConfig cfg(static_cast<MyListBoxItem*>(it)->mFile, KConfig::SimpleConfig);

	ComponentDescription->setText(cfg.group("").readEntry("Comment",i18n("No description available")));
	ComponentDescription->setMinimumSize(ComponentDescription->sizeHint());


	QString cfgType=cfg.group("").readEntry("configurationType");
	QWidget *newConfigWidget = 0;
	if (cfgType.isEmpty() || (cfgType=="component"))
	{
		if (!(configWidget && qobject_cast<CfgComponent*>(configWidget)))
		{
			CfgComponent* cfgcomp = new CfgComponent(configContainer);
                        cfgcomp->ChooserDocu->setText(i18n("Choose from the list below which component should be used by default for the %1 service.", it->text()));
			newConfigWidget = cfgcomp;
		}
                else
                {
                        static_cast<CfgComponent*>(configWidget)->ChooserDocu->setText(i18n("Choose from the list below which component should be used by default for the %1 service.", it->text()));
                }
	}
	else if (cfgType=="internal_email")
	{
		if (!(configWidget && qobject_cast<CfgEmailClient*>(configWidget)))
		{
			newConfigWidget = new CfgEmailClient(configContainer);
		}

	}
#ifdef Q_OS_UNIX
	else if (cfgType=="internal_terminal")
	{
		if (!(configWidget && qobject_cast<CfgTerminalEmulator*>(configWidget)))
		{
			newConfigWidget = new CfgTerminalEmulator(configContainer);
		}

	}
#ifdef Q_WS_X11
	else if (cfgType=="internal_wm")
	{
		if (!(configWidget && qobject_cast<CfgWm*>(configWidget)))
		{
			newConfigWidget = new CfgWm(configContainer);
		}

	}
#endif
#endif
	else if (cfgType=="internal_browser")
	{
		if (!(configWidget && qobject_cast<CfgBrowser*>(configWidget)))
		{
			newConfigWidget = new CfgBrowser(configContainer);
		}

	}

	if (newConfigWidget)
	{
		configContainer->addWidget(newConfigWidget);
		configContainer->setCurrentWidget (newConfigWidget);
		configContainer->removeWidget(configWidget);
		delete configWidget;
		configWidget=newConfigWidget;
		connect(configWidget,SIGNAL(changed(bool)),this,SLOT(emitChanged(bool)));
	        configContainer->setMinimumSize(configWidget->sizeHint());
	}

	if (configWidget)
		dynamic_cast<CfgPlugin*>(configWidget)->load(&cfg);

        emitChanged(false);
	latestEditedService=static_cast<MyListBoxItem*>(it)->mFile;
}


void ComponentChooser::emitChanged(bool val) {
	somethingChanged=val;
	emit changed(val);
}


ComponentChooser::~ComponentChooser()
{
	delete configWidget;
}

void ComponentChooser::load() {
	if( configWidget )
	{
		CfgPlugin * plugin = dynamic_cast<CfgPlugin*>( configWidget );
		if( plugin )
		{
			KConfig cfg(latestEditedService, KConfig::SimpleConfig);
			plugin->load( &cfg );
		}
	}
}

void ComponentChooser::save() {
	if( configWidget )
	{
		CfgPlugin* plugin = dynamic_cast<CfgPlugin*>( configWidget );
		if( plugin )
		{
			KConfig cfg(latestEditedService, KConfig::SimpleConfig);
			plugin->save( &cfg );
		}
	}
}

void ComponentChooser::restoreDefault() {
    if (configWidget)
    {
        dynamic_cast<CfgPlugin*>(configWidget)->defaults();
        emitChanged(true);
    }

/*
	txtEMailClient->setText("kmail");
	chkRunTerminal->setChecked(false);

	// Check if -e is needed, I do not think so
	terminalLE->setText("xterm");  //No need for i18n
	terminalCB->setChecked(true);
	emitChanged(false);
*/
}

// vim: sw=4 ts=4 noet
