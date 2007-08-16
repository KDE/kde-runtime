/*
   Copyright (C) 2004 George Staikos <staikos@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */


#include "knetattach.h"

#include <QtCore/QVariant>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kdirnotify.h>

KNetAttach::KNetAttach( QWidget* parent )
    : Q3Wizard( parent ), Ui_KNetAttach()
{
    setupUi( this );

    connect(_recent, SIGNAL(toggled(bool)), _recentConnectionName, SLOT(setEnabled(bool)));
    connect(_user, SIGNAL(textChanged(const QString&)), this, SLOT(updateParametersPageStatus()));
    connect(_host, SIGNAL(textChanged(const QString&)), this, SLOT(updateParametersPageStatus()));
    connect(_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateParametersPageStatus()));
    connect(_useEncryption, SIGNAL(toggled(bool)), this, SLOT(updatePort(bool)));
    connect(_createIcon, SIGNAL(toggled(bool)), this, SLOT(updateFinishButtonText(bool)));

    setIcon(SmallIcon("knetattach"));
    disconnect(finishButton(), SIGNAL(clicked()), (QDialog*)this, SLOT(accept()));
    connect(finishButton(), SIGNAL(clicked()), this, SLOT(finished()));
    finishButton()->setText(i18n("Save && C&onnect"));
    //setResizeMode(Fixed); FIXME: make the wizard fixed-geometry
    setFinishEnabled(_folderParameters, false);
    KConfig crecent( "krecentconnections", KConfig::NoGlobals  );
    KConfigGroup recent(&crecent, "General");
    QStringList idx = recent.readEntry("Index",QStringList());
    if (idx.isEmpty()) {
	_recent->setEnabled(false);
	if (_recent->isChecked()) {
	    _webfolder->setChecked(true);
	}
    } else {
	_recent->setEnabled(true);
	_recentConnectionName->addItems(idx);
    }
}

void KNetAttach::setInformationText( const QString &type )
{
    QString text;

    if (type=="WebFolder") {
	text = i18n("Enter a name for this <i>WebFolder</i> as well as a server address, port and folder path to use and press the <b>Save & Connect</b> button.");
    } else if (type=="Fish") {
	text = i18n("Enter a name for this <i>Secure shell connection</i> as well as a server address, port and folder path to use and press the <b>Save & Connect</b> button.");
    } else if (type=="FTP") {
        text = i18n("Enter a name for this <i>File Transfer Protocol connection</i> as well as a server address and folder path to use and press the <b>Save & Connect</b> button.");
    } else if (type=="SMB") {
        text = i18n("Enter a name for this <i>Microsoft Windows network drive</i> as well as a server address and folder path to use and press the <b>Save & Connect</b> button.");
    }

    _informationText->setText(text);
}

void KNetAttach::showPage( QWidget *page )
{
    if (page == _folderType) {
    } else if (page == _folderParameters) {
	_host->setFocus();
	_connectionName->setFocus();

	if (_webfolder->isChecked()) {
	    setInformationText("WebFolder");
	    updateForProtocol("WebFolder");
	    _port->setValue(80);
	} else if (_fish->isChecked()) {
	    setInformationText("Fish");
	    updateForProtocol("Fish");
	    _port->setValue(22);
	} else if (_ftp->isChecked()) {
	    setInformationText("FTP");
	    updateForProtocol("FTP");
	    _port->setValue(21);
	    if (_path->text().isEmpty()) {
		_path->setText("/");
	    }
	} else if (_smb->isChecked()) {
	    setInformationText("SMB");
	    updateForProtocol("SMB");
	} else { //if (_recent->isChecked()) {
	    KConfig recent( "krecentconnections", KConfig::NoGlobals );
	    if (!recent.hasGroup(_recentConnectionName->currentText())) {
		KConfigGroup group = recent.group("General");
		QStringList idx = group.readEntry("Index",QStringList());
		if (idx.isEmpty()) {
		    _recent->setEnabled(false);
		    if (_recent->isChecked()) {
			_webfolder->setChecked(true);
		    }
		} else {
		    _recent->setEnabled(true);
		    _recentConnectionName->addItems(idx);
		}
		showPage(_folderType);
		return;
	    }
	    KConfigGroup group = recent.group(_recentConnectionName->currentText());
	    _type = group.readEntry("Type");
	    setInformationText(_type);
	    if (!updateForProtocol(_type)) {
		// FIXME: handle error
	    }
	    KUrl u(group.readEntry("URL"));
	    _host->setText(u.host());
	    _user->setText(u.user());
	    _path->setText(u.path());
	    if (group.hasKey("Port")) {
		_port->setValue(group.readEntry("Port",0));
	    } else {
		_port->setValue(u.port());
	    }
	    _connectionName->setText(_recentConnectionName->currentText());
	    _createIcon->setChecked(false);
	}
	updateParametersPageStatus();
    }

    Q3Wizard::showPage(page);
}


void KNetAttach::updateParametersPageStatus()
{
    setFinishEnabled(_folderParameters,
		  !_host->text().trimmed().isEmpty() &&
		  !_path->text().trimmed().isEmpty() &&
		  !_connectionName->text().trimmed().isEmpty());
}

void KNetAttach::finished()
{
    setBackEnabled(_folderParameters,false);
    setFinishEnabled(_folderParameters, false);
    KUrl url;
    if (_type == "WebFolder") {
	if (_useEncryption->isChecked()) {
	    url.setProtocol("webdavs");
	} else {
	    url.setProtocol("webdav");
	}
	url.setPort(_port->value());
    } else if (_type == "Fish") {
	url.setProtocol("fish");
	url.setPort(_port->value());
    } else if (_type == "FTP") {
	url.setProtocol("ftp");
	url.setPort(_port->value());
    } else if (_type == "SMB") {
	url.setProtocol("smb");
    } else { // recent
    }

    url.setHost(_host->text().trimmed());
    url.setUser(_user->text().trimmed());
    QString path = _path->text().trimmed();
    if (!path.startsWith('/')) {
	path = QString("/") + path;
    }
    url.setPath(path);
   _folderParameters->setEnabled(false);
    bool success = doConnectionTest(url);
   _folderParameters->setEnabled(true);
    if (!success) {
	KMessageBox::sorry(this, i18n("Unable to connect to server.  Please check your settings and try again."));
	showPage(_folderParameters);
	setBackEnabled(_folderParameters, true);
	return;
    }

    KToolInvocation::invokeBrowser(url.url());

    QString name = _connectionName->text().trimmed();

    if (_createIcon->isChecked()) {
	KGlobal::dirs()->addResourceType("remote_entries", "data", "remoteview");

	QString path = KGlobal::dirs()->saveLocation("remote_entries");
	path += name + ".desktop";
	KConfig _desktopFile( path, KConfig::OnlyLocal );
	KConfigGroup desktopFile(&_desktopFile, "Desktop Entry");
	desktopFile.writeEntry("Icon", "package-network");
	desktopFile.writeEntry("Name", name);
	desktopFile.writeEntry("Type", "Link");
	desktopFile.writeEntry("URL", url.prettyUrl());
	desktopFile.sync();
	org::kde::KDirNotify::emitFilesAdded( "remote:/" );
    }

    if (!name.isEmpty()) {
	KConfig _recent("krecentconnections", KConfig::NoGlobals);
	KConfigGroup recent(&_recent, "General");
	QStringList idx = recent.readEntry("Index",QStringList());
	_recent.deleteGroup(name); // erase anything stale
	if (idx.contains(name)) {
	    idx.removeAll(name);
	    idx.prepend(name);
	    recent.writeEntry("Index", idx);
	    recent.changeGroup(name);
	} else {
	    QString last;
	    if (!idx.isEmpty()) {
		last = idx.last();
		idx.pop_back();
	    }
	    idx.prepend(name);
	    _recent.deleteGroup(last);
	    recent.writeEntry("Index", idx);
	}
	recent.changeGroup(name);
	recent.writeEntry("URL", url.prettyUrl());
	if (_type == "WebFolder" || _type == "Fish" || _type == "FTP") {
	    recent.writeEntry("Port", _port->value());
	}
	recent.writeEntry("Type", _type);
	recent.sync();
    }

    QDialog::accept();
}


void KNetAttach::updatePort(bool encryption)
{
    if (_webfolder->isChecked()) {
	if (encryption) {
	    _port->setValue(443);
	} else {
	    _port->setValue(80);
	}
    }
}


bool KNetAttach::doConnectionTest(const KUrl& url)
{
    KIO::UDSEntry entry;
    if (KIO::NetAccess::stat(url, entry, this)) {
	// Anything to test here?
	return true;
    }
    return false;
}


bool KNetAttach::updateForProtocol(const QString& protocol)
{
    _type = protocol;
    if (protocol == "WebFolder") {
	_useEncryption->show();
	_portText->show();
	_port->show();
	_userText->show();
	_user->show();
    } else if (protocol == "Fish") {
	_useEncryption->hide();
	_portText->show();
	_port->show();
	_userText->show();
	_user->show();
    } else if (protocol == "FTP") {
	_useEncryption->hide();
	_portText->show();
	_port->show();
	_userText->show();
	_user->show();
    } else if (protocol == "SMB") {
	_useEncryption->hide();
	_portText->hide();
	_port->hide();
	_userText->hide();
	_user->hide();
    } else {
	_type = "";
	return false;
    }
    return true;
}


void KNetAttach::updateFinishButtonText(bool save)
{
    if (save) {
	finishButton()->setText(i18n("Save && C&onnect"));
    } else {
	finishButton()->setText(i18n("C&onnect"));
    }
}

// vim: ts=8 sw=4 noet
#include "knetattach.moc"
