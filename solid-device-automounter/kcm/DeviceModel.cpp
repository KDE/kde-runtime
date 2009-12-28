#include "DeviceModel.h"

#include <KLocalizedString>
#include <Solid/DeviceNotifier>
#include <Solid/Device>
#include <Solid/StorageVolume>
#include <KIcon>

#include "AutomounterSettings.h"

#include <KDebug>

DeviceModel::DeviceModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    foreach(const QString &dev, AutomounterSettings::knownDevices()) {
        addNewDevice(dev);
    }
    
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString)), this, SLOT(deviceAttached(const QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString)), this, SLOT(deviceRemoved(const QString)));
}

DeviceModel::~DeviceModel()
{
    
}

void
DeviceModel::forgetDevice(const QString &udi)
{
    if (m_disconnected.contains(udi)) {
        beginRemoveRows(index(0, 0), m_disconnected.indexOf(udi), m_disconnected.indexOf(udi));
        m_disconnected.removeOne(udi);
        endRemoveRows();
    } else if (m_attached.contains(udi)) {
        beginRemoveRows(index(1, 0), m_attached.indexOf(udi), m_attached.indexOf(udi));
        m_attached.removeOne(udi);
        endRemoveRows();
    }
    m_loginForced.remove(udi);
    m_attachedForced.remove(udi);
}

QVariant
DeviceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
            case 0:
                return i18n("Device");
            case 1:
                return i18n("Automount on login");
            case 2:
                return i18n("Automount on attach");
        }
    }
    return QVariant();
}

void
DeviceModel::deviceAttached(const QString &udi)
{
    Solid::Device dev(udi);
    if (dev.is<Solid::StorageVolume>()) {
        if (m_disconnected.contains(udi)) {
            emit layoutAboutToBeChanged();
            beginRemoveRows(index(0, 0), m_disconnected.indexOf(udi), m_disconnected.indexOf(udi));
            m_disconnected.removeOne(udi);
            endRemoveRows();
            emit layoutChanged();
        }
        addNewDevice(udi);
    }
}

void
DeviceModel::deviceRemoved(const QString &udi)
{
    if (m_attached.contains(udi)) {
        emit layoutAboutToBeChanged();
        beginRemoveRows(index(1, 0), m_attached.indexOf(udi), m_attached.indexOf(udi));
        m_attached.removeOne(udi);
        endRemoveRows();
        emit layoutChanged();
        addNewDevice(udi);
    }
}

void
DeviceModel::addNewDevice(const QString &udi)
{
    AutomounterSettings::self()->readConfig();
    if (!m_loginForced.contains(udi))
        m_loginForced[udi] = AutomounterSettings::deviceAutomountIsForced(udi, AutomounterSettings::Login);
    if (!m_attachedForced.contains(udi))
        m_loginForced[udi] = AutomounterSettings::deviceAutomountIsForced(udi, AutomounterSettings::Attach);
    emit layoutAboutToBeChanged();
    Solid::Device dev(udi);
    if (dev.isValid()) {
        beginInsertRows(index(0, 0), m_attached.size(), m_attached.size()+1);
        m_attached << udi;
        kDebug() << "Adding attached device" << udi;
    } else {
        beginInsertRows(index(1, 0), m_disconnected.size(), m_disconnected.size()+1);
        m_disconnected << udi;
        kDebug() << "Adding disconnected device" << udi;
    }
    endInsertRows();
    emit layoutChanged();
}

void
DeviceModel::reload()
{
    foreach(const QString &udi, m_loginForced.keys()) {
        m_loginForced[udi] = AutomounterSettings::deviceAutomountIsForced(udi, AutomounterSettings::Login);
        m_attachedForced[udi] = AutomounterSettings::deviceAutomountIsForced(udi, AutomounterSettings::Attach);
    }
}

QModelIndex
DeviceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.row() == 0) {
            if (row >= 0 && row < m_attached.size() && column >= 0 && column <= 2)
                return createIndex(row, column, 0);
        } else if (parent.row() == 1) {
            if (row >= 0 && row < m_disconnected.size() && column >= 0 && column <= 2)
                return createIndex(row, column, 1);
        }
    } else {
         if ((row == 0 || row == 1) && column >= 0 && column <= 2)
            return createIndex(row, column, 3);
    }
    return QModelIndex();
}

QModelIndex
DeviceModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        ///TODO: Use internalId with constants instead of parent().row() everywhere.
        if (index.internalId() == 3)
            return QModelIndex();
        return createIndex(index.internalId(), 0, 3);
    }
    return QModelIndex();
}

Qt::ItemFlags
DeviceModel::flags(const QModelIndex &index) const
{
    if (index.isValid()) {
        if (index.parent().isValid()) {
            if (index.column() > 0) {
                return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
            } else if (index.column() == 0) {
                return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
            }
        } else {
            return Qt::ItemIsEnabled;
        }
    }
    return Qt::NoItemFlags;
}

bool
DeviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole && index.column() > 0) {
        QString udi = index.data(Qt::UserRole).toString();
        switch(index.column()) {
            case 1:
                m_loginForced[udi] = (value.toInt() == Qt::Checked) ? true : false;
                break;
            case 2:
                m_attachedForced[udi] = (value.toInt() == Qt::Checked) ? true : false;
                break;
        }
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QVariant
DeviceModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.parent().isValid()) {
        if (index.parent().row() == 0) {
            QString udi = m_attached[index.row()];
            Solid::Device dev(udi);
            if (role == Qt::UserRole)
                return udi;
            if (index.column() == 0) {
                switch(role) {
                    case Qt::DisplayRole:
                        return dev.description();
                    case Qt::ToolTipRole:
                        return i18n("UDI: %1", udi);
                    case Qt::DecorationRole:
                        return KIcon(dev.icon());
                }
            } else if (index.column() == 1) {
                switch(role) {
                    case Qt::CheckStateRole:
                        return m_loginForced[udi] ? Qt::Checked : Qt::Unchecked;
                    case Qt::ToolTipRole:
                        if (m_loginForced[udi] || AutomounterSettings::shouldAutomountDevice(udi, AutomounterSettings::Login))
                            return i18n("This device will be automatically mounted at login.");
                        return i18n("This device will not be automatically mounted at login.");
                }
            } else if (index.column() == 2) {
                switch(role) {
                    case Qt::CheckStateRole:
                        return m_attachedForced[udi] ? Qt::Checked : Qt::Unchecked;
                    case Qt::ToolTipRole:
                        if (m_attachedForced[udi] || AutomounterSettings::shouldAutomountDevice(udi, AutomounterSettings::Attach))
                            return i18n("This device will be automatically mounted when attached.");
                        return i18n("This device will not be automatically mounted when attached.");
                }
            }
        } else if (index.parent().row() == 1) {
            QString udi = m_disconnected[index.row()];
            if (role == Qt::UserRole)
                return udi;
            if (index.column() == 0) {
                switch(role) {
                    case Qt::DisplayRole:
                        return AutomounterSettings::getDeviceName(udi);
                    case Qt::ToolTipRole:
                        return i18n("UDI: %1", udi);
                    case Qt::DecorationRole:
                        return KIcon(AutomounterSettings::getDeviceIcon(udi));
                }
            } else if (index.column() == 1) {
                switch(role) {
                    case Qt::CheckStateRole:
                        return m_loginForced[udi] ? Qt::Checked : Qt::Unchecked;
                    case Qt::ToolTipRole:
                        if (m_loginForced[udi] || AutomounterSettings::shouldAutomountDevice(udi, AutomounterSettings::Login))
                            return i18n("This device will be automatically mounted at login.");
                        return i18n("This device will not be automatically mounted at login.");
                }
            } else if (index.column() == 2) {
                switch(role) {
                    case Qt::CheckStateRole:
                        return m_attachedForced[udi] ? Qt::Checked : Qt::Unchecked;
                    case Qt::ToolTipRole:
                        if (m_attachedForced[udi] || AutomounterSettings::shouldAutomountDevice(udi, AutomounterSettings::Attach))
                            return i18n("This device will be automatically mounted when attached.");
                        return i18n("This device will not be automatically mounted when attached.");
                }
            }
        }
    } else if (index.isValid()) {
        if (role == Qt::DisplayRole && index.column() == 0) {
            if (index.row() == 0)
                return i18n("Attached Devices");
            else if (index.row() == 1)
                return i18n("Disconnected Devices");
        }
    }
    return QVariant();
}

int
DeviceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.parent().isValid())
            return 0;
        if (parent.row() == 0)
            return m_attached.size();
        return m_disconnected.size();
    } else {
        return 2;
    }
}

int
DeviceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}
