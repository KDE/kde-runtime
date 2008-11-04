/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PHONON_KDEPLUGINFACTORY_H
#define PHONON_KDEPLUGINFACTORY_H

#include <phonon/platformplugin.h>
#include <phonon/objectdescription.h>
#include <QtCore/QObject>
#include <kservice.h>

namespace Phonon
{

class DeviceListing;

class KdePlatformPlugin : public QObject, public PlatformPlugin
{
    Q_INTERFACES(Phonon::PlatformPlugin)
    Q_OBJECT
    public:
        KdePlatformPlugin();
        ~KdePlatformPlugin();

        AbstractMediaStream *createMediaStream(const QUrl &url, QObject *parent);

        QIcon icon(const QString &name) const;
        void notification(const char *notificationName, const QString &text,
                const QStringList &actions, QObject *receiver,
                const char *actionSlot) const;
        QString applicationName() const;
        QObject *createBackend();
        QObject *createBackend(const QString &library, const QString &version);
        bool isMimeTypeAvailable(const QString &mimeType) const;
        void saveVolume(const QString &outputName, qreal volume);
        qreal loadVolume(const QString &outputName) const;

        virtual QList<int> objectDescriptionIndexes(ObjectDescriptionType type) const;
        virtual QHash<QByteArray, QVariant> objectDescriptionProperties(ObjectDescriptionType type, int index) const;

        virtual QList<QPair<QByteArray, QString> > deviceAccessListFor(const Phonon::AudioOutputDevice &) const;

    signals:
        void objectDescriptionChanged(ObjectDescriptionType);

    private:
        QObject *createBackend(KService::Ptr newService);
        void ensureDeviceListingObject() const;
        mutable DeviceListing *m_devList;
};

} // namespace Phonon

#endif // PHONON_KDEPLUGINFACTORY_H
