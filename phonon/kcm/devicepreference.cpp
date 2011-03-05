/*  This file is part of the KDE project
    Copyright (C) 2006-2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) version 3.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "devicepreference.h"

#include <QtCore/QList>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QItemDelegate>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHeaderView>

#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <phonon/backendinterface.h>
#include <phonon/backendcapabilities.h>
#include <phonon/globalconfig.h>
#include <phonon/objectdescription.h>
#include <phonon/phononnamespace.h>
#include "factory_p.h"
#include <kfadewidgeteffect.h>

#include <kdialog.h>
#include <klistwidget.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#ifndef METATYPE_QLIST_INT_DEFINED
#define METATYPE_QLIST_INT_DEFINED
// Want this exactly once, see phonondefs_p.h kcm/devicepreference.cpp
Q_DECLARE_METATYPE(QList<int>)
#endif

static const Phonon::Category captureCategories[] = {
    Phonon::NoCategory,
    Phonon::CommunicationCategory,
    Phonon::AccessibilityCategory
};
static const int captureCategoriesCount = sizeof(captureCategories)/sizeof(Phonon::Category);

void operator++(Phonon::Category &c)
{
    c = static_cast<Phonon::Category>(1 + static_cast<int>(c));
    //Q_ASSERT(c <= Phonon::LastCategory);
}

class CategoryItem : public QStandardItem {
    public:
        CategoryItem(Phonon::Category cat, Phonon::ObjectDescriptionType t = Phonon::AudioOutputDeviceType)
            : QStandardItem(),
            m_cat(cat),
            m_odtype(t)
        {
            if (cat == Phonon::NoCategory) {
                switch(t) {
                case Phonon::AudioOutputDeviceType:
                    setText(i18n("Audio Output"));
                    break;
                case Phonon::AudioCaptureDeviceType:
                    setText(i18n("Audio Capture"));
                    break;
                case Phonon::VideoCaptureDeviceType:
                    setText(i18n("Video Capture"));
                    break;
                default:
                    setText(i18n("Invalid"));
                }
            } else {
                setText(Phonon::categoryToString(cat));
            }
        }

        int type() const { return 1001; }
        Phonon::Category category() const { return m_cat; }
        Phonon::ObjectDescriptionType odtype() const { return m_odtype; }

    private:
        Phonon::Category m_cat;
        Phonon::ObjectDescriptionType m_odtype;
};

/**
 * Need this to change the colors of the ListView if the Palette changed. With CSS set this won't
 * change automatically
 */
void DevicePreference::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::PaletteChange) {
        //deviceList->viewport()->setStyleSheet(deviceList->viewport()->styleSheet());
        deviceList->setStyleSheet(deviceList->styleSheet());
    }
}

DevicePreference::DevicePreference(QWidget *parent)
    : QWidget(parent),
    m_headerModel(0, 1, 0),
    m_showingOutputModel(true),
    m_media(0), m_output(0)
{
    setupUi(this);
    testPlaybackButton->setIcon(KIcon("media-playback-start"));
    testPlaybackButton->setEnabled(false);
    testPlaybackButton->setToolTip(i18n("Play a test sound on the selected device"));
    removeButton->setIcon(KIcon("list-remove"));
    deferButton->setIcon(KIcon("go-down"));
    preferButton->setIcon(KIcon("go-up"));
    deviceList->setDragDropMode(QAbstractItemView::InternalMove);
    //deviceList->viewport()->setStyleSheet(QString("QWidget#qt_scrollarea_viewport {"
    deviceList->setStyleSheet(QString("QTreeView {"
                "background-color: palette(base);"
                "background-image: url(%1);"
                "background-position: bottom left;"
                "background-attachment: fixed;"
                "background-repeat: no-repeat;"
                "}")
            .arg(KStandardDirs::locate("data", "kcm_phonon/listview-background.png")));
    deviceList->setAlternatingRowColors(false);
    QStandardItem *parentItem = m_categoryModel.invisibleRootItem();

    // Audio Output Parent
    QStandardItem *aOutputItem = new CategoryItem(Phonon::NoCategory, Phonon::AudioOutputDeviceType);
    m_audioOutputModel[Phonon::NoCategory] = new Phonon::AudioOutputDeviceModel(this);
    aOutputItem->setEditable(false);
    aOutputItem->setToolTip(i18n("Defines the default ordering of devices which can be overridden by individual categories."));
    parentItem->appendRow(aOutputItem);

    // Audio Capture Parent
    QStandardItem *aCaptureItem = new CategoryItem(Phonon::NoCategory, Phonon::AudioCaptureDeviceType);
    m_audioCaptureModel[Phonon::NoCategory] = new Phonon::AudioCaptureDeviceModel(this);
    aCaptureItem->setEditable(false);
    aCaptureItem->setToolTip(i18n("Defines the default ordering of devices which can be overridden by individual categories."));
    parentItem->appendRow(aCaptureItem);

    // Video Capture Parent
    QStandardItem *vCaptureItem = new CategoryItem(Phonon::NoCategory, Phonon::VideoCaptureDeviceType);
    m_videoCaptureModel[Phonon::NoCategory] = new Phonon::VideoCaptureDeviceModel(this);
    vCaptureItem->setEditable(false);
    vCaptureItem->setToolTip(i18n("Defines the default ordering of devices which can be overridden by individual categories."));
    parentItem->appendRow(vCaptureItem);

    // Audio Output Children
    parentItem = aOutputItem;
    for (int i = 0; i <= Phonon::LastCategory; ++i) {
        m_audioOutputModel[i] = new Phonon::AudioOutputDeviceModel(this);
        QStandardItem *item = new CategoryItem(static_cast<Phonon::Category>(i));
        item->setEditable(false);
        parentItem->appendRow(item);
    }

    // Audio Capture Children
    parentItem = aCaptureItem;
    for (int i = 1; i < captureCategoriesCount; ++i) { // i == 1 to skip NoCategory
        m_audioCaptureModel[captureCategories[i]] = new Phonon::AudioCaptureDeviceModel(this);
        QStandardItem *item = new CategoryItem(captureCategories[i], Phonon::AudioCaptureDeviceType);
        item->setEditable(false);
        parentItem->appendRow(item);
    }

    // Video Capture Children
    parentItem = vCaptureItem;
    for (int i = 1; i < captureCategoriesCount; ++i) { // i == 1 to skip NoCategory
        m_videoCaptureModel[captureCategories[i]] = new Phonon::VideoCaptureDeviceModel(this);
        QStandardItem *item = new CategoryItem(captureCategories[i], Phonon::VideoCaptureDeviceType);
        item->setEditable(false);
        parentItem->appendRow(item);
    }

    categoryTree->setModel(&m_categoryModel);
    if (categoryTree->header()) {
        categoryTree->header()->hide();
    }
    categoryTree->expandAll();

    connect(categoryTree->selectionModel(),
            SIGNAL(currentChanged(const QModelIndex &,const QModelIndex &)),
            SLOT(updateDeviceList()));

    for (int i = -1; i <= Phonon::LastCategory; ++i) {
        connect(m_audioOutputModel[i], SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(changed()));
        connect(m_audioOutputModel[i], SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(changed()));
        connect(m_audioOutputModel[i], SIGNAL(layoutChanged()), this, SIGNAL(changed()));
        connect(m_audioOutputModel[i], SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SIGNAL(changed()));
        if (m_audioCaptureModel.contains(i)) {
            connect(m_audioCaptureModel[i], SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(changed()));
            connect(m_audioCaptureModel[i], SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(changed()));
            connect(m_audioCaptureModel[i], SIGNAL(layoutChanged()), this, SIGNAL(changed()));
            connect(m_audioCaptureModel[i], SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SIGNAL(changed()));
        }
        if (m_videoCaptureModel.contains(i)) {
            connect(m_videoCaptureModel[i], SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(changed()));
            connect(m_videoCaptureModel[i], SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(changed()));
            connect(m_videoCaptureModel[i], SIGNAL(layoutChanged()), this, SIGNAL(changed()));
            connect(m_videoCaptureModel[i], SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SIGNAL(changed()));
        }
    }
    connect(showCheckBox, SIGNAL(stateChanged (int)), this, SIGNAL(changed()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(availableAudioOutputDevicesChanged()), SLOT(updateAudioOutputDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(availableAudioCaptureDevicesChanged()), SLOT(updateAudioCaptureDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(availableVideoCaptureDevicesChanged()), SLOT(updateVideoCaptureDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(capabilitiesChanged()), SLOT(updateAudioOutputDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(capabilitiesChanged()), SLOT(updateAudioCaptureDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(capabilitiesChanged()), SLOT(updateVideoCaptureDevices()));

    if (!categoryTree->currentIndex().isValid()) {
        categoryTree->setCurrentIndex(m_categoryModel.index(0, 0).child(1, 0));
    }
}

void DevicePreference::updateDeviceList()
{
    QStandardItem *currentItem = m_categoryModel.itemFromIndex(categoryTree->currentIndex());
    KFadeWidgetEffect *animation = new KFadeWidgetEffect(deviceList);
    if (deviceList->selectionModel()) {
        disconnect(deviceList->selectionModel(),
                SIGNAL(currentRowChanged(const QModelIndex &,const QModelIndex &)),
                this, SLOT(updateButtonsEnabled()));
    }
    if (currentItem && currentItem->type() == 1001) {
        CategoryItem *catItem = static_cast<CategoryItem *>(currentItem);
        const Phonon::Category cat = catItem->category();
        switch (catItem->odtype()) {
        case Phonon::AudioOutputDeviceType:
            deviceList->setModel(m_audioOutputModel[cat]);
            break;
        case Phonon::AudioCaptureDeviceType:
            deviceList->setModel(m_audioCaptureModel[cat]);
            break;
        case Phonon::VideoCaptureDeviceType:
            deviceList->setModel(m_videoCaptureModel[cat]);
            break;
        default: ;
        }
        m_showingOutputModel = catItem->odtype() == Phonon::AudioOutputDeviceType;
        if (cat == Phonon::NoCategory) {
            switch (catItem->odtype()) {
            case Phonon::AudioOutputDeviceType:
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Default Audio Output Device Preference"), Qt::DisplayRole);
                break;
            case Phonon::AudioCaptureDeviceType:
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Default Audio Capture Device Preference"), Qt::DisplayRole);
                break;
            case Phonon::VideoCaptureDeviceType:
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Default Video Capture Device Preference"), Qt::DisplayRole);
                break;
            default: ;
            }
        } else {
            switch (catItem->odtype()) {
            case Phonon::AudioOutputDeviceType:
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Audio Output Device Preference for the '%1' Category",
                        Phonon::categoryToString(cat)), Qt::DisplayRole);
                break;
            case Phonon::AudioCaptureDeviceType:
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Audio Capture Device Preference for the '%1' Category",
                        Phonon::categoryToString(cat)), Qt::DisplayRole);
                break;
            case Phonon::VideoCaptureDeviceType:
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Video Capture Device Preference for the '%1' Category ",
                        Phonon::categoryToString(cat)), Qt::DisplayRole);
                break;
            default: ;
            }
        }
    } else {
        m_showingOutputModel = false;
        m_headerModel.setHeaderData(0, Qt::Horizontal, QString(), Qt::DisplayRole);
        deviceList->setModel(0);
    }
    deviceList->header()->setModel(&m_headerModel);
    updateButtonsEnabled();
    if (deviceList->selectionModel()) {
        connect(deviceList->selectionModel(),
                SIGNAL(currentRowChanged(const QModelIndex &,const QModelIndex &)),
                this, SLOT(updateButtonsEnabled()));
    }
    deviceList->resizeColumnToContents(0);
    animation->start();
}

void DevicePreference::updateAudioCaptureDevices()
{
    kDebug();
    const QList<Phonon::AudioCaptureDevice> list = availableAudioCaptureDevices();
    QHash<int, Phonon::AudioCaptureDevice> hash;
    foreach (const Phonon::AudioCaptureDevice &dev, list) {
        hash.insert(dev.index(), dev);
    }
    for (int ii = 0; ii < captureCategoriesCount; ++ii) {
        const int i = captureCategories[ii];
        Phonon::AudioCaptureDeviceModel *model = m_audioCaptureModel.value(i);
        Q_ASSERT(model);

        QHash<int, Phonon::AudioCaptureDevice> hashCopy(hash);
        QList<Phonon::AudioCaptureDevice> orderedList;
        if (model->rowCount() > 0) {
            QList<int> order = model->tupleIndexOrder();
            foreach (int idx, order) {
                if (hashCopy.contains(idx)) {
                    orderedList << hashCopy.take(idx);
                }
            }
            if (hashCopy.size() > 1) {
                // keep the order of the original list
                foreach (const Phonon::AudioCaptureDevice &dev, list) {
                    if (hashCopy.contains(dev.index())) {
                        orderedList << hashCopy.take(dev.index());
                    }
                }
            } else if (hashCopy.size() == 1) {
                orderedList += hashCopy.values();
            }
            model->setModelData(orderedList);
        } else {
            model->setModelData(list);
        }
    }
    deviceList->resizeColumnToContents(0);
}

void DevicePreference::updateVideoCaptureDevices()
{
    kDebug();
    const QList<Phonon::VideoCaptureDevice> list = availableVideoCaptureDevices();
    QHash<int, Phonon::VideoCaptureDevice> hash;
    foreach (const Phonon::VideoCaptureDevice &dev, list) {
        hash.insert(dev.index(), dev);
    }
    for (int ii = 0; ii < captureCategoriesCount; ++ii) {
        const int i = captureCategories[ii];
        Phonon::VideoCaptureDeviceModel *model = m_videoCaptureModel.value(i);
        Q_ASSERT(model);

        QHash<int, Phonon::VideoCaptureDevice> hashCopy(hash);
        QList<Phonon::VideoCaptureDevice> orderedList;
        if (model->rowCount() > 0) {
            QList<int> order = model->tupleIndexOrder();
            foreach (int idx, order) {
                if (hashCopy.contains(idx)) {
                    orderedList << hashCopy.take(idx);
                }
            }
            if (hashCopy.size() > 1) {
                // keep the order of the original list
                foreach (const Phonon::VideoCaptureDevice &dev, list) {
                    if (hashCopy.contains(dev.index())) {
                        orderedList << hashCopy.take(dev.index());
                    }
                }
            } else if (hashCopy.size() == 1) {
                orderedList += hashCopy.values();
            }
            model->setModelData(orderedList);
        } else {
            model->setModelData(list);
        }
    }
    deviceList->resizeColumnToContents(0);
}

void DevicePreference::updateAudioOutputDevices()
{
    const QList<Phonon::AudioOutputDevice> list = availableAudioOutputDevices();
    QHash<int, Phonon::AudioOutputDevice> hash;
    foreach (const Phonon::AudioOutputDevice &dev, list) {
        hash.insert(dev.index(), dev);
    }
    for (int i = -1; i <= Phonon::LastCategory; ++i) {
        Phonon::AudioOutputDeviceModel *model = m_audioOutputModel.value(i);
        Q_ASSERT(model);

        QHash<int, Phonon::AudioOutputDevice> hashCopy(hash);
        QList<Phonon::AudioOutputDevice> orderedList;
        if (model->rowCount() > 0) {
            QList<int> order = model->tupleIndexOrder();
            foreach (int idx, order) {
                if (hashCopy.contains(idx)) {
                    orderedList << hashCopy.take(idx);
                }
            }
            if (hashCopy.size() > 1) {
                // keep the order of the original list
                foreach (const Phonon::AudioOutputDevice &dev, list) {
                    if (hashCopy.contains(dev.index())) {
                        orderedList << hashCopy.take(dev.index());
                    }
                }
            } else if (hashCopy.size() == 1) {
                orderedList += hashCopy.values();
            }
            model->setModelData(orderedList);
        } else {
            model->setModelData(list);
        }
    }
    deviceList->resizeColumnToContents(0);
}

QList<Phonon::AudioOutputDevice> DevicePreference::availableAudioOutputDevices() const
{
    return Phonon::BackendCapabilities::availableAudioOutputDevices();
}

QList<Phonon::AudioCaptureDevice> DevicePreference::availableAudioCaptureDevices() const
{
    return Phonon::BackendCapabilities::availableAudioCaptureDevices();
}

QList<Phonon::VideoCaptureDevice> DevicePreference::availableVideoCaptureDevices() const
{
    return Phonon::BackendCapabilities::availableVideoCaptureDevices();
}

void DevicePreference::load()
{
    showCheckBox->setChecked(!Phonon::GlobalConfig().hideAdvancedDevices());
    loadCategoryDevices();
}

void DevicePreference::loadCategoryDevices()
{
    // "Load" the settings from the backend.
    for (Phonon::Category cat = Phonon::NoCategory; cat <= Phonon::LastCategory; ++cat) {
        QList<Phonon::AudioOutputDevice> list;
        const QList<int> deviceIndexes = Phonon::GlobalConfig().audioOutputDeviceListFor(cat);
        foreach (int i, deviceIndexes) {
            list.append(Phonon::AudioOutputDevice::fromIndex(i));
        }
        m_audioOutputModel[cat]->setModelData(list);
    }
    for (int i = 0; i < captureCategoriesCount; ++i) {
        const Phonon::Category cat = captureCategories[i];
        QList<Phonon::AudioCaptureDevice> list;
        const QList<int> deviceIndexes = Phonon::GlobalConfig().audioCaptureDeviceListFor(cat);
        foreach (int i, deviceIndexes) {
            list.append(Phonon::AudioCaptureDevice::fromIndex(i));
        }
        m_audioCaptureModel[cat]->setModelData(list);
    }
    for (int i = 0; i < captureCategoriesCount; ++i) {
        const Phonon::Category cat = captureCategories[i];
        QList<Phonon::VideoCaptureDevice> list;
        const QList<int> deviceIndexes = Phonon::GlobalConfig().videoCaptureDeviceListFor(cat);
        foreach (int i, deviceIndexes) {
            list.append(Phonon::VideoCaptureDevice::fromIndex(i));
        }
        m_videoCaptureModel[cat]->setModelData(list);
    }

    deviceList->resizeColumnToContents(0);
}

void DevicePreference::save()
{
    if (!m_removeOnApply.isEmpty()) {
        QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kded", "/modules/phononserver",
                "org.kde.PhononServer", "removeAudioDevices");
        msg << QVariant::fromValue(m_removeOnApply);
        QDBusConnection::sessionBus().send(msg);
        m_removeOnApply.clear();
    }

    for (Phonon::Category cat = Phonon::NoCategory; cat <= Phonon::LastCategory; ++cat) {
        Q_ASSERT(m_audioOutputModel.value(cat));
        const QList<int> order = m_audioOutputModel.value(cat)->tupleIndexOrder();
        Phonon::GlobalConfig().setAudioOutputDeviceListFor(cat, order);
    }

    for (int i = 1; i < captureCategoriesCount; ++i) {
        const Phonon::Category cat = captureCategories[i];
        Q_ASSERT(m_audioCaptureModel.value(cat));
        const QList<int> order = m_audioCaptureModel.value(cat)->tupleIndexOrder();
        Phonon::GlobalConfig().setAudioCaptureDeviceListFor(cat, order);
    }

    for (int i = 1; i < captureCategoriesCount; ++i) {
        const Phonon::Category cat = captureCategories[i];
        Q_ASSERT(m_videoCaptureModel.value(cat));
        const QList<int> order = m_videoCaptureModel.value(cat)->tupleIndexOrder();
        Phonon::GlobalConfig().setVideoCaptureDeviceListFor(cat, order);
    }
}

void DevicePreference::defaults()
{
    {
        const QList<Phonon::AudioOutputDevice> list = availableAudioOutputDevices();
        for (int i = -1; i <= Phonon::LastCategory; ++i) {
            m_audioOutputModel[i]->setModelData(list);
        }
    }
    {
        const QList<Phonon::AudioCaptureDevice> list = availableAudioCaptureDevices();
        for (int i = 0; i < captureCategoriesCount; ++i) {
            m_audioCaptureModel[captureCategories[i]]->setModelData(list);
        }
    }
    {
        const QList<Phonon::VideoCaptureDevice> list = availableVideoCaptureDevices();
        for (int i = 0; i < captureCategoriesCount; ++i) {
            m_videoCaptureModel[captureCategories[i]]->setModelData(list);
        }
    }

    deviceList->resizeColumnToContents(0);
}

void DevicePreference::on_preferButton_clicked()
{
    QAbstractItemModel *model = deviceList->model();
    {
        Phonon::AudioOutputDeviceModel *deviceModel = qobject_cast<Phonon::AudioOutputDeviceModel *>(model);
        if (deviceModel) {
            deviceModel->moveUp(deviceList->currentIndex());
            updateButtonsEnabled();
            emit changed();
        }
    }
    {
        Phonon::AudioCaptureDeviceModel *deviceModel = qobject_cast<Phonon::AudioCaptureDeviceModel *>(model);
        if (deviceModel) {
            deviceModel->moveUp(deviceList->currentIndex());
            updateButtonsEnabled();
            emit changed();
        }
    }
    {
        Phonon::VideoCaptureDeviceModel *deviceModel = qobject_cast<Phonon::VideoCaptureDeviceModel *>(model);
        if (deviceModel) {
            deviceModel->moveUp(deviceList->currentIndex());
            updateButtonsEnabled();
            emit changed();
        }
    }
}

void DevicePreference::on_deferButton_clicked()
{
    QAbstractItemModel *model = deviceList->model();
    {
        Phonon::AudioOutputDeviceModel *deviceModel = qobject_cast<Phonon::AudioOutputDeviceModel *>(model);
        if (deviceModel) {
            deviceModel->moveDown(deviceList->currentIndex());
            updateButtonsEnabled();
            emit changed();
        }
    }
    {
        Phonon::AudioCaptureDeviceModel *deviceModel = qobject_cast<Phonon::AudioCaptureDeviceModel *>(model);
        if (deviceModel) {
            deviceModel->moveDown(deviceList->currentIndex());
            updateButtonsEnabled();
            emit changed();
        }
    }
    {
        Phonon::VideoCaptureDeviceModel *deviceModel = qobject_cast<Phonon::VideoCaptureDeviceModel *>(model);
        if (deviceModel) {
            deviceModel->moveDown(deviceList->currentIndex());
            updateButtonsEnabled();
            emit changed();
        }
    }
}

template<Phonon::ObjectDescriptionType T>
void DevicePreference::removeDevice(const Phonon::ObjectDescription<T> &deviceToRemove,
        QMap<int, Phonon::ObjectDescriptionModel<T> *> *modelMap)
{
    QDBusInterface phononServer(QLatin1String("org.kde.kded"), QLatin1String("/modules/phononserver"),
            QLatin1String("org.kde.PhononServer"));
    QDBusReply<bool> reply = phononServer.call(QLatin1String("isAudioDeviceRemovable"), deviceToRemove.index());
    if (!reply.isValid()) {
        kError(600) << reply.error();
        return;
    }
    if (!reply.value()) {
        return;
    }
    m_removeOnApply << deviceToRemove.index();

    // remove from all models, idx.row() is only correct for the current model
    foreach (Phonon::ObjectDescriptionModel<T> *model, *modelMap) {
        QList<Phonon::ObjectDescription<T> > data = model->modelData();
        for (int row = 0; row < data.size(); ++row) {
            if (data[row] == deviceToRemove) {
                model->removeRows(row, 1);
                break;
            }
        }
    }
    updateButtonsEnabled();
    emit changed();
}

void DevicePreference::on_removeButton_clicked()
{
    const QModelIndex idx = deviceList->currentIndex();

    if (idx.isValid()) {
        QAbstractItemModel *model = deviceList->model();
        Phonon::AudioOutputDeviceModel *aPlaybackModel = qobject_cast<Phonon::AudioOutputDeviceModel *>(model);
        Phonon::AudioCaptureDeviceModel *aCaptureModel = qobject_cast<Phonon::AudioCaptureDeviceModel *>(model);
        Phonon::VideoCaptureDeviceModel *vCaptureModel = qobject_cast<Phonon::VideoCaptureDeviceModel *>(model);

        if (aPlaybackModel) {
            removeDevice(aPlaybackModel->modelData(idx), &m_audioOutputModel);
        }

        if (aCaptureModel) {
            removeDevice(aCaptureModel->modelData(idx), &m_audioCaptureModel);
        }

        if (vCaptureModel) {
            removeDevice(vCaptureModel->modelData(idx), &m_videoCaptureModel);
        }
    }

    deviceList->resizeColumnToContents(0);
}

void DevicePreference::on_applyPreferencesButton_clicked()
{
    const QModelIndex idx = categoryTree->currentIndex();
    const QStandardItem *item = m_categoryModel.itemFromIndex(idx);
    if( !item ) 
        return;
    Q_ASSERT(item->type() == 1001);
    const CategoryItem *catItem = static_cast<const CategoryItem *>(item);
    const QList<Phonon::AudioOutputDevice> modelData = m_audioOutputModel.value(catItem->category())->modelData();
    // TODO Capture !!

    KDialog dialog(this);
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);

    QWidget mainWidget(&dialog);
    dialog.setMainWidget(&mainWidget);

    QLabel label(&mainWidget);
    label.setText(i18n("Apply the currently shown device preference list to the following other "
                "audio output categories:"));
    label.setWordWrap(true);

    KListWidget list(&mainWidget);
    for (Phonon::Category cat = Phonon::NoCategory; cat <= Phonon::LastCategory; ++cat) {
        QListWidgetItem *item = new QListWidgetItem(cat == Phonon::NoCategory
                ? i18n("Default/Unspecified Category") : Phonon::categoryToString(cat), &list, cat);
        item->setCheckState(Qt::Checked);
        if (cat == catItem->category()) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }

    QVBoxLayout layout(&mainWidget);
    layout.setMargin(0);
    layout.addWidget(&label);
    layout.addWidget(&list);

    switch (dialog.exec()) {
    case QDialog::Accepted:
        for (Phonon::Category cat = Phonon::NoCategory; cat <= Phonon::LastCategory; ++cat) {
            if (cat != catItem->category()) {
                QListWidgetItem *item = list.item(static_cast<int>(cat) + 1);
                Q_ASSERT(item->type() == cat);
                if (item->checkState() == Qt::Checked) {
                    m_audioOutputModel.value(cat)->setModelData(modelData);
                }
            }
        }
        emit changed();
        break;
    case QDialog::Rejected:
        // nothing to do
        break;
    }
}

void DevicePreference::on_showCheckBox_toggled()
{
    // In order to get the right list from the backend, we need to update the settings now
    // before calling availableAudio{Output,Capture}Devices()
    Phonon::GlobalConfig().setHideAdvancedDevices(!showCheckBox->isChecked());
    loadCategoryDevices();
}

void DevicePreference::on_testPlaybackButton_toggled(bool down)
{
    if (down) {
        QModelIndex idx = deviceList->currentIndex();
        if (!idx.isValid() || !m_showingOutputModel) {
            return;
        }
        const Phonon::AudioOutputDeviceModel *model = static_cast<const Phonon::AudioOutputDeviceModel *>(idx.model());
        const Phonon::AudioOutputDevice &device = model->modelData(idx);
        m_media = new Phonon::MediaObject(this);
        m_output = new Phonon::AudioOutput(this);
        m_output->setOutputDevice(device);

        // just to be very sure that nothing messes our test sound up
        m_output->setVolume(1.0);
        m_output->setMuted(false);

        Phonon::createPath(m_media, m_output);
        connect(m_media, SIGNAL(finished()), testPlaybackButton, SLOT(toggle()));
        m_media->setCurrentSource(KStandardDirs::locate("sound", "KDE-Sys-Log-In.ogg"));
        m_media->play();
    } else {
        disconnect(m_media, SIGNAL(finished()), testPlaybackButton, SLOT(toggle()));
        delete m_media;
        m_media = 0;
        delete m_output;
        m_output = 0;
    }
}

void DevicePreference::updateButtonsEnabled()
{
    //kDebug() ;
    if (deviceList->model()) {
        //kDebug() << "model available";
        QModelIndex idx = deviceList->currentIndex();
        preferButton->setEnabled(idx.isValid() && idx.row() > 0);
        deferButton->setEnabled(idx.isValid() && idx.row() < deviceList->model()->rowCount() - 1);
        removeButton->setEnabled(idx.isValid() && !(idx.flags() & Qt::ItemIsEnabled));
        testPlaybackButton->setEnabled(m_showingOutputModel && idx.isValid() &&
                (idx.flags() & Qt::ItemIsEnabled));
    } else {
        preferButton->setEnabled(false);
        deferButton->setEnabled(false);
        removeButton->setEnabled(false);
        testPlaybackButton->setEnabled(false);
    }
}

#include "moc_devicepreference.cpp"
// vim: sw=4 ts=4
