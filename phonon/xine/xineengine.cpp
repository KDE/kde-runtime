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

#include "xineengine.h"

#include "mediaobject.h"
#include <QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <phonon/audiodeviceenumerator.h>
#include <phonon/audiodevice.h>
#include <QList>
#include <kconfiggroup.h>
#include <kicon.h>
#include "videowidget.h"
#include <klocale.h>
#include "xineengine_p.h"
#include "backend.h"
#include "events.h"
#include "xinethread.h"

namespace Phonon
{
namespace Xine
{
    XineEnginePrivate::XineEnginePrivate()
    {
        signalTimer.setSingleShot(true);
        connect(&signalTimer, SIGNAL(timeout()), SLOT(emitAudioDeviceChange()));
        QDBusConnection::sessionBus().registerObject("/internal/PhononXine", this, QDBusConnection::ExportScriptableSlots);
    }

    void XineEnginePrivate::emitAudioDeviceChange()
    {
        kDebug(610) ;
        emit objectDescriptionChanged(AudioOutputDeviceType);
    }

    static XineEngine *s_instance = 0;

    XineEngine::XineEngine(const KSharedConfigPtr &_config)
        : m_xine(xine_new()),
        m_config(_config),
        m_inShutdown(false),
        d(new XineEnginePrivate),
        m_nullPort(0),
        m_nullVideoPort(0),
        m_thread(0)
    {
        Q_ASSERT(s_instance == 0);
        s_instance = this;
        KConfigGroup cg(m_config, "Settings");

        m_deinterlaceDVD = cg.readEntry("deinterlaceDVD", true);
        m_deinterlaceVCD = cg.readEntry("deinterlaceVCD", false);
        m_deinterlaceFile = cg.readEntry("deinterlaceFile", false);
        m_deinterlaceMethod = cg.readEntry("deinterlaceMethod", 0);
    }

    XineEngine::~XineEngine()
    {
        m_inShutdown = true;
        /*if (m_thread) {
            m_thread->stopAllStreams();
        }*/
        if (m_thread) {
            m_thread->quit();
            if (!m_thread->wait(10000)) {
                // timed out
                // assuming a deadlock, we better create a backtrace than block something important
                kFatal(610) << "Xine Thread took longer than 4s to quit. Assuming a deadlock. Please report a useful backtrace (including all threads) to bugs.kde.org";
                // if somebody made kFatal non-fatal
                m_thread->terminate();
            }
            delete m_thread;
        }

        QList<QObject *> cleanupObjects(m_cleanupObjects);
#if 0
        qDeleteAll(cleanupObjects);
#else
        const QList<QObject *>::Iterator end = cleanupObjects.end();
        QList<QObject *>::Iterator it = cleanupObjects.begin();
        while (it != end) {
            kDebug(610) << "delete" << (*it)->metaObject()->className();
            delete *it;
            ++it;
        }
#endif

        //kDebug(610) ;
        if (m_nullPort) {
            xine_close_audio_driver(m_xine, m_nullPort);
        }
        if (m_nullVideoPort) {
            xine_close_video_driver(m_xine, m_nullVideoPort);
        }
        xine_exit(m_xine);
        m_xine = 0;
        s_instance = 0;
        delete d;
    }

    XineEngine *XineEngine::self()
    {
        Q_ASSERT(s_instance);
        return s_instance;
    }

    const QObject *XineEngine::sender()
    {
        return self()->d;
    }

    xine_t *XineEngine::xine()
    {
        return self()->m_xine;
    }

    XineThread *XineEngine::thread()
    {
        XineEngine *const e = self();
        if (!e->m_thread) {
            e->m_thread = new XineThread;
            e->m_thread->moveToThread(e->m_thread);
            e->m_thread->start();
            e->m_thread->waitForEventLoop();
        }
        return e->m_thread;
    }

    bool XineEngine::deinterlaceDVD()
    {
        return s_instance->m_deinterlaceDVD;
    }

    bool XineEngine::deinterlaceVCD()
    {
        return s_instance->m_deinterlaceVCD;
    }

    bool XineEngine::deinterlaceFile()
    {
        return s_instance->m_deinterlaceFile;
    }

    int XineEngine::deinterlaceMethod()
    {
        return s_instance->m_deinterlaceMethod;
    }

    void XineEngine::xineEventListener(void *p, const xine_event_t *xineEvent)
    {
        if (!p || !xineEvent) {
            return;
        }
        //kDebug(610) << "Xine event: " << xineEvent->type << QByteArray((char *)xineEvent->data, xineEvent->data_length);

        XineStream *xs = static_cast<XineStream *>(p);

        switch (xineEvent->type) {
        case XINE_EVENT_UI_SET_TITLE: /* request title display change in ui */
            QCoreApplication::postEvent(xs, new QEVENT(NewMetaData));
            break;
        case XINE_EVENT_UI_PLAYBACK_FINISHED: /* frontend can e.g. move on to next playlist entry */
            QCoreApplication::postEvent(xs, new QEVENT(MediaFinished));
            break;
        case XINE_EVENT_PROGRESS: /* index creation/network connections */
            {
                xine_progress_data_t *progress = static_cast<xine_progress_data_t *>(xineEvent->data);
                QCoreApplication::postEvent(xs, new ProgressEvent(QString::fromUtf8(progress->description), progress->percent));
            }
            break;
        case XINE_EVENT_SPU_BUTTON: // the mouse pointer enter/leave a button, used to change the cursor
            {
                xine_spu_button_t *button = static_cast<xine_spu_button_t *>(xineEvent->data);
                if (button->direction == 1) { // enter a button
                    xs->handleDownstreamEvent(new QEVENT(NavButtonIn));
                } else {
                    xs->handleDownstreamEvent(new QEVENT(NavButtonOut));
                }
            }
            break;
        case XINE_EVENT_UI_CHANNELS_CHANGED:    /* inform ui that new channel info is available */
            kDebug(610) << "XINE_EVENT_UI_CHANNELS_CHANGED";
            {
                QCoreApplication::postEvent(xs, new QEVENT(UiChannelsChanged));
            }
            break;
        case XINE_EVENT_UI_MESSAGE:             /* message (dialog) for the ui to display */
            {
                kDebug(610) << "XINE_EVENT_UI_MESSAGE";
                const xine_ui_message_data_t *message = static_cast<xine_ui_message_data_t *>(xineEvent->data);
                if (message->type == XINE_MSG_AUDIO_OUT_UNAVAILABLE) {
                    kDebug(610) << "XINE_MSG_AUDIO_OUT_UNAVAILABLE";
                    // we don't know for sure which AudioOutput failed. but the one without any
                    // capabilities must be the guilty one
                    xs->handleDownstreamEvent(new QEVENT(AudioDeviceFailed));
                }
            }
            break;
        case XINE_EVENT_FRAME_FORMAT_CHANGE:    /* e.g. aspect ratio change during dvd playback */
            kDebug(610) << "XINE_EVENT_FRAME_FORMAT_CHANGE";
            {
                xine_format_change_data_t *data = static_cast<xine_format_change_data_t *>(xineEvent->data);
                xs->handleDownstreamEvent(new FrameFormatChangeEvent(data->width, data->height, data->aspect, data->pan_scan));
            }
            break;
        case XINE_EVENT_AUDIO_LEVEL:            /* report current audio level (l/r/mute) */
            kDebug(610) << "XINE_EVENT_AUDIO_LEVEL";
            break;
        case XINE_EVENT_QUIT:                   /* last event sent when stream is disposed */
            kDebug(610) << "XINE_EVENT_QUIT";
            break;
        case XINE_EVENT_UI_NUM_BUTTONS:         /* number of buttons for interactive menus */
            kDebug(610) << "XINE_EVENT_UI_NUM_BUTTONS";
            break;
        case XINE_EVENT_DROPPED_FRAMES:         /* number of dropped frames is too high */
            kDebug(610) << "XINE_EVENT_DROPPED_FRAMES";
            break;
        case XINE_EVENT_MRL_REFERENCE_EXT:      /* demuxer->frontend: MRL reference(s) for the real stream */
            {
                xine_mrl_reference_data_ext_t *reference = static_cast<xine_mrl_reference_data_ext_t *>(xineEvent->data);
                kDebug(610) << "XINE_EVENT_MRL_REFERENCE_EXT: " << reference->alternative
                    << ", " << reference->start_time
                    << ", " << reference->duration
                    << ", " << reference->mrl
                    << ", " << (reference->mrl + strlen(reference->mrl) + 1)
                    ;
                QCoreApplication::postEvent(xs, new ReferenceEvent(reference->alternative, reference->mrl));
            }
            break;
    }
    }

    xine_audio_port_t *XineEngine::nullPort()
    {
        if (!s_instance->m_nullPort) {
            s_instance->m_nullPort = xine_open_audio_driver(s_instance->m_xine, "none", 0);
        }
        Q_ASSERT(s_instance->m_nullPort);
        return s_instance->m_nullPort;
    }

    xine_video_port_t *XineEngine::nullVideoPort()
    {
        if (!s_instance->m_nullVideoPort) {
            s_instance->m_nullVideoPort = xine_open_video_driver(s_instance->m_xine, "auto", XINE_VISUAL_TYPE_NONE, 0);
        }
        Q_ASSERT(s_instance->m_nullVideoPort);
        return s_instance->m_nullVideoPort;
    }

    QList<int> XineEngine::audioOutputIndexes()
    {
        XineEngine *that = self();
        that->checkAudioOutputs();
        QList<int> list;
        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            list << that->m_audioOutputInfos[i].index;
        }
        return list;
    }

    QHash<QByteArray, QVariant> XineEngine::audioOutputProperties(int audioDevice)
    {
        QHash<QByteArray, QVariant> ret;
        XineEngine *that = self();
        that->checkAudioOutputs();

        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            if (that->m_audioOutputInfos[i].index == audioDevice) {
                ret.insert("name", that->m_audioOutputInfos[i].name);
                ret.insert("description", that->m_audioOutputInfos[i].description);

                const QString iconName = that->m_audioOutputInfos[i].icon;
                if (!iconName.isEmpty()) {
                    ret.insert("icon", KIcon(iconName));
                }
                ret.insert("available", that->m_audioOutputInfos[i].available);

                ret.insert("initialPreference", that->m_audioOutputInfos[i].initialPreference);
                ret.insert("isAdvanced", that->m_audioOutputInfos[i].isAdvanced);

                return ret;
            }
        }
        ret.insert("name", QString());
        ret.insert("description", QString());
        ret.insert("available", false);
        ret.insert("initialPreference", 0);
        ret.insert("isAdvanced", false);
        return ret;
    }

    QByteArray XineEngine::audioDriverFor(int audioDevice)
    {
        XineEngine *that = self();
        that->checkAudioOutputs();
        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            if (that->m_audioOutputInfos[i].index == audioDevice) {
                return that->m_audioOutputInfos[i].driver;
            }
        }
        return QByteArray();
    }

    QStringList XineEngine::alsaDevicesFor(int audioDevice)
    {
        XineEngine *that = self();
        that->checkAudioOutputs();
        for (int i = 0; i < that->m_audioOutputInfos.size(); ++i) {
            if (that->m_audioOutputInfos[i].index == audioDevice) {
                if (that->m_audioOutputInfos[i].driver == "alsa") { // only for ALSA
                    //return that->m_audioOutputInfos[i].devices;
                }
            }
        }
        return QStringList();
    }

    void XineEngine::addAudioOutput(int index, int initialPreference, const QString &name, const QString &description,
            const QString &icon, const QByteArray &driver, bool isAdvanced)
    {
        AudioOutputInfo info(index, initialPreference, name, description, icon, driver);
        info.isAdvanced = isAdvanced;
        const int listIndex = m_audioOutputInfos.indexOf(info);
        if (listIndex == -1) {
            info.available = true;
            m_audioOutputInfos << info;
        } else {
            AudioOutputInfo &infoInList = m_audioOutputInfos[listIndex];
            if (infoInList.icon != icon || infoInList.initialPreference != initialPreference) {
                infoInList.icon = icon;
                infoInList.initialPreference = initialPreference;
            }
            infoInList.available = true;
        }
    }

    void XineEngine::checkAudioOutputs()
    {
        if (m_audioOutputInfos.isEmpty()) {
            kDebug(610) << "isEmpty";
            int nextIndex = 10000;

            // This will list the audio drivers, not the actual devices.
            const char *const *outputPlugins = xine_list_audio_output_plugins(xine());
            for (int i = 0; outputPlugins[i]; ++i) {
                kDebug(610) << "outputPlugin: " << outputPlugins[i];
                if (0 == strcmp(outputPlugins[i], "alsa")
                        || 0 == strcmp(outputPlugins[i], "none")
                        || 0 == strcmp(outputPlugins[i], "file")
                        || 0 == strcmp(outputPlugins[i], "oss")) {
                    // ignore these drivers (hardware devices are listed by the KDE platform plugin)
                } else if (0 == strcmp(outputPlugins[i], "jack")) {
                    addAudioOutput(nextIndex++, 9, i18n("Jack Audio Connection Kit"),
                            i18n("<html><p>JACK is a low-latency audio server. It can connect a number "
                                "of different applications to an audio device, as well as allowing "
                                "them to share audio between themselves.</p>"
                                "<p>JACK was designed from the ground up for professional audio "
                                "work, and its design focuses on two key areas: synchronous "
                                "execution of all clients, and low latency operation.</p></html>"),
                            /*icon name */"audio-backend-jack", outputPlugins[i]);
                } else if (0 == strcmp(outputPlugins[i], "arts")) {
                    addAudioOutput(nextIndex++, -100, i18n("aRts"),
                            i18n("<html><p>aRts is the old soundserver and media framework that was used "
                                "in KDE2 and KDE3. Its use is discouraged.</p></html>"),
                            /*icon name */"audio-backend-arts", outputPlugins[i]);
                } else if (0 == strcmp(outputPlugins[i], "pulseaudio")) {
                    addAudioOutput(nextIndex++, 10, i18n("PulseAudio"),
                            xine_get_audio_driver_plugin_description(xine(), outputPlugins[i]),
                            /*icon name */"audio-backend-pulseaudio", outputPlugins[i], true /*isAdvanced*/);
                } else if (0 == strcmp(outputPlugins[i], "esd")) {
                    addAudioOutput(nextIndex++, 8, i18n("Esound (ESD)"),
                            xine_get_audio_driver_plugin_description(xine(), outputPlugins[i]),
                            /*icon name */"audio-backend-esd", outputPlugins[i]);
                } else {
                    addAudioOutput(nextIndex++, -20, outputPlugins[i],
                            xine_get_audio_driver_plugin_description(xine(), outputPlugins[i]),
                            /*icon name */outputPlugins[i], outputPlugins[i]);
                }
            }

            qSort(m_audioOutputInfos);

            // now m_audioOutputInfos holds all devices this computer has ever seen
            foreach (const AudioOutputInfo &info, m_audioOutputInfos) {
                kDebug(610) << info.index << info.name << info.driver;
            }
        }
    }

    void XineEngine::setObjectDescriptionProperities( ObjectDescriptionType type, int index, const QHash<QByteArray, QVariant>& properities )
    {
        s_instance->m_objectDescriptions[type][index] = properities;
    }

} // namespace Xine
} // namespace Phonon

#include "xineengine_p.moc"
// vim: sw=4 ts=4 tw=100 et
