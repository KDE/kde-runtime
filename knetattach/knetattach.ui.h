/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <kstandarddirs.h>
#include <kdirnotify_stub.h>

void KNetAttach::init()
{
    disconnect(finishButton(), SIGNAL(clicked()), (QDialog*)this, SLOT(accept()));
    connect(finishButton(), SIGNAL(clicked()), this, SLOT(finished()));
    //setResizeMode(Fixed); FIXME: make the wizard fixed-geometry
    setNextEnabled(_folderParameters, false);
    setFinishEnabled(_connectPage, false);
    KConfig recent("krecentconnections", true, false);
    recent.setGroup("General");
    QStringList idx = recent.readListEntry("Index");
    if (idx.isEmpty()) {
	_recent->setEnabled(false);
	if (_recent->isChecked()) {
	    _webfolder->setChecked(true);
	}
    } else {
	_recent->setEnabled(true);
	_recentConnectionName->insertStringList(idx);
    }
}


void KNetAttach::showPage( QWidget *page )
{
    if (page == _folderType) {
    } else if (page == _folderParameters) {
	_host->setFocus();
	if (_webfolder->isChecked()) {
	    updateForProtocol("WebFolder");
	    _port->setValue(80);
	} else if (_fish->isChecked()) {
	    updateForProtocol("Fish");
	    _port->setValue(22);
	} else if (_smb->isChecked()) {
	    updateForProtocol("SMB");
	} else { //if (_recent->isChecked()) {
	    KConfig recent("krecentconnections", true, false);
	    if (!recent.hasGroup(_recentConnectionName->currentText())) {
		recent.setGroup("General");
		QStringList idx = recent.readListEntry("Index");
		if (idx.isEmpty()) {
		    _recent->setEnabled(false);
		    if (_recent->isChecked()) {
			_webfolder->setChecked(true);
		    }
		} else {
		    _recent->setEnabled(true);
		    _recentConnectionName->insertStringList(idx);
		}
		showPage(_folderType);
		return;
	    }
	    recent.setGroup(_recentConnectionName->currentText());
	    _type = recent.readEntry("Type");
	    if (!updateForProtocol(_type)) {
		// FIXME: handle error
	    }
	    KURL u(recent.readEntry("URL"));
	    _host->setText(u.host());
	    _path->setText(u.path());
	    if (recent.hasKey("Port")) {
		_port->setValue(recent.readNumEntry("Port"));
	    } else {
		_port->setValue(u.port());
	    }
	    _connectionName->setText(_recentConnectionName->currentText());
	    _createIcon->setChecked(false);
	}
	updateParametersPageStatus();
    } else if (page == _connectPage) {
	_connectionName->setFocus();
	updateConnectPageStatus();
    }

    QWizard::showPage(page);
}


void KNetAttach::updateParametersPageStatus()
{
    setNextEnabled(_folderParameters, !_host->text().stripWhiteSpace().isEmpty() && !_path->text().stripWhiteSpace().isEmpty());
}


void KNetAttach::updateConnectPageStatus()
{
    setFinishEnabled(_connectPage, !_createIcon->isChecked() || !_connectionName->text().stripWhiteSpace().isEmpty());
}


void KNetAttach::finished()
{
    setBackEnabled(_connectPage, false);
    setFinishEnabled(_connectPage, false);
    KURL url;
    if (_type == "WebFolder") {
	if (_useEncryption->isChecked()) {
	    url.setProtocol("webdavs");
	} else {
	    url.setProtocol("webdav");
	}
	url.setPort(_port->value());
    } else if (_type == "Fish") {
	url.setProtocol("fish");
    } else if (_type == "SMB") {
	url.setProtocol("smb");
    } else { // recent
    }

    url.setHost(_host->text().stripWhiteSpace());
    QString path = _path->text().stripWhiteSpace();
    if (!path.startsWith("/")) {
	path = QString("/") + path;
    }
    url.setPath(path);
    _connectPage->setEnabled(false);
    bool success = doConnectionTest(url);
    _connectPage->setEnabled(true);
    if (!success) {
	KMessageBox::sorry(this, i18n("Unable to connect to server.  Please check your settings and try again."));
	showPage(_folderParameters);
	setBackEnabled(_connectPage, true);
	return;
    }

    kapp->invokeBrowser(url.url());

    QString name = _connectionName->text().stripWhiteSpace();

    if (_createIcon->isChecked()) {
	KGlobal::dirs()->addResourceType("remote_entries",
		KStandardDirs::kde_default("data") + "remoteview");

	QString path = KGlobal::dirs()->saveLocation("remote_entries");
	path += name + ".desktop";
	KSimpleConfig desktopFile(path, false);
	desktopFile.setGroup("Desktop Entry");
	desktopFile.writeEntry("Icon", "package_network");
	desktopFile.writeEntry("Name", name);
	desktopFile.writeEntry("Type", "Link");
	desktopFile.writeEntry("URL", url.prettyURL());
	desktopFile.sync();
	KDirNotify_stub notifier("*", "*");
	notifier.FilesAdded( "remote:/" );
    }

    if (!name.isEmpty()) {
	KConfig recent("krecentconnections", false, false);
	recent.setGroup("General");
	QStringList idx = recent.readListEntry("Index");
	recent.deleteGroup(name); // erase anything stale
	if (idx.contains(name)) {
	    idx.remove(name);
	    idx.prepend(name);
	    recent.writeEntry("Index", idx);
	    recent.setGroup(name);
	} else {
	    QString last;
	    if (!idx.isEmpty()) {
		last = idx.last();
		idx.pop_back();
	    }
	    idx.prepend(name);
	    recent.deleteGroup(last);
	    recent.writeEntry("Index", idx);
	}
	recent.setGroup(name);
	recent.writeEntry("URL", url.prettyURL());
	if (_type == "WebFolder" || _type == "Fish") {
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


bool KNetAttach::doConnectionTest(const KURL& url)
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
    } else if (protocol == "Fish") {
	_useEncryption->hide();
	_portText->show();
	_port->show();
    } else if (protocol == "SMB") {
	_useEncryption->hide();
	_portText->hide();
	_port->hide();
    } else {
	_type = "";
	return false;
    }
    return true;
}
// vim: ts=8 sw=4 noet
