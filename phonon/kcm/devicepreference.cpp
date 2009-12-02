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
        CategoryItem(Phonon::Category cat, bool output = true)
            : QStandardItem(cat == Phonon::NoCategory ? (output ? i18n("Audio Output") : i18n("Audio Capture")) : Phonon::categoryToString(cat)),
            isOutputItem(output),
            m_cat(cat)
        {
        }

        int type() const { return 1001; }
        Phonon::Category category() const { return m_cat; }
        const bool isOutputItem;

    private:
        Phonon::Category m_cat;
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
    QStandardItem *outputItem = new CategoryItem(Phonon::NoCategory);
    m_outputModel[Phonon::NoCategory] = new Phonon::AudioOutputDeviceModel(this);
    outputItem->setEditable(false);
    outputItem->setToolTip(i18n("Defines the default ordering of devices which can be overridden by individual categories."));
    parentItem->appendRow(outputItem);

    // Audio Capture Parent
    QStandardItem *captureItem = new CategoryItem(Phonon::NoCategory, false);
    m_captureModel[Phonon::NoCategory] = new Phonon::AudioCaptureDeviceModel(this);
    captureItem->setEditable(false);
    captureItem->setToolTip(i18n("Defines the default ordering of devices which can be overridden by individual categories."));
    parentItem->appendRow(captureItem);

    // Audio Output Children
    parentItem = outputItem;
    for (int i = 0; i <= Phonon::LastCategory; ++i) {
        m_outputModel[i] = new Phonon::AudioOutputDeviceModel(this);
        QStandardItem *item = new CategoryItem(static_cast<Phonon::Category>(i));
        item->setEditable(false);
        parentItem->appendRow(item);
    }

    // Audio Capture Children
    parentItem = captureItem;
    for (int i = 1; i < captureCategoriesCount; ++i) { // i == 1 to skip NoCategory
        m_captureModel[captureCategories[i]] = new Phonon::AudioCaptureDeviceModel(this);
        QStandardItem *item = new CategoryItem(captureCategories[i], false);
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
        connect(m_outputModel[i], SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(changed()));
        connect(m_outputModel[i], SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(changed()));
        connect(m_outputModel[i], SIGNAL(layoutChanged()), this, SIGNAL(changed()));
        connect(m_outputModel[i], SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SIGNAL(changed()));
        if (m_captureModel.contains(i)) {
            connect(m_captureModel[i], SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(changed()));
            connect(m_captureModel[i], SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(changed()));
            connect(m_captureModel[i], SIGNAL(layoutChanged()), this, SIGNAL(changed()));
            connect(m_captureModel[i], SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SIGNAL(changed()));
        }
    }
    connect(showCheckBox, SIGNAL(stateChanged (int)), this, SIGNAL(changed()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(availableAudioOutputDevicesChanged()), SLOT(updateAudioOutputDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(availableAudioCaptureDevicesChanged()), SLOT(updateAudioCaptureDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(capabilitiesChanged()), SLOT(updateAudioOutputDevices()));
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(capabilitiesChanged()), SLOT(updateAudioCaptureDevices()));

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
        if (catItem->isOutputItem) {
            deviceList->setModel(m_outputModel[cat]);
        } else {
            deviceList->setModel(m_captureModel[cat]);
        }
        m_showingOutputModel = catItem->isOutputItem;
        if (cat == Phonon::NoCategory) {
            if (catItem->isOutputItem) {
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Default Output Device Preference"), Qt::DisplayRole);
            } else {
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Default Capture Device Preference"), Qt::DisplayRole);
            }
        } else {
            if (catItem->isOutputItem) {
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Output Device Preference for the '%1' Category", Phonon::categoryToString(cat)), Qt::DisplayRole);
            } else {
                m_headerModel.setHeaderData(0, Qt::Horizontal, i18n("Capture Device Preference for the '%1' Category", Phonon::categoryToString(cat)), Qt::DisplayRole);
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
        Phonon::AudioCaptureDeviceModel *model = m_captureModel.value(i);
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

void DevicePreference::updateAudioOutputDevices()
{
    const QList<Phonon::AudioOutputDevice> list = availableAudioOutputDevices();
    QHash<int, Phonon::AudioOutputDevice> hash;
    foreach (const Phonon::AudioOutputDevice &dev, list) {
        hash.insert(dev.index(), dev);
    }
    for (int i = -1; i <= Phonon::LastCategory; ++i) {
        Phonon::AudioOutputDeviceModel *model = m_outputModel.value(i);
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
        m_outputModel[cat]->setModelData(list);
    }
    for (int i = 0; i < captureCategoriesCount; ++i) {
        const Phonon::Category cat = captureCategories[i];
        QList<Phonon::AudioCaptureDevice> list;
        const QList<int> deviceIndexes = Phonon::GlobalConfig().audioCaptureDeviceListFor(cat);
        foreach (int i, deviceIndexes) {
            list.append(Phonon::AudioCaptureDevice::fromIndex(i));
        }
        m_captureModel[cat]->setModelData(list);
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
        Q_ASSERT(m_outputModel.value(cat));
        const QList<int> order = m_outputModel.value(cat)->tupleIndexOrder();
        Phonon::GlobalConfig().setAudioOutputDeviceListFor(cat, order);
    }
    for (int i = 1; i < captureCategoriesCount; ++i) {
        const Phonon::Category cat = captureCategories[i];
        Q_ASSERT(m_captureModel.value(cat));
        const QList<int> order = m_captureModel.value(cat)->tupleIndexOrder();
        Phonon::GlobalConfig().setAudioCaptureDeviceListFor(cat, order);
    }
}

void DevicePreference::defaults()
{
    {
        const QList<Phonon::AudioOutputDevice> list = availableAudioOutputDevices();
        for (int i = -1; i <= Phonon::LastCategory; ++i) {
            m_outputModel[i]->setModelData(list);
        }
    }
    {
        const QList<Phonon::AudioCaptureDevice> list = availableAudioCaptureDevices();
        for (int i = 0; i < captureCategoriesCount; ++i) {
            m_captureModel[captureCategories[i]]->setModelData(list);
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

    QAbstractItemModel *model = deviceList->model();
    Phonon::AudioOutputDeviceModel *playbackModel = qobject_cast<Phonon::AudioOutputDeviceModel *>(model);
    if (playbackModel && idx.isValid()) {
        removeDevice(playbackModel->modelData(idx), &m_outputModel);
    } else {
        Phonon::AudioCaptureDeviceModel *captureModel = qobject_cast<Phonon::AudioCaptureDeviceModel *>(model);
        if (captureModel && idx.isValid()) {
            removeDevice(captureModel->modelData(idx), &m_captureModel);
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
    const QList<Phonon::AudioOutputDevice> modelData = m_outputModel.value(catItem->category())->modelData();

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
                    m_outputModel.value(cat)->setModelData(modelData);
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
