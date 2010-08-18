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
#include <phonon/objectdescription.h>
#include <QtCore/QBasicTimer>
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
        Q_SCRIPTABLE QByteArray audioDevicesIndexes(int type);
        Q_SCRIPTABLE QByteArray audioDevicesProperties(int index);
        Q_SCRIPTABLE bool isAudioDeviceRemovable(int index) const;
        Q_SCRIPTABLE void removeAudioDevices(const QList<int> &indexes);

    protected:
        void timerEvent(QTimerEvent *e);

    private slots:
        void deviceAdded(const QString &udi);
        void deviceRemoved(const QString &udi);
        // TODO add callbacks for Jack changes and whatever else, if somehow possible (Pulse handled by libphonon)

        void alsaConfigChanged();

        void askToRemoveDevices(const QStringList &, const QList<int> &indexes);

    private:
        void findDevices();
        void findVirtualDevices();
        void updateAudioDevicesCache();

        KSharedConfigPtr m_config;
        QBasicTimer m_updateDeviceListing;

        // cache
        QByteArray m_audioOutputDevicesIndexesCache;
        QByteArray m_audioCaptureDevicesIndexesCache;
        QHash<int, QByteArray> m_audioDevicesPropertiesCache;

        // devices ordered by preference
        QList<PS::AudioDevice> m_audioOutputDevices;
        QList<PS::AudioDevice> m_audioCaptureDevices;

        QStringList m_udisOfAudioDevices;
};

#endif // PHONONSERVER_H
