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

#include <klocale.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kwalletentry.h>
#include <kpassdlg.h>

#include <qdir.h>

#include <assert.h>


extern "C" {
   KDEDModule *create_kwalletd(const QCString &name) {
	   return new KWalletD(name);
   }

   void *__kde_do_unload;
}


KWalletD::KWalletD(const QCString &name)
: KDEDModule(name) {
	srand(time(0));
	KGlobal::dirs()->addResourceType("kwallet", "share/apps/kwallet");
	KApplication::dcopClient()->setNotifications(true);
	connect(KApplication::dcopClient(),
		SIGNAL(applicationRemoved(const QCString&)),
		this,
		SLOT(slotAppUnregistered(const QCString&)));
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
DCOPClient *dc = callingDcopClient();
int rc = -1;
bool brandNew = false;

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
			if (_wallets.count() == 1) {
				KApplication::startServiceByDesktopName("kwalletmanager");
			}
		} else if (!_handles[dc->senderId()].contains(rc)) {
			_handles[dc->senderId()].append(rc);
		 	_wallets.find(rc)->ref();
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

	if (w) {
		// can refCount() ever be 0?
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

	if (dc && w) { // the handle is valid and we have a client
		if (_handles.contains(dc->senderId())) { // we know this app
			if (_handles[dc->senderId()].contains(handle)) {
				// the app owns this handle
				_handles[dc->senderId()].remove(handle);
			}
		}

		// watch the side effect of the deref()
		if (w->deref() == 0 || force) {
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


QMap<QString,QString> KWalletD::readMap(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry *e = b->readEntry(key);
		if (e && e->type() == KWallet::Wallet::Map) {
			return e->map();
		}
	}

return QMap<QString,QString>();
}


QByteArray KWalletD::readEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		b->setFolder(folder);
		KWallet::Entry *e = b->readEntry(key);
		if (e && e->type() == KWallet::Wallet::Stream) {
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


int KWalletD::writeMap(int handle, const QString& folder, const QString& key, const QMap<QString,QString>& value) {
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


long KWalletD::entryType(int handle, const QString& folder, const QString& key) {
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
		QValueList<int> *l = &_handles[app];
		for (QValueList<int>::Iterator i = l->begin(); i != l->end(); i++) {
			close(*i, false);
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
				return w;
			}
		}
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


void KWalletD::emitFolderUpdated(const QString& wallet, const QString& folder) {
	QByteArray data;
	QDataStream ds(data, IO_WriteOnly);
	ds << wallet;
	ds << folder;
	emitDCOPSignal("folderUpdated(QString, QString)", data);
}

#include "kwalletd.moc"
