/***************************************************************************
 *   Copyright (C) 2004,2005 by Jakub Stachowski                           *
 *   qbast@go2.pl                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                      *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "kcmdnssd.h"

#include <sys/stat.h>
#include <config-runtime.h>

#include <QtDBus/QtDBus>

//Added by qt3to4:

#include <klocale.h>
#include <kaboutdata.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <dnssd/settings.h>
#include <dnssd/domainbrowser.h>

#define MDNSD_CONF "/etc/mdnsd.conf"
#define MDNSD_PID "/var/run/mdnsd.pid"

K_PLUGIN_FACTORY(KCMDnssdFactory, registerPlugin<KCMDnssd>();)
K_EXPORT_PLUGIN(KCMDnssdFactory("kcmkdnssd"))

KCMDnssd::KCMDnssd(QWidget *parent, const QVariantList&)
		: KCModule( KCMDnssdFactory::componentData(), parent)
{

    widget = new Ui_ConfigDialog();
    widget->setupUi(this);
    setAboutData(new KAboutData("kcm_kdnssd", 0,
	                            ki18n("ZeroConf configuration"),0,KLocalizedString(),KAboutData::License_GPL,
	                            ki18n("(C) 2004-2007 Jakub Stachowski")));
    setQuickHelp(i18n("Setup services browsing with ZeroConf"));
    addConfig(DNSSD::Configuration::self(),this);
    setButtons( Default|Apply );
}

KCMDnssd::~KCMDnssd()
{
}

void KCMDnssd::save()
{
	KCModule::save();

	// Send signal to all kde applications which have a DNSSD::DomainBrowserPrivate instance
	QDBusMessage message =
            QDBusMessage::createSignal("/libdnssd", "org.kde.DNSSD.DomainBrowser", "domainListChanged");
	QDBusConnection::sessionBus().send(message);
}


#include "kcmdnssd.moc"

