/*
  This file is part of the KDE libraries

  Copyright (c) 2006 Thiago Macieira <thiago@kde.org>

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
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

*/

#ifndef _KWALLETDADAPTOR_H_
#define _KWALLETDADAPTOR_H_

#include <dbus/qdbus.h>

class KWalletDAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KWalletD")
    KWalletD *parent;
public:
    inline KWalletDAdaptor(KWalletD *p)
        : QDBusAbstractAdaptor(p), parent(p)
        { setAutoRelaySignals(true); }

public Q_SLOTS:
    // Is the wallet enabled?  If not, all open() calls fail.
    inline bool isEnabled() const
        { return parent->isEnabled(); }

    // Open and unlock the wallet
    inline int open(const QString& wallet, qlonglong wId, const QDBusMessage& msg)
        { return parent->open(wallet, wId, msg); }

    // Open and unlock the wallet with this path
    inline int openPath(const QString& path, qlonglong wId, const QDBusMessage& msg)
        { return parent->openPath(path, wId, msg); }

    // disabled -thiago
    //virtual void openAsynchronous(const QString& wallet, const QByteArray& returnObject, uint wId);

    // Close and lock the wallet
    // If force = true, will close it for all users.  Behave.  This
    // can break applications, and is generally intended for use by
    // the wallet manager app only.
    inline int close(const QString& wallet, bool force)
        { return parent->close(wallet, force); }
    inline int close(int handle, bool force, const QDBusMessage& msg)
        { return parent->close(handle, force, msg); }

    // Save to disk but leave open
    inline Q_ASYNC void sync(int handle, const QDBusMessage& msg)
        { parent->sync(handle, msg); }

    // Physically deletes the wallet from disk.
    inline int deleteWallet(const QString& wallet)
        { return parent->deleteWallet(wallet); }

    // Returns true if the wallet is open
    inline bool isOpen(const QString& wallet)
        { return parent->isOpen(wallet); }
    inline bool isOpen(int handle)
        { return parent->isOpen(handle); }

    // List the users of this wallet
    inline QStringList users(const QString& wallet) const
        { return parent->users(wallet); }

    // Change the password of this wallet
    inline void changePassword(const QString& wallet, qlonglong wId, const QDBusMessage& msg)
        { parent->changePassword(wallet, wId, msg); }

    // A list of all wallets
    inline QStringList wallets() const
        { return parent->wallets(); }

    // A list of all folders in this wallet
    inline QStringList folderList(int handle, const QDBusMessage& msg)
        { return parent->folderList(handle, msg); }

    // Does this wallet have this folder?
    inline bool hasFolder(int handle, const QString& folder, const QDBusMessage& msg)
        { return parent->hasFolder(handle, folder, msg); }

    // Create this folder
    inline bool createFolder(int handle, const QString& folder, const QDBusMessage& msg)
        { return parent->createFolder(handle, folder, msg); }

    // Remove this folder
    inline bool removeFolder(int handle, const QString& folder, const QDBusMessage& msg)
        { return parent->removeFolder(handle, folder, msg); }

    // List of entries in this folder
    inline QStringList entryList(int handle, const QString& folder, const QDBusMessage& msg)
        { return parent->entryList(handle, folder, msg); }

    // Read an entry.  If the entry does not exist, it just
    // returns an empty result.  It is your responsibility to check
    // hasEntry() first.
    inline QByteArray readEntry(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->readEntry(handle, folder, key, msg); }
    inline QByteArray readMap(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->readMap(handle, folder, key, msg); }
    inline QString readPassword(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->readPassword(handle, folder, key, msg); }
    inline QVariantMap readEntryList(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->readEntryList(handle, folder, key, msg); }
    inline QVariantMap readMapList(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->readMapList(handle, folder, key, msg); }
    inline QVariantMap readPasswordList(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->readPasswordList(handle, folder, key, msg); }

    // Rename an entry.  rc=0 on success.
    inline int renameEntry(int handle, const QString& folder, const QString& oldName, const QString& newName, const QDBusMessage& msg)
        { return parent->renameEntry(handle, folder, oldName, newName, msg); }

    // Write an entry.  rc=0 on success.
    inline int writeEntry(int handle, const QString& folder, const QString& key, const QByteArray& value, int entryType, const QDBusMessage& msg)
        { return parent->writeEntry(handle, folder, key, value, entryType, msg); }
    inline int writeEntry(int handle, const QString& folder, const QString& key, const QByteArray& value, const QDBusMessage& msg)
        { return parent->writeEntry(handle, folder, key, value, msg); }
    inline int writeMap(int handle, const QString& folder, const QString& key, const QByteArray& value, const QDBusMessage& msg)
        { return parent->writeMap(handle, folder, key, value, msg); }
    inline int writePassword(int handle, const QString& folder, const QString& key, const QString& value, const QDBusMessage& msg)
        { return parent->writePassword(handle, folder, key, value, msg); }

    // Does the entry exist?
    inline bool hasEntry(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->hasEntry(handle, folder, key, msg); }

    // What type is the entry?
    inline int entryType(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->entryType(handle, folder, key, msg); }

    // Remove an entry.  rc=0 on success.
    inline int removeEntry(int handle, const QString& folder, const QString& key, const QDBusMessage& msg)
        { return parent->removeEntry(handle, folder, key, msg); }

    // Disconnect an app from a wallet
    inline bool disconnectApplication(const QString& wallet, const QString& application)
        { return parent->disconnectApplication(wallet, application); }

    inline void reconfigure()
        { parent->reconfigure(); }

    // Determine
    inline bool folderDoesNotExist(const QString& wallet, const QString& folder)
        { return parent->folderDoesNotExist(wallet, folder); }
    inline bool keyDoesNotExist(const QString& wallet, const QString& folder, const QString& key)
        { return parent->keyDoesNotExist(wallet, folder, key); }

    inline void closeAllWallets()
        { parent->closeAllWallets(); }

    inline QString networkWallet()
        { return parent->networkWallet(); }

    inline QString localWallet()
        { return parent->localWallet(); }

Q_SIGNALS:
    void walletListDirty();
    void walletCreated(const QString& wallet);
    void walletOpened(const QString& wallet);
    void walletDeleted(const QString& wallet);
    void walletClosed(const QString& wallet);
    void walletClosed(int handle);
    void allWalletsClosed();
    void folderListUpdated(const QString& wallet);
    void folderUpdated(const QString&, const QString&);
    void applicationDisconnected(const QString& wallet, const QString& application);
};
    
#endif

