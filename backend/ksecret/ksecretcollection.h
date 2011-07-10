/*
 * Copyright 2010, Michael Leupold <lemma@confuego.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KSECRETCOLLECTION_H
#define KSECRETCOLLECTION_H

#include "../backendcollection.h"
#include "ksecretitem.h"
#include "ksecretfile.h"

#include <QtCore/QTimer>

class KSecretCollectionManager;
class KSecretUnlockCollectionJob;
class KSecretLockCollectionJob;
class KSecretDeleteCollectionJob;
class KSecretCreateItemJob;

/**
 * Represents a part of the ksecret file stored in memory. The part's type
 * is stored as unsigned integer as the part is unknown to this version of
 * ksecretsservice.
 */
struct UnknownFilePart {
    quint32 m_type;
    QByteArray m_contents;
};

/**
 * Represents an encrypted key as stored inside the ksecret file. The key
 * type is stored as unsigned integer as the key type might not be known
 * to this version of ksecretsservice.
 */
struct EncryptedKey {
    quint32 m_type;
    QByteArray m_key;
    QByteArray m_iv;
};

/**
 * Holds a description of a ksecret file part.
 */
struct FilePartEntry {
    quint32 m_type;
    quint32 m_position;
    quint32 m_length;
};

/**
 * A collection stored on disk using the ksecret file format.
 */
class KSecretCollection : public BackendCollection
{
    Q_OBJECT

private:
    /**
     * Constructor for loading an existing collection from a file or creating
     * a new collection using create().
     *
     * @param parent the collection manager that loads this collection
     */
    KSecretCollection(BackendCollectionManager *parent);

public:
    /**
     * Create a new collection.
     *
     * @param id id for the new collection
     * @param password password to use for encrypting the collection
     * @param parent parent collection manager
     * @param errorMessage set in case of an error
     * @return the new collection or 0 in case of an error
     */
    static KSecretCollection *create(const QString &id, const QCA::SecureArray &password,
                                     BackendCollectionManager *parent, QString &errorMessage);

    /**
     * Destructor
     */
    virtual ~KSecretCollection();

    /**
     * The unique identifier for this collection
     */
    virtual QString id() const;

    /**
     * The human-readable label for this collection.
     * @todo error
     */
    virtual BackendReturn<QString> label() const;

    /**
     * Set this collection's label human-readable label.
     *
     * @todo error
     * @param label the new label for this collection
     */
    virtual BackendReturn<void> setLabel(const QString &label);

    /**
     * The time this collection was created.
     */
    virtual QDateTime created() const;

    /**
     * The time this collection was last modified.
     */
    virtual QDateTime modified() const;

    /**
     * Check whether this collection is locked.
     *
     * @return true if the collection is locked, false if the collection
     *         is unlocked.
     */
    virtual bool isLocked() const;

    /**
     * List all items inside this backend.
     *
     * @return a list containing all items inside this backend. An empty list
     *         either means that no items were found or that an error occurred
     *         (eg. collection needs unlocking before listing the items).
     * @todo error
     */
    virtual BackendReturn<QList<BackendItem*> > items() const;

    /**
     * Return all items whose attributes match the search terms.
     *
     * @param attributes attributes against which the items should be matched. 
     *      If no attributes are given, all items are returned
     * @return a list of items matching the attributes. An empty list either means that
     *         no items were found or that an error occurred (eg. collection needs
     *         unlocking before listing the items).
     * @todo error
     */
    virtual BackendReturn<QList<BackendItem*> > searchItems(
        const QMap<QString, QString> &attributes) const;

    /**
     * Create a job for unlocking this collection.
     */
    virtual UnlockCollectionJob *createUnlockJob(const CollectionUnlockInfo &unlockInfo);

    /**
     * Create a job for locking this collection.
     */
    virtual LockCollectionJob *createLockJob();

    /**
     * Create a job for deleting this collection.
     */
    virtual DeleteCollectionJob *createDeleteJob(const CollectionDeleteInfo& deleteJobInfo);

    /**
     * Create a job for creating an item.
     *
     * @param label label to assign to the new item
     * @param attributes attributes to store for the new item
     * @param secret the secret to store
     * @param replace if true, an existing item with the same attributes
     *                will be replaced, if false no item will be created
     *                if one with the same attributes already exists
     * @param locked if true, the item will be locked after creation
     */
    virtual CreateItemJob *createCreateItemJob(const ItemCreateInfo& createInfo);

    /**
     * Create a job for changing this collection's authentication.
     */
    virtual ChangeAuthenticationCollectionJob* createChangeAuthenticationJob();

    /**
     * Get the path of the ksecret file the collection is stored inside.
     *
     * @return the path of the collection file
     */
    const QString &path() const;

    /**
     * This member is called during collection creation to store the creator application path.
     * If the collection aloready has a creator set, then this methid has no effect
     * @param exePath Path to the application that created this collection
     * @return false if the collection aloready has a creator set
     */
    bool setCreatorApplication(QString exePath);
    
    /**
     * This method returns the executable path of the application which originally
     * created this collection.
     */
    const QString &creatorApplication() const {
        return m_creatorApplication;
    }

    private Q_SLOTS:
    /**
     * Remove an item from our list of known items.
     *
     * @param item Item to remove
     */
    void slotItemDeleted(BackendItem *item);

    /**
     * This slot is called whenever an Item's attributes change to rebuild the item lookup
     * hashes the collection uses to search items.
     *
     * @param item Item whose attributes changed
     */
    void changeAttributeHashes(KSecretItem *item);
    
    /**
     * This slot can be called to start the sync timer (if it's not running
     * yet). It's supposed to be called after every change to the collection
     * or one of its items.
     */
    void startSyncTimer();
    
    /**
     * This slot is called by the sync timer to sync the collection
     * on changes.
     */
    void sync();
    
public:
    /**
     * Deserialize a ksecret collection from a KSecretFile.
     *
     * @param path Path to load the collection from
     * @param parent parent collection manager
     * @param errorMessage set if there's an error
     * @return the KSecretCollection on success, 0 in case of an error
     */
    static KSecretCollection *deserialize(const QString &path, KSecretCollectionManager *parent,
                                          QString &errorMessage);

    /**
     * This methode get the application permission set by the user and stored into the 
     * backend file, or PermissionUndefined if the application is not found in the stored list
     * @param path complete path to the executable to be chedked
     * @return PermssionUndefined if the executable is not yet referenced by the collection or the
     * value stored by setApplicationPermission 
     */
    virtual ApplicationPermission applicationPermission(const QString& path) const;
    
    /**
     * Store the permission for the give application executable
     * @param path complete path to the executable
     * @param perm the permission to be stored for this executable
     * @return true if the application permission was accepted. The permission may not be accepted
     * if the executable given in the path parameter does not exist on the disk
     */
    virtual bool setApplicationPermission(const QString& path, ApplicationPermission perm);

protected:
    /**
     * Try to unlock the collection using the password provided.
     *
     * @return true if unlocking succeeded, false else
     * @remarks this is used by KSecretUnlockCollectionJob
     */
    BackendReturn<bool> tryUnlockPassword(const QCA::SecureArray &password);

    /**
     * Lock this collection. This is implemented here for convenience purposes.
     *
     * @remarks this is used by KSecretLockCollectionJob
     */
    BackendReturn<bool> lock();

    /**
     * Create a new item inside this collection.
     *
     * @remarks this is used by KSecretCreateItemJob
     */
    BackendReturn<BackendItem*> createItem(const QString &label,
                                           const QMap<QString, QString> &attributes,
                                           const QCA::SecureArray &secret, 
                                           const QString& contentType,
                                           bool replace,
                                           bool locked);

    /**
     * Delete this collection and emit the corresponding signals.
     *
     * @remarks this is used by KSecretDeleteCollectionJob
     */
    BackendReturn<bool> deleteCollection();

private:
    friend class KSecretUnlockCollectionJob;
    friend class KSecretLockCollectionJob;
    friend class KSecretDeleteCollectionJob;
    friend class KSecretCreateItemJob;
    friend class KSecretCreateCollectionJob;


    /**
     * Try to unlock using the currently set symmetric key.
     *
     * @return true if unlocking succeeded, false if it failed
     * @remarks this is used by KSecretUnlockCollectionJob
     */
    bool tryUnlock();

    /**
     * Set-up the encryption to be used by the secret collection.
     * This creates hash and cipher functors as configured.
     *
     * @param errorMessage set in case of an error
     * @return true if setting up the encryption worked, false if
     *         there were errors (ie. unsupported encryption methods.
     */
    bool setupAlgorithms(QString &errorMessage);

    /**
     * Deserialize the ksecret file header.
     *
     * @param file ksecret file to read from
     * @param errorMessage set if there's an error
     * @return true on success, false in case of an error
     */
    bool deserializeHeader(KSecretFile &file, QString &errorMessage);

    /**
     * Deserialize the algorithms used by a collection and check whether they
     * are supported.
     *
     * @param file ksecret file to read from
     * @param errorMessage set if there's an error
     * @return true on success, false in case of an error
     */
    bool deserializeAlgorithms(KSecretFile &file, QString &errorMessage);

    /**
     * Deserialize the parts inside the ksecret file.
     *
     * @param file ksecret file to read from
     * @param errorMessage set if there's an error
     * @return true on success, false in case of an error
     */
    bool deserializeParts(KSecretFile &file, QString &errorMessage);

    /**
     * Deserialize the collection property part inside a ksecret file.
     *
     * @param partContents contents of the part to deserialize
     * @return true on success, false in case of an error
     */
    bool deserializePartCollProps(const QByteArray &partContents);

    /**
     * Deserialize an item hashes part inside a ksecret file.
     *
     * @param partContents contents of the part to deserialize
     * @return true on success, false in case of an error
     */
    bool deserializePartItemHashes(const QByteArray &partContents);

    /**
     * Deserialize a symmetric key part inside a ksecret file.
     *
     * @param partContents contents of the part to deserialize
     * @return true on success, false in case of an error
     */
    bool deserializePartSymKey(const QByteArray &partContents);

    /**
     * Deserialize an acl part inside a ksecret file.
     *
     * @param partContents contents of the part to deserialize
     * @return true on success, false in case of an error
     */
    bool deserializePartAcls(const QByteArray &partContents);

    /**
     * Deserialize a config part inside a ksecret file.
     *
     * @param partContents contents of the part to deserialize
     * @return true on success, false in case of an error
     */
    bool deserializePartConfig(const QByteArray &partContents);

    /**
     * Deserialize the unlocked items part contained in file.
     *
     * @param file File to read the unlocked items from
     * @return true if reading the items was successful, false else
     * @remarks if the return value of this method is false, some of the
     *          items might already have been overwritten. So in this case
     *          it's wise to clear the items' data and re-lock them.
     */
    bool deserializeItemsUnlocked(KSecretFile &file);

    /**
     * Deserialize the contents of an encrypted part.
     *
     * @param partContents the contents of the part
     * @param decryptedPart the decrypted part contents
     * @return true if decrypting and deserializing was successful, false else
     */
    bool deserializePartEncrypted(const QByteArray &partContents, QCA::SecureArray &decryptedPart);

    /**
     * Serialize this ksecret collection back to a KSecretFile.
     *
     * @param errorMessage set if there's an replaceerror
     * @return true on success, false in case of an error
     */
    bool serialize(QString &errorMessage) const;

    /**
     * Serialize a ksecret file's headers.
     *
     * @param file ksecret file to write to
     * @return true if serialization was successful, false else
     */
    bool serializeHeader(KSecretFile &file) const;

    /**
     * Serialize a collection's parts to a ksecret file.
     *
     * @param file ksecret file to write to
     * @return true if serialization was successful, false else
     */
    bool serializeParts(KSecretFile &file) const;

    /**
     * Serialize a collection's properties to a ksecret file.
     *
     * @param file ksecret file to serialize the properties to
     * @param entry file part descriptor to put part information to
     * @return true if serialization was successful, false else
     */
    bool serializePropertiesPart(KSecretFile &file, FilePartEntry &entry) const;

    /**
     * Serialize a collection's configuration to a ksecret file.
     *
     * @param file ksecret file to serialize the configuration values to
     * @param entry file part descriptor to put part information to
     * @return true if serialization was successful, false else
     */
    bool serializeConfigPart(KSecretFile &file, FilePartEntry &entry) const;

    /**
     * Serialize a collection's acls to a ksecret file.
     *
     * @param file ksecret file to serialize the acls to
     * @param entry file part descriptor to put part information to
     * @return true if serialization was successful, false else
     */
    bool serializeAclsPart(KSecretFile &file, FilePartEntry &entry) const;

    /**
     * Serialize the item hashes of this collection to the ksecret file.
     *
     * @param file ksecret file to write to
     * @return true on success, false in case of an error
     */
    bool serializeItemHashes(KSecretFile &file, FilePartEntry &entry) const;

    /**
     * Serialize the unlocked items of this collection to the ksecret file.
     *
     * @param file ksecret file to write to
     * @return true on success, false in case of an error
     */
    bool serializeItems(KSecretFile &file, FilePartEntry &entry) const;

    /**
     * Write an encrypted part.
     *
     * @param data data to be encrypted and written
     * @param file ksecret file to write to
     * @param true on success, false in case of an error
     */
    bool serializeEncrypted(const QCA::SecureArray &data, KSecretFile &file) const;

    /**
     * Serialize a file part by writing it to the ksecret file and appending a hash mac
     * to sign its contents.
     *
     * @param data data to write to the partreplace
     * @param file ksecret file to write the data to
     * @return true if serializing was successful, false else
     */
    bool serializeAuthenticated(const QByteArray &data, KSecretFile &file) const;
    
    /**
     * Set or unset the dirty flag. When setting the flag, the syncTimer will be started
     * but only of the collection is in the unlocked state
     * @param dirty the dirty value to give
     */
    void setDirty( bool dirty = true );

    // flag which is set when the collection was changed and which
    // denotes that the collection should be synced back to disk.
    bool m_dirty;
    // timer for syncing the collection on changes
    QTimer m_syncTimer;
    
    QString m_id;
    QString m_label;
    QDateTime m_created;
    QDateTime m_modified;
    QMap<QString, QByteArray> m_propUnknownKeys; // unknown collection properties

    QString m_path; // path of the ksecret file on disk

    // verifier
    QCA::InitializationVector m_verInitVector; // initialization vector of the verifier
    QCA::SecureArray m_verEncryptedRandom; // encrypted random data of the verifier

    // configuration values
    bool m_cfgCloseScreensaver;     // close when the screensaver starts
    bool m_cfgCloseIfUnused;        // close when the last application stops using it
    quint32 m_cfgCloseUnusedTimeout; // timeout the collection will be closed after if unused (secs)
    QMap<QString, QByteArray> m_cfgUnknownKeys; // unknown configuration keys

    quint32 m_algoHash;             // hashing/mac algorithm identifier
    QCA::Hash *m_hash;              // hashing algorithm
    QCA::MessageAuthenticationCode *m_mac; // message authentication code algorithm
    quint32 m_algoCipher;           // encryption algorithm identifier
    QCA::Cipher *m_cipher;          // encryption algorithm

    QCA::SymmetricKey *m_symmetricKey; // the symmetric key used for encryption/decryption

    // the configuration values message authentication code
    QByteArray m_configValuesMac;
    // the properties message authentication code
    QByteArray m_propertiesMac;

    // the acls message authentication code
    QHash<QString, ApplicationPermission> m_acls;
    QMap<QString, int> m_unknownAcls;
    QByteArray m_aclsMac;
    QString m_creatorApplication;
    
    // unknown file parts stored as-is
    QList<UnknownFilePart*> m_unknownParts;

    // contains the encrypted item parts
    QList<QByteArray> m_encryptedItemParts;

    // contains the encrypted symmetric keys
    QList<EncryptedKey*> m_encryptedSymKeys;

    // maps lookup attribute hashes to items
    QMultiHash<QByteArray, KSecretItem*> m_itemHashes;
    // maps item ids to their hashes for changing/removal
    QHash<KSecretItem*, QSet<QByteArray> > m_reverseItemHashes;

    // maps item identifiers to items
    QHash<QString, KSecretItem*> m_items;
};

#endif
