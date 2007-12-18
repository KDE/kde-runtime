/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "backend.h"
#include <QtCore/QDebug>
#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtPlugin>

#include "backendheader.h"

#include "videowidget.h"
#include "audiooutput.h"
#include "mediaobject.h"
#include "videoeffect.h"
#include "medianode.h"
#include "audiodevice.h"
#include "audiomixer.h"
#include "backendinfo.h"

#include "audiograph.h"
#include "audiomixer.h"
#include "audiooutput.h"
#include "audiosplitter.h"
#include "audioeffects.h"

namespace Phonon
{
namespace QT7
{

Backend::Backend()
{
    IMPLEMENTED << "Creating backend QT7";
}

Backend::Backend(QObject *parent, const QStringList &) : QObject(parent)
{
    IMPLEMENTED << "Creating backend QT7";
    setProperty("identifier",     QLatin1String("Mac OS X/QuickTime7"));
    setProperty("backendName",    QLatin1String("Mac OS X/QuickTime7"));
    setProperty("backendComment", QLatin1String("Developed by Trolltech"));
    setProperty("backendVersion", QLatin1String("0.1"));
    setProperty("backendIcon",    QLatin1String(""));
    setProperty("backendWebsite", QLatin1String("http://www.trolltech.com/"));
}

Backend::~Backend()
{
}

QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
{
    switch (c) {
    case MediaObjectClass:
        IMPLEMENTED << "Creating new MediaObjectClass.";
        return new MediaObject(parent);
        break;
    case VolumeFaderEffectClass:
        IMPLEMENTED << "Creating new VolumeFaderEffectClass.";
        return new AudioMixer(parent);
        break;
    case AudioOutputClass:
        IMPLEMENTED << "Creating new AudioOutputClass.";
        return new AudioOutput(parent);
        break;
    case AudioDataOutputClass:
        NOT_IMPLEMENTED << "Creating new AudioDataOutputClass.";
        break;
    case VisualizationClass:
        NOT_IMPLEMENTED << "Creating new VisualizationClass.";
        break;
    case VideoDataOutputClass:
        NOT_IMPLEMENTED << "Creating new VideoDataOutputClass.";
        break;
    case EffectClass:
        IMPLEMENTED << "Creating new EffectClass.";
        return new AudioEffect(args[0].toInt());
        break;
    case VideoWidgetClass:
        IMPLEMENTED << "Creating new VideoWidget.";
        return new VideoWidget(parent);
        break;
    default:
        return 0;
    }
    return 0;
}

bool Backend::startConnectionChange(QSet<QObject *> objects)
{
    IMPLEMENTED;
    for (int i=0; i<objects.size(); i++){
        MediaNode *node = qobject_cast<MediaNode*>(objects.values()[i]);
        if (node && node->audioGraph()){
            if (node->audioGraph()->graphCannotPlay())
                node->audioGraph()->startAllOverFromScratch();
            MediaNodeEvent event(MediaNodeEvent::AboutToRestartAudioStream);
            node->audioGraph()->root()->notify(&event);
        }
    }        
    return true;
}

bool Backend::endConnectionChange(QSet<QObject *> objects)
{
    IMPLEMENTED;
    if (!objects.empty()){      
        // Update the audio graph, and start it
        // if it should be running:
        MediaNode *node = qobject_cast<MediaNode*>(objects.values()[0]);
        if (node && node->audioGraph()){
            node->audioGraph()->update();
            MediaNodeEvent event(MediaNodeEvent::RestartAudioStreamRequest);
            node->audioGraph()->root()->notify(&event);
        }
    }
    return true;
}

bool Backend::connectNodes(QObject *aSource, QObject *aSink)
{
    IMPLEMENTED;
    MediaNode *source = qobject_cast<MediaNode*>(aSource);
    if (!source) return false;
    MediaNode *sink = qobject_cast<MediaNode*>(aSink);
    if (!sink) return false;

    return source->connectToSink(sink);
}


bool Backend::disconnectNodes(QObject *aSource, QObject *aSink)
{
    IMPLEMENTED;
    MediaNode *source = qobject_cast<MediaNode*>(aSource);
    if (!source) return false;
    MediaNode *sink = qobject_cast<MediaNode*>(aSink);
    if (!sink) return false;

    return source->disconnectToSink(sink);
}


QStringList Backend::availableMimeTypes() const
{
    IMPLEMENTED;
    return BackendInfo::quickTimeMimeTypes(BackendInfo::In).keys();
}

/**
* Returns a set of indexes that acts as identifiers for the various properties
* this backend supports for the given ObjectDescriptionType.
* More information for a given property/index can be
* looked up in Backend::objectDescriptionProperties(...).
*/
QList<int> Backend::objectDescriptionIndexes(ObjectDescriptionType type) const
{
    QList<int> ret;

    switch (type){
    case AudioOutputDeviceType:{
        IMPLEMENTED_SILENT << "Creating index set for type: AudioOutputDeviceType";
        QList<AudioDeviceID> devices = AudioDevice::devices(AudioDevice::Out);
        for (int i=0; i<devices.size(); i++)
            ret << int(devices[i]);
        break; }
    case EffectType:{
        IMPLEMENTED_SILENT << "Creating index set for type: EffectType";
        ret = AudioEffect::effectList();
        break; }
        
#if 0 // will be awailable in a later version of phonon.
    case AudioCaptureDeviceType:{
        IMPLEMENTED_SILENT << "Creating index set for type: AudioCaptureDeviceType";
        QList<AudioDeviceID> devices = AudioDevice::devices(AudioDevice::In).keys();
        for (int i=0; i<devices.size(); i++)
            ret <<int(devices[i]);
        break; }
    case VideoEffectType:{
        // Just count the number of filters awailable (c), and
        // add return a set with the numbers 1..c inserted:
        IMPLEMENTED_SILENT << "Creating index set for type: VideoEffectType";
        QList<QString> filters = objc_getCiFilterInfo()->filterDisplayNames;
        for (int i=0; i<filters.size(); i++)
            ret << insert(i);
        break; }
#endif
    default:
        NOT_IMPLEMENTED;
        break;
    }
    return ret;
}

QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{
    QHash<QByteArray, QVariant> ret;

    switch (type){
    case AudioOutputDeviceType:{
        IMPLEMENTED_SILENT << "Creating description hash for type: AudioOutputDeviceType";
        ret.insert("name", AudioDevice::deviceSourceNameElseDeviceName(index));
        ret.insert("description", AudioDevice::deviceNameElseDeviceSourceName(index));
        break; }
    case EffectType:{
        AudioEffect e(index);
        ret.insert("name", e.name());
        ret.insert("description", e.description());
        break; }
        
#if 0 // will be awailable in a later version of phonon.
    case VideoEffectType:{
        // Get list of effects, pick out filter at index, and return its name:
        IMPLEMENTED_SILENT << "Creating description hash for type: VideoEffectType";
        QList<QString> filters = objc_getCiFilterInfo()->filterDisplayNames;
        ret.insert("name", filters[index]);
    case AudioCaptureDeviceType:{
        IMPLEMENTED_SILENT << "Creating description hash for type: AudioCaptureDeviceType";
        QMap<AudioDeviceID, QString> devices = AudioDevice::devices(AudioDevice::In);
        ret.insert("name", devices.value(index));
        break; }
#endif
    default:
        NOT_IMPLEMENTED;
        break;
    }

    return ret;
}

Q_EXPORT_PLUGIN2(phonon_qt7, Backend)
}}

#include "moc_backend.cpp"

