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

void KNetAttach::init()
{
    setIcon(SmallIcon("knetattach"));
    disconnect(finishButton(), SIGNAL(clicked()), (QDialog*)this, SLOT(accept()));
    connect(finishButton(), SIGNAL(clicked()), this, SLOT(finished()));
    finishButton()->setText(i18n("Save && C&onnect"));
    //setResizeMode(Fixed); FIXME: make the wizard fixed-geometry
    setFinishEnabled(_folderParameters, false);
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
	    setInformationText(_type);
	    if (!updateForProtocol(_type)) {
		// FIXME: handle error
	    }
	    KURL u(recent.readEntry("URL"));
	    _host->setText(u.host());
	    _user->setText(u.user());
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
    }

    QWizard::showPage(page);
}


void KNetAttach::updateParametersPageStatus()
{
    setFinishEnabled(_folderParameters,
		  !_host->text().stripWhiteSpace().isEmpty() &&
		  !_path->text().stripWhiteSpace().isEmpty() &&
		  !_connectionName->text().stripWhiteSpace().isEmpty());
}

void KNetAttach::finished()
{
    setBackEnabled(_folderParameters,false);
    setFinishEnabled(_folderParameters, false);
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
	url.setPort(_port->value());
    } else if (_type == "FTP") {
	url.setProtocol("ftp");
	url.setPort(_port->value());
    } else if (_type == "SMB") {
	url.setProtocol("smb");
    } else { // recent
    }

    url.setHost(_host->text().stripWhiteSpace());
    url.setUser(_user->text().stripWhiteSpace());
    QString path = _path->text().stripWhiteSpace();
    if (!path.startsWith("/")) {
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
