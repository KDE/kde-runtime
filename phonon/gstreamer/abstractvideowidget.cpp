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

#include "abstractvideowidget.h"
#include <QtCore/QEvent>
#include <QtGui/QPalette>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>
#include <QApplication>
#include <X11/Xlib.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/interfaces/propertyprobe.h>
#include "mediaobject.h"
#include "message.h"
#include "common.h"

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace Gstreamer
{

AbstractVideoWidget::AbstractVideoWidget(Backend *backend, QWidget *parent) :
        QObject(parent),
        MediaNode(backend, VideoSink),
        m_videoBin(0),
        m_videoSink(0),
        m_aspectRatio(Phonon::VideoWidget::AspectRatioAuto),
        m_brightness(0.0),
        m_hue(0.0),
        m_contrast(1.0),
        m_saturation(1.0),
        m_videoBalance(0),
        m_colorspace(0) {}

AbstractVideoWidget::~AbstractVideoWidget()
{
    if (m_videoBin) {
        gst_element_set_state (m_videoBin, GST_STATE_NULL);
        gst_object_unref (m_videoBin);
    }
}

void AbstractVideoWidget::setupVideoBin()
{
    //Video sink must be created before we can connect the bin
    Q_ASSERT(m_videoSink);

    if (!m_videoSink) {
        qWarning("Error setting up video output device. Video output not available!");
        m_videoSink = gst_element_factory_make ("fakesink", NULL);
        g_object_set (G_OBJECT (m_videoSink), "sync", TRUE, NULL);
    }

    m_videoBin = gst_bin_new (NULL);
    Q_ASSERT(m_videoBin);
    gst_object_ref (m_videoBin); // keep alive


    GstElement *queue = gst_element_factory_make ("queue", NULL);

    //Colorspace ensures that the output of the stream matches the input format accepted by our video sink
    m_colorspace = gst_element_factory_make ("ffmpegcolorspace", NULL);

    //Video scale is used to prepare the correct aspect ratio of the movie.
    //m_videoScale = gst_element_factory_make ("videoscale", NULL);

    if (queue && m_videoBin && m_colorspace && m_videoSink) {
        //Ensure that the bare essentials are prepared
        gst_bin_add_many (GST_BIN (m_videoBin), queue, m_colorspace, m_videoSink, NULL);
        if (gst_element_link(queue, m_colorspace)) {
            bool success = false;
            //Video balance controls color/sat/hue in the YUV colorspace
            m_videoBalance = gst_element_factory_make ("videobalance", NULL);
            if (m_videoBalance) {
                // For video balance to work we have to first ensure that the video is in YUV colorspace,
                // then hand it off to the videobalance filter before finally converting it back to RGB.
                // Hence we nede a videoFilter to convert the colorspace before and after videobalance
                GstElement *m_colorspace2 = gst_element_factory_make ("ffmpegcolorspace", NULL);
                gst_bin_add_many(GST_BIN(m_videoBin), m_videoBalance, m_colorspace2, NULL);
                success = gst_element_link_many(m_colorspace, m_videoBalance, m_colorspace2, m_videoSink, NULL);
            } else {
                //If video balance is not available, just connect scale to sink directly
                success = gst_element_link(m_colorspace, m_videoSink);
            }

            if (success) {
                GstPad *videopad = gst_element_get_pad (queue, "sink");
                gst_element_add_pad (m_videoBin, gst_ghost_pad_new ("sink", videopad));
                gst_object_unref (videopad);
                if (widget()->parentWidget())
                    widget()->parentWidget()->winId();// Due to some existing issues with alien in 4.4, 
                                                     //  we must currently force the creation of a parent widget.
                m_isValid = true; //initialization ok, accept input
            }
        }
    }
}

Phonon::VideoWidget::AspectRatio AbstractVideoWidget::aspectRatio() const
{
    return m_aspectRatio;
}

QSize AbstractVideoWidget::defaultSizeHint() const
{
    if (!m_movieSize.isEmpty())
        return m_movieSize;
    else
        return QSize(640, 480);
}

void AbstractVideoWidget::setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio)
{
    m_aspectRatio = aspectRatio;
}

Phonon::VideoWidget::ScaleMode AbstractVideoWidget::scaleMode() const
{
    return m_scaleMode;
}

void AbstractVideoWidget::setScaleMode(Phonon::VideoWidget::ScaleMode scaleMode)
{
    m_scaleMode = scaleMode;
}

qreal AbstractVideoWidget::brightness() const
{
    return m_brightness;
}

qreal clampedValue(qreal val)
{
    if (val > 1.0 )
        return 1.0;
    else if (val < -1.0)
        return -1.0;
    else return val;
}

void AbstractVideoWidget::setBrightness(qreal newValue)
{
    newValue = clampedValue(newValue);

    if (newValue == m_brightness)
        return;

    m_brightness = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "brightness", newValue, NULL); //gstreamer range is [-1, 1]

}

qreal AbstractVideoWidget::contrast() const
{
    return m_contrast;
}

void AbstractVideoWidget::setContrast(qreal newValue)
{
    newValue = clampedValue(newValue);

    if (newValue == m_contrast)
        return;

    m_contrast = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "contrast", (newValue + 1.0), NULL); //gstreamer range is [0-2]
}

qreal AbstractVideoWidget::hue() const
{
    return m_hue;
}

void AbstractVideoWidget::setHue(qreal newValue)
{
    if (newValue == m_hue)
        return;

    newValue = clampedValue(newValue);

    m_hue = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "hue", newValue, NULL); //gstreamer range is [-1, 1]
}

qreal AbstractVideoWidget::saturation() const
{
    return m_saturation;
}

void AbstractVideoWidget::setSaturation(qreal newValue)
{
    newValue = clampedValue(newValue);

    if (newValue == m_saturation)
        return;

    m_saturation = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "saturation", newValue + 1.0, NULL); //gstreamer range is [0, 2]
}


void AbstractVideoWidget::setMovieSize(const QSize &size)
{
    m_backend->logMessage(QString("New video size %0 x %1").arg(size.width()).arg(size.height()), Backend::Info);
    if (size == m_movieSize)
        return;
    m_movieSize = size;
    widget()->updateGeometry();
    widget()->update();
}

void AbstractVideoWidget::mediaNodeEvent(const MediaNodeEvent *event)
{
    switch (event->type()) {
    case MediaNodeEvent::VideoSizeChanged: {
            const QSize *size = static_cast<const QSize*>(event->data());
            setMovieSize(*size);
        }
        break;
    default:
        break;
    }
}

}
} //namespace Phonon::Gstreamer

QT_END_NAMESPACE

#include "moc_abstractvideowidget.cpp"
