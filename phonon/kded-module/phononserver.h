/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#ifndef PHONONSERVER_H
#define PHONONSERVER_H

#include <kdedmodule.h>
#include <ksharedconfig.h>
#include <Phonon/ObjectDescription>
#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtDBus/QDBusVariant>
#include "audiodevice.h"

class PhononServer : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.PhononServer")
    public:
        PhononServer(QObject *parent, const QList<QVariant> &args);
        ~PhononServer();

    public slots:
        Q_SCRIPTABLE QDBusVariant audioDevicesIndexes(int type);
        Q_SCRIPTABLE QDBusVariant audioDevicesProperties(int type, int index);

    signals:
        Q_SCRIPTABLE void audioOutputDevicesChanged(const QDBusVariant &devicesMap);
        Q_SCRIPTABLE void audioCaptureDevicesChanged(const QDBusVariant &devicesMap);

    private slots:
        void deviceAdded(const QString &udi);
        void deviceRemoved(const QString &udi);
        // TODO add callbacks for Pulse, Jack, asoundrc changes and whatever else, if somehow possible

    private:
        void findDevices();
        void findVirtualDevices();
        void updateAudioDevicesCache();

        KSharedConfigPtr m_config;

        // cache
        QVariant m_audioOutputDevicesIndexesCache;
        QVariant m_audioCaptureDevicesIndexesCache;
        QHash<int, QVariant> m_audioOutputDevicesPropertiesCache;
        QHash<int, QVariant> m_audioCaptureDevicesPropertiesCache;

        // devices ordered by preference
        QList<PS::AudioDevice> m_audioOutputDevices;
        QList<PS::AudioDevice> m_audioCaptureDevices;
};

#endif // PHONONSERVER_H
