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
#ifndef _KWALLETD_H_
#define _KWALLETD_H_

#include <kded/kdedmodule.h>
#include <qstring.h>
#include <qintdict.h>
#include "kwalletbackend.h"

#include <time.h>
#include <stdlib.h>

class KWalletD : public KDEDModule {
	Q_OBJECT
	K_DCOP
	public:
		KWalletD(const QCString &name);
		virtual ~KWalletD();

	k_dcop:
		// Open and unlock the wallet
		virtual int open(const QString& wallet);

		// Close and lock the wallet
		// If force = true, will close it for all users.  Behave.  This
		// can break applications, and is generally intended for use by
		// the wallet manager app only.
		virtual int close(const QString& wallet, bool force);
		virtual int close(int handle, bool force);

		// Physically deletes the wallet from disk.
		virtual int deleteWallet(const QString& wallet);

		// Returns true if the wallet is open
		virtual bool isOpen(const QString& wallet) const;
		virtual bool isOpen(int handle) const;

		// A list of all wallets
		virtual QStringList wallets() const;

		// A list of all folders in this wallet
		virtual QStringList folderList(int handle);

		// Does this wallet have this folder?
		virtual bool hasFolder(int handle, const QString& folder);

		// Create this folder
		virtual bool createFolder(int handle, const QString& folder);

		// Remove this folder
		virtual bool removeFolder(int handle, const QString& folder);

		// List of entries in this folder
		virtual QStringList entryList(int handle, const QString& folder);

		// Read an entry.  If the entry does not exist, it just
		// returns an empty result.  It is your responsibility to check
		// hasEntry() first.
		virtual QByteArray readEntry(int handle, const QString& folder, const QString& key);
		virtual QMap<QString,QString> readMap(int handle, const QString& folder, const QString& key);
		virtual QString readPassword(int handle, const QString& folder, const QString& key);

		// Rename an entry.  rc=0 on success.
		virtual int renameEntry(int handle, const QString& folder, const QString& oldName, const QString& newName);

		// Write an entry.  rc=0 on success.
		virtual int writeEntry(int handle, const QString& folder, const QString& key, const QByteArray& value);
		virtual int writeMap(int handle, const QString& folder, const QString& key, const QMap<QString,QString>& value);
		virtual int writePassword(int handle, const QString& folder, const QString& key, const QString& value);

		// Does the entry exist?
		virtual bool hasEntry(int handle, const QString& folder, const QString& key);

		// What type is the entry?
		virtual long entryType(int handle, const QString& folder, const QString& key);

		// Remove an entry.  rc=0 on success.
		virtual int removeEntry(int handle, const QString& folder, const QString& key);

	private slots:
		void slotAppUnregistered(const QCString& app);

	private:
		// This also validates the handle.  May return NULL.
		KWallet::Backend* getWallet(int handle);
		// Generate a new unique handle.
		int generateHandle();
		// Invalidate a handle (remove it from the QMap)
		void invalidateHandle(int handle);
		// Emit signals about closing wallets
		void doCloseSignals(int,const QString&);
		QIntDict<KWallet::Backend> _wallets;
		QMap<QCString,QValueList<int> > _handles;
		QMap<QString,QCString> _passwords;
};


#endif
