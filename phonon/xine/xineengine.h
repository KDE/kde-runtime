/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
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

#ifndef XINEENGINE_H
#define XINEENGINE_H

#include <QtCore/QHash>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <Phonon/Global>

#include <KDebug>
#include <KSharedConfig>

#include <xine.h>

namespace Phonon
{
    class AudioDevice;

namespace Xine
{
    enum MediaStreamType {
        Audio = 1,
        Video = 2,
        StillImage = 4,
        Subtitle = 8,
        AllMedia = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(MediaStreamTypes, MediaStreamType)
} // namespace Xine
} // namespace Phonon
Q_DECLARE_OPERATORS_FOR_FLAGS(Phonon::Xine::MediaStreamTypes)
namespace Phonon
{
namespace Xine
{
class Backend;
class XineEnginePrivate;
class XineThread;

class XineEngine
{
    friend class Phonon::Xine::Backend;
    friend class XineEnginePrivate;
    public:
        static XineEngine *self();
        static xine_t *xine();
        static void xineEventListener(void *, const xine_event_t *);

        static QList<int> audioOutputIndexes();
        static QHash<QByteArray, QVariant> audioOutputProperties(int audioDevice);
        static QByteArray audioDriverFor(int audioDevice);
        static QStringList alsaDevicesFor(int audioDevice);
        static xine_audio_port_t *nullPort();
        static xine_video_port_t *nullVideoPort();

        static const QObject *sender();

        static XineThread *thread();

        static void addCleanupObject(QObject *o) { self()->m_cleanupObjects << o; }
        static void removeCleanupObject(QObject *o) { self()->m_cleanupObjects.removeAll(o); }

        static bool deinterlaceDVD();
        static bool deinterlaceVCD();
        static bool deinterlaceFile();
        static int deinterlaceMethod();

    protected:
        XineEngine(const KSharedConfigPtr &cfg);
        ~XineEngine();

    private:
        void checkAudioOutputs();
        void addAudioOutput(const AudioDevice &dev, const QByteArray &driver);
        void addAudioOutput(int idx, int initialPreference, const QString &n,
                const QString &desc, const QString &ic, const QByteArray &dr,
                const QStringList &dev, const QString &mixerDevice);
        xine_t *m_xine;

        struct AudioOutputInfo
        {
            AudioOutputInfo(int idx, int ip, const QString &n, const QString &desc, const QString &ic,
                    const QByteArray &dr, const QStringList &dev, const QString &mdev)
                : devices(dev), name(n), description(desc), icon(ic), mixerDevice(mdev), driver(dr),
                index(idx), initialPreference(ip), available(false), isAdvanced(false) {}

            QStringList devices;
            QString name;
            QString description;
            QString icon;
            QString mixerDevice;
            QByteArray driver;
            int index;
            int initialPreference;
            bool available : 1;
            bool isAdvanced : 1;
            inline bool operator==(const AudioOutputInfo &rhs) const { return name == rhs.name && driver == rhs.driver; }
            inline bool operator<(const AudioOutputInfo &rhs) const { return initialPreference > rhs.initialPreference; }
        };
        QList<AudioOutputInfo> m_audioOutputInfos;
        QList<QObject *> m_cleanupObjects;
        KSharedConfigPtr m_config;
        int m_deinterlaceMethod : 8;
        enum UseOss {
            False = 0,
            True = 1,
            Unknown = 2
        };
        UseOss m_useOss : 2;
        bool m_deinterlaceDVD : 1;
        bool m_deinterlaceVCD : 1;
        bool m_deinterlaceFile : 1;
        const XineEnginePrivate *const d;
        xine_audio_port_t *m_nullPort;
        xine_video_port_t *m_nullVideoPort;
        XineThread *m_thread;
};
}
}

/**
 * Implements needed operator to use Phonon::State with kDebug
 */
inline kdbgstream &operator<<(kdbgstream &stream, const Phonon::State state)
{
    switch(state)
    {
    case Phonon::ErrorState:
        stream << "ErrorState";
        break;
    case Phonon::LoadingState:
        stream << "LoadingState";
        break;
    case Phonon::StoppedState:
        stream << "StoppedState";
        break;
    case Phonon::PlayingState:
        stream << "PlayingState";
        break;
    case Phonon::BufferingState:
        stream << "BufferingState";
        break;
    case Phonon::PausedState:
        stream << "PausedState";
        break;
    }
    return stream;
}

#endif // XINEENGINE_H
// vim: sw=4 ts=4 tw=80 et
