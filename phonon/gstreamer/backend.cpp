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

#include "common.h"
#include "backend.h"
#include "audiooutput.h"
#include "audioeffect.h"
#include "mediaobject.h"
#include "xvideowidget.h"
#include "videowidget.h"
#include "devicemanager.h"
#include "effectmanager.h"
#include "message.h"
#include <gst/interfaces/propertyprobe.h>

#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtCore/QtPlugin>

Q_EXPORT_PLUGIN2(phonon_gstreamer, Phonon::Gstreamer::Backend);

namespace Phonon
{
namespace Gstreamer
{

class MediaNode;

Backend::Backend(QObject *parent, const QVariantList &)
        : QObject(parent)
        , m_busIntervalTimer(0)
        , m_deviceManager(0)
        , m_effectManager(0)
        , m_debugLevel(Warning)
        , m_isValid(false)
{
    gst_init (0, 0);  //init gstreamer: must be called before any gst-related functions

    setProperty("identifier",     QLatin1String("phonon_gstreamer"));
    setProperty("backendName",    QLatin1String("Gstreamer"));
    setProperty("backendComment", QLatin1String("Gstreamer plugin for Phonon"));
    setProperty("backendVersion", QLatin1String("0.1"));
    setProperty("backendWebsite", QLatin1String("http://www.trolltech.com/"));

    //check if we should enable debug output
    QString debugLevelString = qgetenv("PHONON_GST_DEBUG");
    int debugLevel = debugLevelString.toInt();
    if (debugLevel > 3) //3 is maximum
        debugLevel = 3;
    m_debugLevel = (DebugLevel)debugLevel;

    m_isValid = checkDependencies();

    m_deviceManager = new DeviceManager(this);
    m_effectManager = new EffectManager(this);

    m_busIntervalTimer = new QTimer(this);
    m_busIntervalTimer->setInterval(100);
    connect(m_busIntervalTimer, SIGNAL(timeout()), this, SLOT(handleBusMessages()));
}

Backend::~Backend() {}


/***
 * !reimp
 */
QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
{
    // Return nothing if dependencies are not met

    switch (c) {
    case MediaObjectClass:
        return new MediaObject(this, parent);

    case AudioOutputClass: {
            AudioOutput *ao = new AudioOutput(this, parent);
            m_audioOutputs.append(ao);
            return ao;
        }
    case EffectClass:
        return new AudioEffect(this, args[0].toInt(), parent);

    case AudioDataOutputClass:
        logMessage("createObject() : AudioDataOutput not implemented");
        break;

    case VideoDataOutputClass:
        logMessage("createObject() : VideoDataOutput not implemented");
        break;

    case VideoWidgetClass: {
            QWidget *widget =  qobject_cast<QWidget*>(parent);
            return m_deviceManager->createVideoWidget(widget);
        }
    case VolumeFaderEffectClass: //Fall through
    case VisualizationClass:
    default:
        logMessage("createObject() : Backend object not available");
    }
    return 0;
}

// Returns true if all dependencies are met
// and gstreamer is usable, otherwise false
bool Backend::isValid() const
{
    return m_isValid;
}

bool Backend::supportsVideo() const
{
    return isValid();
}

bool Backend::checkDependencies() const
{
    bool success = false;
    // Verify that gst-plugins-base is installed
    GstElementFactory *acFactory = gst_element_factory_find ("audioconvert");
    if (acFactory) {
        gst_object_unref(acFactory);
        success = true;
        // Check if gst-plugins-good is installed
        GstElementFactory *csFactory = gst_element_factory_find ("videobalance");
        if (csFactory) {
            gst_object_unref(csFactory);
        } else {
            QString message = tr("Warning: You do not seem to have the package gstreamer0.10-plugins-good installed.\n"
                                 "          Some video features have been disabled.");
            qDebug() << message;
        }
    } else {
        qWarning() << tr("Warning: You do not seem to have the base GStreamer plugins installed.\n"
                         "          All audio and video support has been disabled");
    }
    return success;
}

/***
 * !reimp
 */
QStringList Backend::availableMimeTypes() const
{
    QStringList availableMineTypes;

    // I dont know a clean way to collect this info, so
    // we will have to check for well-known decoder factories
    if (isValid()) {
        GstElementFactory *theoraFactory = gst_element_factory_find ("theoradec");
        if (theoraFactory) {
            availableMineTypes  << QLatin1String("video/x-theora");
            gst_object_unref(GST_OBJECT(theoraFactory));
        }

        GstElementFactory *vorbisFactory = gst_element_factory_find ("vorbisdec");
        if (vorbisFactory) {
            availableMineTypes << QLatin1String("audio/x-vorbis");
            gst_object_unref(GST_OBJECT(vorbisFactory));
        }

        GstElementFactory *flacFactory = gst_element_factory_find ("flacdec");
        if (flacFactory) {
            availableMineTypes << QLatin1String("audio/x-flac");
            gst_object_unref(GST_OBJECT(flacFactory));
        }

        GstElementFactory *mpegFactory = gst_element_factory_find ("mpeg2dec");
        if (mpegFactory) {
            availableMineTypes << QLatin1String("video/x-mpeg");
            gst_object_unref(GST_OBJECT(mpegFactory));
        }
        if ((mpegFactory = gst_element_factory_find ("ffmpeg"))
                || (mpegFactory = gst_element_factory_find ("mad"))) {
            availableMineTypes << QLatin1String("audio/x-mpeg");
            gst_object_unref(GST_OBJECT(mpegFactory));
        }
    }

    return availableMineTypes;
}

/***
 * !reimp
 */
QList<int> Backend::objectDescriptionIndexes(ObjectDescriptionType type) const
{
    QList<int> list;

    if (!isValid())
        return list;

    switch (type) {
    case Phonon::AudioOutputDeviceType: {
            QList<AudioDevice> deviceList = deviceManager()->audioOutputDevices();
            for (int dev = 0 ; dev < deviceList.size() ; ++dev)
                list.append(deviceList[dev].id);
            break;
        }
        break;

    case Phonon::EffectType: {
            QList<AudioEffectInfo*> effectList = effectManager()->audioEffects();
            for (int eff = 0 ; eff < effectList.size() ; ++eff)
                list.append(eff);
            break;
        }
        break;
    }
    return list;
}

/***
 * !reimp
 */
QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{

    QHash<QByteArray, QVariant> ret;

    if (!isValid())
        return ret;

    switch (type) {
    case Phonon::AudioOutputDeviceType: {
            QList<AudioDevice> audioDevices = deviceManager()->audioOutputDevices();
            if (index >= 0 && index < audioDevices.size()) {
                ret.insert("name", audioDevices[index].gstId);
                ret.insert("description", audioDevices[index].gstId);
                ret.insert("icon", QLatin1String("audio-card"));
            }
        }
        break;

    case Phonon::EffectType: {
            QList<AudioEffectInfo*> effectList = effectManager()->audioEffects();
            if (index >= 0 && index <= effectList.size()) {
                const AudioEffectInfo *effect = effectList[index];
                ret.insert("name", effect->name());
                ret.insert("description", effect->description());
                ret.insert("author", effect->author());
            } else
                Q_ASSERT(1); // Since we use list position as ID, this should not happen
        }
    }
    return ret;
}

/***
 * !reimp
 */
bool Backend::startConnectionChange(QSet<QObject *> objects)
{
    Q_UNUSED(objects);
    return true;
}

/***
 * !reimp
 */
bool Backend::connectNodes(QObject *source, QObject *sink)
{
    if (isValid()) {
        MediaNode *sourceNode = qobject_cast<MediaNode *>(source);
        MediaNode *sinkNode = qobject_cast<MediaNode *>(sink);
        if (sourceNode && sinkNode) {
            if (sourceNode->connectNode(sink)) {
                logMessage(QString("Backend connected %0 to %1").arg(source->metaObject()->className()).arg(sink->metaObject()->className()));
                return true;
            }
        }
    }
    logMessage(QString("Linking %0 to %1 failed").arg(source->metaObject()->className()).arg(sink->metaObject()->className()), Warning);
    return false;
}

/***
 * !reimp
 */
bool Backend::disconnectNodes(QObject *source, QObject *sink)
{
    MediaNode *sourceNode = qobject_cast<MediaNode *>(source);
    MediaNode *sinkNode = qobject_cast<MediaNode *>(sink);

    if (sourceNode && sinkNode)
        return sourceNode->disconnectNode(sink);
    else
        return false;
}

/***
 * !reimp
 */
bool Backend::endConnectionChange(QSet<QObject *> objects)
{
    Q_UNUSED(objects);
    bool success = true;
    return success;
}

/***
 * Request bus messages for this mediaobject
 */
void Backend::addBusWatcher(MediaObject* node)
{
    m_busWatchers.append(node);
    if (m_busWatchers.size() == 1)
        m_busIntervalTimer->start();
}

/***
 * Ignore bus messages for this mediaobject
 */
void Backend::removeBusWatcher(MediaObject* node)
{
    m_busWatchers.removeAll(node);
    if (m_busWatchers.size() == 0)
        m_busIntervalTimer->stop();
}

/***
 * Polls each mediaobject's pipeline and delivers
 * pending any pending messages
 */
void Backend::handleBusMessages()
{
    // iterate over all the listening media nodes and deliver the message to each one
    GstMessage* message;
    QListIterator<MediaObject*> i(m_busWatchers);
    while (i.hasNext()) {
        MediaObject *mediaObject = i.next();
        GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (mediaObject->pipeline()));
        while ((message = gst_bus_poll(bus, GST_MESSAGE_ANY, 0)) != 0) {
            mediaObject->handleBusMessage(Message(message));
        }
    }
}

DeviceManager* Backend::deviceManager() const
{
    return m_deviceManager;
}

EffectManager* Backend::effectManager() const
{
    return m_effectManager;
}

/**
 * Returns a debuglevel that is determined by the
 * PHONON_GSTREAMER_DEBUG environment variable.
 *
 *  Warning - important warnings
 *  Info    - general info
 *  Debug   - gives extra info
 */
Backend::DebugLevel Backend::debugLevel() const
{
    return m_debugLevel;
}

/***
 * Prints a conditional debug message based on the current debug level
 * If obj is provided, classname and objectname will be printed as well
 *
 * see debugLevel()
 */
void Backend::logMessage(const QString &message, int priority, QObject *obj) const
{
    if (debugLevel() > 0) {
        QString output;
        if (obj)
            output = QString("%0 (%1 %2)").arg(message).arg(obj->objectName()).arg(obj->metaObject()->className());
        else
            output = message;

        if (priority <= (int)debugLevel()) {
            if (priority == Backend::Warning)
                qDebug() << "PHONON_GST(warn): " << output;
            else
                qDebug() << "PHONON_GST(info): " << output;
        }
    }
}

}
}

#include "moc_backend.cpp"
