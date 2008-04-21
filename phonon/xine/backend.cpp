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

#include "backend.h"
#include "mediaobject.h"
#include "effect.h"
#include "events.h"
#include "audiooutput.h"
#include "audiodataoutput.h"
#include "nullsink.h"
#include "visualization.h"
#include "volumefadereffect.h"
#include "videodataoutput.h"
#include "videowidget.h"
#include "wirecall.h"
#include "xinethread.h"

#include <kdebug.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtGui/QApplication>

#include <phonon/audiodevice.h>
#include <phonon/audiodeviceenumerator.h>

extern "C" {
#include <xine/xine_plugin.h>
#include "shareddata.h"
#include "sinknode.h"
#include "sourcenode.h"
extern plugin_info_t phonon_xine_plugin_info[];
}

K_PLUGIN_FACTORY(XineBackendFactory, registerPlugin<Phonon::Xine::Backend>();)
K_EXPORT_PLUGIN(XineBackendFactory("xinebackend"))

namespace Phonon
{
namespace Xine
{

Backend::Backend(QObject *parent, const QVariantList &)
    : QObject(parent)
{
    setProperty("identifier",     QLatin1String("phonon_xine"));
    setProperty("backendName",    QLatin1String("Xine"));
    setProperty("backendComment", i18n("Phonon Xine Backend"));
    setProperty("backendVersion", QLatin1String("0.1"));
    setProperty("backendIcon",    QLatin1String("phonon-xine"));
    setProperty("backendWebsite", QLatin1String("http://multimedia.kde.org/"));

    new XineEngine(XineBackendFactory::componentData().config());
    char configfile[2048];

    const QByteArray phonon_xine_verbosity(getenv("PHONON_XINE_VERBOSITY"));
    kDebug(610) << "setting xine verbosity to" << phonon_xine_verbosity.toInt();
    xine_engine_set_param(XineEngine::xine(), XINE_ENGINE_PARAM_VERBOSITY, phonon_xine_verbosity.toInt());
    sprintf(configfile, "%s%s", xine_get_homedir(), "/.xine/config");
    xine_config_load(XineEngine::xine(), configfile);
    xine_init(XineEngine::xine());

    kDebug(610) << "Using Xine version " << xine_get_version_string();

    connect(XineEngine::sender(), SIGNAL(objectDescriptionChanged(ObjectDescriptionType)),
            SIGNAL(objectDescriptionChanged(ObjectDescriptionType)));

    xine_register_plugins(XineEngine::xine(), phonon_xine_plugin_info);
}

Backend::~Backend()
{
    delete XineEngine::self();
}

QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
{
    switch (c) {
    case MediaObjectClass:
        return new MediaObject(parent);
    case VolumeFaderEffectClass:
        return new VolumeFaderEffect(parent);
    case AudioOutputClass:
        return new AudioOutput(parent);
    case AudioDataOutputClass:
        return new AudioDataOutput(parent);
    case VisualizationClass:
        return new Visualization(parent);
    case VideoDataOutputClass:
        return new VideoDataOutput(parent);
    case EffectClass:
        {
            Q_ASSERT(args.size() == 1);
            kDebug(610) << "creating Effect(" << args[0];
            Effect *e = new Effect(args[0].toInt(), parent);
            if (e->isValid()) {
                return e;
            }
            delete e;
            return 0;
        }
    case VideoWidgetClass:
        {
            VideoWidget *vw = new VideoWidget(qobject_cast<QWidget *>(parent));
            if (vw->isValid()) {
                return vw;
            }
            delete vw;
            return 0;
        }
    }
    return 0;
}

bool Backend::supportsVideo() const
{
    return true;
}

bool Backend::supportsOSD() const
{
    return true;
}

bool Backend::supportsFourcc(quint32 fourcc) const
{
    switch(fourcc)
    {
    case 0x00000000:
        return true;
    default:
        return false;
    }
}

bool Backend::supportsSubtitles() const
{
    return true;
}

QStringList Backend::availableMimeTypes() const
{
    if (m_supportedMimeTypes.isEmpty())
    {
        char *mimeTypes_c = xine_get_mime_types(XineEngine::xine());
        QString mimeTypes(mimeTypes_c);
        free(mimeTypes_c);
        QStringList lstMimeTypes = mimeTypes.split(";", QString::SkipEmptyParts);
        foreach (QString mimeType, lstMimeTypes)
            m_supportedMimeTypes << mimeType.left(mimeType.indexOf(':')).trimmed();
        if (m_supportedMimeTypes.contains("application/ogg"))
            m_supportedMimeTypes << QLatin1String("audio/x-vorbis+ogg") << QLatin1String("application/ogg");
    }

    return m_supportedMimeTypes;
}

QList<int> Backend::objectDescriptionIndexes(ObjectDescriptionType type) const
{
    QList<int> list;
    switch(type)
    {
    case Phonon::AudioOutputDeviceType:
        return XineEngine::audioOutputIndexes();
    case Phonon::AudioCaptureDeviceType:
        break;
/*
    case Phonon::VideoOutputDeviceType:
        {
            const char *const *outputPlugins = xine_list_video_output_plugins(XineEngine::xine());
            for (int i = 0; outputPlugins[i]; ++i)
                list << 40000 + i;
            break;
        }
    case Phonon::VideoCaptureDeviceType:
        list << 30000 << 30001;
        break;
    case Phonon::VisualizationType:
        break;
    case Phonon::AudioCodecType:
        break;
    case Phonon::VideoCodecType:
        break;
    case Phonon::ContainerFormatType:
        break;
        */
    case Phonon::EffectType:
        {
            const char *const *postPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_AUDIO_FILTER);
            for (int i = 0; postPlugins[i]; ++i)
                list << 0x7F000000 + i;
            /*const char *const *postVPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_VIDEO_FILTER);
            for (int i = 0; postVPlugins[i]; ++i) {
                list << 0x7E000000 + i;
            } */
            break;
        }
    case Phonon::AudioStreamType:
    case Phonon::SubtitleStreamType:
        {
            ObjectDescriptionHash hash = XineEngine::objectDescriptions();
            ObjectDescriptionHash::iterator it = hash.find(type);
            if( it != hash.end() )
                list = it.value().keys();
        }
        break;
    }
    return list;
}

QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{
    //kDebug(610) << type << index;
    QHash<QByteArray, QVariant> ret;
    switch (type) {
    case Phonon::AudioOutputDeviceType:
        ret = XineEngine::audioOutputProperties(index);
        break;
    case Phonon::AudioCaptureDeviceType:
        break;
        /*
    case Phonon::VideoOutputDeviceType:
        {
            const char *const *outputPlugins = xine_list_video_output_plugins(XineEngine::xine());
            for (int i = 0; outputPlugins[i]; ++i) {
                if (40000 + i == index) {
                    ret.insert("name", QLatin1String(outputPlugins[i]));
                    ret.insert("description", "");
                    // description should be the result of the following call, but it crashes.
                    // It looks like libxine initializes the plugin even when we just want the description...
                    //QLatin1String(xine_get_video_driver_plugin_description(XineEngine::xine(), outputPlugins[i])));
                    break;
                }
            }
        }
        break;
    case Phonon::VideoCaptureDeviceType:
        switch (index) {
        case 30000:
            ret.insert("name", "USB Webcam");
            ret.insert("description", "first description");
            break;
        case 30001:
            ret.insert("name", "DV");
            ret.insert("description", "second description");
            break;
        }
        break;
    case Phonon::VisualizationType:
        break;
    case Phonon::AudioCodecType:
        break;
    case Phonon::VideoCodecType:
        break;
    case Phonon::ContainerFormatType:
        break;
        */
    case Phonon::EffectType:
        {
            const char *const *postPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_AUDIO_FILTER);
            for (int i = 0; postPlugins[i]; ++i) {
                if (0x7F000000 + i == index) {
                    ret.insert("name", QLatin1String(postPlugins[i]));
                    ret.insert("description", QLatin1String(xine_get_post_plugin_description(XineEngine::xine(), postPlugins[i])));
                    break;
                }
            }
            /*const char *const *postVPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_VIDEO_FILTER);
            for (int i = 0; postVPlugins[i]; ++i) {
                if (0x7E000000 + i == index) {
                    ret.insert("name", QLatin1String(postPlugins[i]));
                    break;
                }
            } */
        }
        break;
    case Phonon::AudioStreamType:
    case Phonon::SubtitleStreamType:
        {
            ObjectDescriptionHash descriptionHash = XineEngine::objectDescriptions();
            ObjectDescriptionHash::iterator descIt = descriptionHash.find(type);
            if(descIt != descriptionHash.end())
            {
                ChannelIndexHash indexHash = descIt.value();
                ChannelIndexHash::iterator indexIt = indexHash.find(index);
                if(indexIt != indexHash.end() )
                {
                    ret = indexIt.value();
                }
            }
        }
        break;
    }
    return ret;
}

bool Backend::startConnectionChange(QSet<QObject *> nodes)
{
    Q_UNUSED(nodes);
    // there's nothing we can do but hope the connection changes won't take too long so that buffers
    // would underrun. But we should be pretty safe the way xine works by not doing anything here.
    return true;
}

bool Backend::connectNodes(QObject *_source, QObject *_sink)
{
    kDebug(610);
    SourceNode *source = qobject_cast<SourceNode *>(_source);
    SinkNode *sink = qobject_cast<SinkNode *>(_sink);
    if (!source || !sink) {
        return false;
    }
    // what streams to connect - i.e. all both nodes support
    const MediaStreamTypes types = source->outputMediaStreamTypes() & sink->inputMediaStreamTypes();
    if (sink->source() != 0 || source->sinks().contains(sink)) {
        return false;
    }
    NullSink *nullSink = 0;
    foreach (SinkNode *otherSinks, source->sinks()) {
        if (otherSinks->inputMediaStreamTypes() & types) {
            if (nullSink) {
                kWarning(610) << "phonon-xine does not support splitting of audio or video streams into multiple outputs. The sink node is already connected to" << otherSinks->threadSafeObject().data();
                return false;
            } else {
                nullSink = dynamic_cast<NullSink *>(otherSinks);
                if (!nullSink) {
                    kWarning(610) << "phonon-xine does not support splitting of audio or video streams into multiple outputs. The sink node is already connected to" << otherSinks->threadSafeObject().data();
                    return false;
                }
            }
        }
    }
    if (nullSink) {
        source->removeSink(nullSink);
        nullSink->unsetSource(source);
    }
    source->addSink(sink);
    sink->setSource(source);
    return true;
}

bool Backend::disconnectNodes(QObject *_source, QObject *_sink)
{
    kDebug(610);
    SourceNode *source = qobject_cast<SourceNode *>(_source);
    SinkNode *sink = qobject_cast<SinkNode *>(_sink);
    if (!source || !sink) {
        return false;
    }
    const MediaStreamTypes types = source->outputMediaStreamTypes() & sink->inputMediaStreamTypes();
    if (!source->sinks().contains(sink) || sink->source() != source) {
        return false;
    }
    source->removeSink(sink);
    sink->unsetSource(source);
    return true;
}

class KeepReference : public QObject
{
    public:
        KeepReference()
        {
            moveToThread(QApplication::instance()->thread());
            XineEngine::addCleanupObject(this);

            // do this so that startTimer is called from the correct thread
            QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>(2345)));
        }

        ~KeepReference()
        {
            XineEngine::removeCleanupObject(this);
        }

        void addObject(QExplicitlySharedDataPointer<SinkNodeXT> sink) { sinks << sink; }
        void addObject(QExplicitlySharedDataPointer<SourceNodeXT> source) { sources << source; }

    protected:
        bool event(QEvent *e)
        {
            if (e->type() == 2345) {
                e->accept();
                startTimer(10000);
                return true;
            }
            return QObject::event(e);
        }

        void timerEvent(QTimerEvent *e)
        {
            killTimer(e->timerId());
            deleteLater();
        }

    private:
        QList<QExplicitlySharedDataPointer<SinkNodeXT> > sinks;
        QList<QExplicitlySharedDataPointer<SourceNodeXT> > sources;
};

bool Backend::endConnectionChange(QSet<QObject *> nodes)
{
    QList<WireCall> wireCallsUnordered;
    QList<WireCall> wireCalls;
    QList<QExplicitlySharedDataPointer<SharedData> > allXtObjects;
    KeepReference *keep = new KeepReference();

    // first we need to find all vertices of the subgraphs formed by the given nodes that are
    // source nodes but don't have a sink node connected and connect them to the NullSink, otherwise
    // disconnections won't work
    foreach (QObject *q, nodes) {
        SourceNode *source = qobject_cast<SourceNode *>(q);
        if (source && source->sinks().isEmpty()) {
            SinkNode *sink = qobject_cast<SinkNode *>(q);
            if (!sink || (sink && sink->source())) {
                WireCall w(source, NullSink::instance());
                wireCalls << w;
                wireCallsUnordered << w;
            }
        }
    }
    // Now that we know (by looking at the subgraph of nodes formed by the given nodes) what has to
    // be rewired we go over the nodes in order (from sink to source) and rewire them (all called
    // from the xine thread).
    foreach (QObject *q, nodes) {
        SourceNode *source = qobject_cast<SourceNode *>(q);
        if (source) {
            //keep->addObject(source->threadSafeObject());
            allXtObjects.append(QExplicitlySharedDataPointer<SharedData>(source->threadSafeObject().data()));
            foreach (SinkNode *sink, source->sinks()) {
                WireCall w(source, sink);
                if (wireCallsUnordered.contains(w)) {
                    Q_ASSERT(!wireCalls.contains(w));
                    wireCalls << w;
                } else {
                    wireCallsUnordered << w;
                }
            }
        }
        SinkNode *sink = qobject_cast<SinkNode *>(q);
        if (sink) {
            keep->addObject(sink->threadSafeObject());
            allXtObjects.append(QExplicitlySharedDataPointer<SharedData>(sink->threadSafeObject().data()));
            if (sink->source()) {
                WireCall w(sink->source(), sink);
                if (wireCallsUnordered.contains(w)) {
                    Q_ASSERT(!wireCalls.contains(w));
                    wireCalls << w;
                } else {
                    wireCallsUnordered << w;
                }
            }
        }
        ConnectNotificationInterface *connectNotify = qobject_cast<ConnectNotificationInterface *>(q);
        if (connectNotify) {
            // the object wants to know when the graph has changed
            connectNotify->graphChanged();
        }
    }
    if (!wireCalls.isEmpty()) {
        qSort(wireCalls);
        // we want to be safe and make sure that the execution of a WireCall has all objects still
        // alive that were used in this connection change
        QList<WireCall>::Iterator it = wireCalls.begin();
        const QList<WireCall>::Iterator end = wireCalls.end();
        for (; it != end; ++it) {
            it->addReferenceTo(allXtObjects);
        }
        QCoreApplication::postEvent(XineEngine::thread(), new RewireEvent(wireCalls));
    }
    return true;
}

void Backend::freeSoundcardDevices()
{
}

}}

#include "backend.moc"

// vim: sw=4 ts=4
