/*
   This file is part of the KDE libraries

   Copyright (c) 2002-2003 George Staikos <staikos@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

*/

#include "kwalletd.h"

#include <dcopclient.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpassdlg.h>
#include <kstddirs.h>
#include <kwalletentry.h>

#include <qdir.h>
#include <qregexp.h>

#include <assert.h>


extern "C" {
   KDEDModule *create_kwalletd(const QCString &name) {
	   return new KWalletD(name);
   }

   void *__kde_do_unload;
}


KWalletD::KWalletD(const QCString &name)
: KDEDModule(name), _failed(0) {
	srand(time(0));
	reconfigure();
	KGlobal::dirs()->addResourceType("kwallet", "share/apps/kwallet");
	KApplication::dcopClient()->setNotifications(true);
	connect(KApplication::dcopClient(),
		SIGNAL(applicationRemoved(const QCString&)),
		this,
		SLOT(slotAppUnregistered(const QCString&)));
	_dw = new KDirWatch(this, "KWallet Directory Watcher");
	_dw->addDir(KGlobal::dirs()->saveLocation("kwallet"));
	_dw->startScan(true);
	connect(_dw, SIGNAL(dirty(const QString&)), this, SLOT(emitWalletListDirty()));
}
  

KWalletD::~KWalletD() {
	// Open wallets get closed without being saved of course.
	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		doCloseSignals(it.currentKey(), it.current()->walletName());
		delete it.current();
	}
	_wallets.clear();

	for (QMap<QString,QCString>::Iterator it = _passwords.begin();
						it != _passwords.end();
						++it) {
		it.data().fill(0);
	}
	_passwords.clear();
}


int KWalletD::generateHandle() {
int rc;

	// ASSUMPTION: RAND_MAX is fairly large.
	do {
		rc = rand();
	} while(_wallets.find(rc));

return rc;
}


int KWalletD::open(const QString& wallet) {
	if (!_enabled) { // guard
		return -1;
	}

DCOPClient *dc = callingDcopClient();
int rc = -1;
bool brandNew = false;

	if (!QRegExp("^[A-Za-z0-9]+[A-Za-z0-9\\s\\-_]*$").exactMatch(wallet)) {
		return -1;
	}

	if (dc) {
		for (QIntDictIterator<KWallet::Backend> i(_wallets); i.current(); ++i) {
			if (i.current()->walletName() == wallet) {
				rc = i.currentKey();
				break;
			}
		}

		if (rc == -1) {
			if (_wallets.count() > 20) {
				kdDebug() << "Too many wallets open." << endl;
				return -1;
			}

			KWallet::Backend *b;
			KPasswordDialog *kpd;
			if (KWallet::Backend::exists(wallet)) {
				b = new KWallet::Backend(wallet);
				kpd = new KPasswordDialog(KPasswordDialog::Password, i18n("The application '%1' has requested to open the wallet '%2'.  Please enter the password for this wallet below if you wish to open it, or cancel to deny access.").arg(dc->senderId()).arg(wallet), false);
				brandNew = true;
			} else {
				b = new KWallet::Backend(wallet);
				kpd = new KPasswordDialog(KPasswordDialog::NewPassword, i18n("The application '%1' has requested to create a new wallet named '%2'.  Please choose a password for this wallet, or cancel to deny the application's request.").arg(dc->senderId()).arg(wallet), false);
			}
			kpd->setCaption(i18n("KDE Wallet Service"));
			const char *p = 0L;
			if (kpd->exec() == KDialog::Accepted) {
				p = kpd->password();
				b->open(QByteArray().duplicate(p, strlen(p)));
			}
			if (!p || !b->isOpen()) {
				delete b;
				delete kpd;
				return -1;
			}
			_wallets.insert(rc = generateHandle(), b);
			_passwords[wallet] = p;
			_handles[dc->senderId()].append(rc);
			b->ref();
			delete kpd;
			QByteArray data;
			QDataStream ds(data, IO_WriteOnly);
			ds << wallet;
			if (brandNew) {
				emitDCOPSignal("walletCreated(QString)", data);
			}
			emitDCOPSignal("walletOpened(QString)", data);
			if (_wallets.count() == 1 && _launchManager) {
				KApplication::startServiceByDesktopName("kwalletmanager");
			}
		} else {
			int response = KMessageBox::Yes;
			
			if (_openPrompt && !_handles[dc->senderId()].contains(rc)) {
				response = KMessageBox::questionYesNo(0L, i18n("The application '%1' has requested access to the open wallet '%2'.  Do you wish to permit this?").arg(dc->senderId()).arg(wallet), i18n("KDE Wallet Service"));
			}

			if (response == KMessageBox::Yes) {
				_handles[dc->senderId()].append(rc);
			 	_wallets.find(rc)->ref();
			} else {
				return -1;
			}
		}
	}

return rc;
}


int KWalletD::deleteWallet(const QString& wallet) {
	QString path = KGlobal::dirs()->saveLocation("kwallet") + QDir::separator() + wallet + ".kwl";

	if (QFile::exists(path)) {
		close(wallet, true);
		QFile::remove(path);
		QByteArray data;
		QDataStream ds(data, IO_WriteOnly);
		ds << wallet;
		emitDCOPSignal("walletDeleted(QString)", data);
		return 0;
	}

return -1;
}


void KWalletD::changePassword(const QString& wallet) {
QIntDictIterator<KWallet::Backend> it(_wallets);
KWallet::Backend *w = 0L;
int handle = -1;
bool reclose = false;

	for (; it.current(); ++it) {
		if (it.current()->walletName() == wallet) {
			break;
		}
	}

	if (!it.current()) {
		handle = open(wallet);
		if (-1 == handle) {
			KMessageBox::sorry(0, i18n("Unable to open wallet.  The wallet must be opened in order to change the password."), i18n("KDE Wallet Service"));
			return;
		}

		w = _wallets.find(handle);
		reclose = true;
	} else {
		handle = it.currentKey();
		w = it.current();
	}

	assert(w);

	KPasswordDialog *kpd;
	kpd = new KPasswordDialog(KPasswordDialog::NewPassword, i18n("Please choose a new password for the wallet '%1'.").arg(wallet), false);
	kpd->setCaption(i18n("KDE Wallet Service"));
	if (kpd->exec() == KDialog::Accepted) {
		const char *p = kpd->password();
		if (p) {
			_passwords[wallet] = p;
			QByteArray pa;
			pa.duplicate(p, strlen(p));
			int rc = w->close(pa);
			if (rc < 0) {
				KMessageBox::sorry(0, i18n("Error re-encrypting the wallet.  Password was not changed."), i18n("KDE Wallet Service"));
				reclose = true;
			} else {
				rc = w->open(pa);
				if (rc < 0) {
					KMessageBox::sorry(0, i18n("Error reopening the wallet.  Data may be lost."), i18n("KDE Wallet Service"));
					reclose = true;
				}
			}
		}
	}

	delete kpd;
	
	if (reclose) {
		close(handle, true);
	}
}


int KWalletD::close(const QString& wallet, bool force) {
int handle = -1;
KWallet::Backend *w = 0L;

	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		if (it.current()->walletName() == wallet) {
			handle = it.currentKey();
			w = it.current();
			break;
		}
	}

return closeWallet(w, handle, force);
}


int KWalletD::closeWallet(KWallet::Backend *w, int handle, bool force) {
	if (w) {
		const QString& wallet = w->walletName();
		if (w->refCount() == 0 || force) {
			invalidateHandle(handle);
			_wallets.remove(handle);
			if (_passwords.contains(wallet)) {
				w->close(QByteArray().duplicate(_passwords[wallet].data(), _passwords[wallet].length()));
				_passwords[wallet].fill(0);
				_passwords.remove(wallet);
			}
			doCloseSignals(handle, wallet);
			delete w;
			return 0;
		}
		return 1;
	}

return -1;
}


int KWalletD::close(int handle, bool force) {
DCOPClient *dc = callingDcopClient();
KWallet::Backend *w = _wallets.find(handle);
bool contains = false;

	if (dc && w) { // the handle is valid and we have a client
		if (_handles.contains(dc->senderId())) { // we know this app
			if (_handles[dc->senderId()].contains(handle)) {
				// the app owns this handle
				_handles[dc->senderId()].remove(_handles[dc->senderId()].find(handle));
				contains = true;
				if (_handles[dc->senderId()].isEmpty()) {
					_handles.remove(dc->senderId());
				}
			}
		}

		// watch the side effect of the deref()
		if ((contains && w->deref() == 0 && !_leaveOpen) || force) {
			_wallets.remove(handle);
			if (force) {
				invalidateHandle(handle);
			}
			if (_passwords.contains(w->walletName())) {
				w->close(QByteArray().duplicate(_passwords[w->walletName()].data(), _passwords[w->walletName()].length()));
				_passwords[w->walletName()].fill(0);
				_passwords.remove(w->walletName());
			}
			doCloseSignals(handle, w->walletName());
			delete w;
			return 0;
		}
		return 1; // not closed
	}

return -1; // not open to begin with, or other error
}


bool KWalletD::isOpen(const QString& wallet) const {
	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		if (it.current()->walletName() == wallet) {
			return true;
		}
	}
return false;
}


bool KWalletD::isOpen(int handle) const {
return _wallets.find(handle) != 0;
}


QStringList KWalletD::wallets() const {
QString path = KGlobal::dirs()->saveLocation("kwallet");
QDir dir(path, "*.kwl");
QStringList rc;

	dir.setFilter(QDir::Files | QDir::NoSymLinks);

	const QFileInfoList *list = dir.entryInfoList();
	QFileInfoListIterator it(*list);
	QFileInfo *fi;
	while ((fi = it.current()) != 0L) {
		QString fn = fi->fileName();
		if (fn.endsWith(".kwl")) {
			fn.truncate(fn.length()-4);
		}
		rc += fn;
		++it;
	}
return rc;
}


QStringList KWalletD::folderList(int handle) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		return b->folderList();
	}

return QStringList();
}


bool KWalletD::hasFolder(int handle, const QString& f) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		return b->hasFolder(f);
	}

return false;
}


bool KWalletD::removeFolder(int handle, const QString& f) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		bool rc = b->removeFolder(f);
		QByteArray data;
		QDataStream ds(data, IO_WriteOnly);
		ds << b->walletName();
		emitDCOPSignal("folderListUpdated(QString)", data);
		return rc;
	}

return false;
}


bool KWalletD::createFolder(int handle, const QString& f) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		bool rc = b->createFolder(f);
		QByteArray data;
		QDataStream ds(data, IO_WriteOnly);
		ds << b->walletName();
		emitDCOPSignal("folderListUpdated(QString)", data);
		return rc;
	}

return false;
}


QByteArray KWalletD::readMap(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry *e = b->readEntry(key);
		if (e && e->type() == KWallet::Wallet::Map) {
			return e->map();
		}
	}

return QByteArray();
}


QByteArray KWalletD::readEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry *e = b->readEntry(key);
		if (e) {
			return e->value();
		}
	}

return QByteArray();
}


QStringList KWalletD::entryList(int handle, const QString& folder) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		return b->entryList();
	}

return QStringList();
}


QString KWalletD::readPassword(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry *e = b->readEntry(key);
		if (e && e->type() == KWallet::Wallet::Password) {
			return e->password();
		}
	}

return QString::null;
}


int KWalletD::writeMap(int handle, const QString& folder, const QString& key, const QByteArray& value) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry e;
		e.setKey(key);
		e.setValue(value);
		e.setType(KWallet::Wallet::Map);
		b->writeEntry(&e);
		emitFolderUpdated(b->walletName(), folder);
		return 0;
	}

return -1;
}


int KWalletD::writeEntry(int handle, const QString& folder, const QString& key, const QByteArray& value, int entryType) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry e;
		e.setKey(key);
		e.setValue(value);
		e.setType(KWallet::Wallet::EntryType(entryType));
		b->writeEntry(&e);
		emitFolderUpdated(b->walletName(), folder);
		return 0;
	}

return -1;
}


int KWalletD::writeEntry(int handle, const QString& folder, const QString& key, const QByteArray& value) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry e;
		e.setKey(key);
		e.setValue(value);
		e.setType(KWallet::Wallet::Stream);
		b->writeEntry(&e);
		emitFolderUpdated(b->walletName(), folder);
		return 0;
	}

return -1;
}


int KWalletD::writePassword(int handle, const QString& folder, const QString& key, const QString& value) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry e;
		e.setKey(key);
		e.setValue(value);
		e.setType(KWallet::Wallet::Password);
		b->writeEntry(&e);
		emitFolderUpdated(b->walletName(), folder);
		return 0;
	}

return -1;
}


int KWalletD::entryType(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return KWallet::Wallet::Unknown;
		}
		b->setFolder(folder);
		if (b->hasEntry(key)) {
			return b->readEntry(key)->type();
		}
	}

return KWallet::Wallet::Unknown;
}


bool KWalletD::hasEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return false;
		}
		b->setFolder(folder);
		return b->hasEntry(key);
	}

return false;
}


int KWalletD::removeEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return 0;
		}
		b->setFolder(folder);
		bool rc = b->removeEntry(key);
		emitFolderUpdated(b->walletName(), folder);
		return rc ? 0 : -3;
	}

return -1;
}


void KWalletD::slotAppUnregistered(const QCString& app) {
	if (_handles.contains(app)) {
		QValueList<int> l = _handles[app];
		for (QValueList<int>::Iterator i = l.begin(); i != l.end(); i++) {
			_handles[app].remove(*i);
			KWallet::Backend *w = _wallets.find(*i);
			if (w && !_leaveOpen && 0 == w->deref()) {
				close(w->walletName(), true);
			}
		}
		_handles.remove(app);
	}
}


void KWalletD::invalidateHandle(int handle) {
	for (QMap<QCString,QValueList<int> >::Iterator i = _handles.begin();
							i != _handles.end();
									++i) {
		i.data().remove(handle);
	}
}


KWallet::Backend *KWalletD::getWallet(int handle) {
DCOPClient *dc = callingDcopClient();
KWallet::Backend *w = _wallets.find(handle);

	if (dc && w) { // the handle is valid and we have a client
		if (_handles.contains(dc->senderId())) { // we know this app
			if (_handles[dc->senderId()].contains(handle)) {
				// the app owns this handle
				_failed = 0;
				return w;
			}
		}
	}

	if (++_failed > 5) {
		KMessageBox::information(0, i18n("There have been repeated failed attempts to gain access to a wallet.  An application may be misbehaving."), i18n("KDE Wallet Service"));
		_failed = 0;
	}

return 0L;
}


void KWalletD::doCloseSignals(int handle, const QString& wallet) {
	QByteArray data;
	QDataStream ds(data, IO_WriteOnly);
	ds << handle;
	emitDCOPSignal("walletClosed(int)", data);

	QByteArray data2;
	QDataStream ds2(data2, IO_WriteOnly);
	ds2 << wallet;
	emitDCOPSignal("walletClosed(QString)", data2);

	if (_wallets.isEmpty()) {
		emitDCOPSignal("allWalletsClosed()", QByteArray());
	}
}


int KWalletD::renameEntry(int handle, const QString& folder, const QString& oldName, const QString& newName) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		int rc = b->renameEntry(oldName, newName);
		emitFolderUpdated(b->walletName(), folder);
		return rc;
	}

return -1;
}


QStringList KWalletD::users(const QString& wallet) const {
QStringList rc;

	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		if (it.current()->walletName() == wallet) {
			for (QMap<QCString,QValueList<int> >::ConstIterator hit = _handles.begin(); hit != _handles.end(); ++hit) {
				if (hit.data().contains(it.currentKey())) {
					rc += hit.key();
				}
			}
			break;
		}
	}

return rc;
}


bool KWalletD::disconnectApplication(const QString& wallet, const QCString& application) {
	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		if (it.current()->walletName() == wallet) {
			if (_handles[application].contains(it.currentKey())) {
				_handles[application].remove(it.currentKey());

				if (_handles[application].isEmpty()) {
					_handles.remove(application);
				}

				if (it.current()->deref() == 0) {
					close(it.current()->walletName(), true);
				}

				QByteArray data;
				QDataStream ds(data, IO_WriteOnly);
				ds << wallet;
				ds << application;
				emitDCOPSignal("applicationDisconnected(QString,QCString)", data);

				return true;
			}
		}
	}

return false;
}


void KWalletD::emitFolderUpdated(const QString& wallet, const QString& folder) {
	QByteArray data;
	QDataStream ds(data, IO_WriteOnly);
	ds << wallet;
	ds << folder;
	emitDCOPSignal("folderUpdated(QString,QString)", data);
}


void KWalletD::emitWalletListDirty() {
	emitDCOPSignal("walletListDirty()", QByteArray());
}


void KWalletD::reconfigure() {
	KConfig cfg("kwalletrc");
	cfg.setGroup("Wallet");
	_enabled = cfg.readBoolEntry("Enabled", true);
	_launchManager = cfg.readBoolEntry("Launch Manager", true);
	_leaveOpen = cfg.readBoolEntry("Leave Open", false);
	_closeIdle = cfg.readBoolEntry("Close When Idle", false);
	_openPrompt = cfg.readBoolEntry("Prompt on Open", true);
	_idleTime = cfg.readNumEntry("Idle Timeout", 10);

	if (!_enabled) { // close all wallets
		while (!_wallets.isEmpty()) { 
			QIntDictIterator<KWallet::Backend> it(_wallets);
			if (!it.current()) { // necessary?
				break;
			}
			closeWallet(it.current(), it.currentKey(), true);
		}
	}
}


bool KWalletD::isEnabled() const {
	return _enabled;
}


#include "kwalletd.moc"
