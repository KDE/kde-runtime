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

#include "xvideowidget.h"

#ifndef Q_WS_QWS

#include <QtGui/QPalette>
#include <X11/Xlib.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/interfaces/propertyprobe.h>
#include "common.h"
#include "mediaobject.h"
#include "message.h"

namespace Phonon
{
namespace Gstreamer
{

XVideoWidget::XVideoWidget(Backend *backend, QWidget *parent)
        : AbstractVideoWidget(backend, parent)
{
    static int count = 0;
    m_name = "XVideoWidget" + QString::number(count++);

    m_renderWidget = new XWidget(this, parent);
    parent->winId();

    QPalette palette;
    palette.setColor(QPalette::Background, Qt::black);
    m_renderWidget->setPalette(palette);
    m_renderWidget->setAutoFillBackground(true);

    if ((m_videoSink = createVideoSink())) {
        setupVideoBin();
        m_isValid = true;
    }
}

GstElement* XVideoWidget::createVideoSink()
{
    GstElement *videoSink = gst_element_factory_make ("xvimagesink", NULL);
    if (!videoSink)
        videoSink = gst_element_factory_make ("ximagesink", NULL);

    return videoSink;
}

void XVideoWidget::mediaNodeEvent(const MediaNodeEvent *event)
{
    AbstractVideoWidget::mediaNodeEvent(event);

    switch (event->type()) {
    case MediaNodeEvent::VideoAvailable: {
            m_renderWidget->setAttribute(Qt::WA_NoSystemBackground, true);
            m_renderWidget->setAttribute(Qt::WA_PaintOnScreen, true);
        }
        break;

    case MediaNodeEvent::SourceChanged: {
            if (m_renderWidget->overlaySet()) {
                m_renderWidget->setOverlay();
            }
            // Setting these attributes ensures smooth video resizing without flicker
            m_renderWidget->setAttribute(Qt::WA_NoSystemBackground, false);
            m_renderWidget->setAttribute(Qt::WA_PaintOnScreen, false);
        }
        break;

    default:
        break;
    }
}

void XVideoWidget::setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio)
{
    AbstractVideoWidget::setAspectRatio(aspectRatio);
    GstElement *sink = videoSink();

    if (sink) {
        switch ( aspectRatio) {
        case Phonon::VideoWidget::AspectRatioAuto:
            g_object_set(G_OBJECT(sink), "force-aspect-ratio", true, NULL);
            break;

        case Phonon::VideoWidget::AspectRatioWidget:
            g_object_set(G_OBJECT(sink), "force-aspect-ratio", false, NULL);
            break;

        case Phonon::VideoWidget::AspectRatio4_3:
        case Phonon::VideoWidget::AspectRatio16_9:
            m_backend->logMessage("Unsupported aspect ratio", 1);

        default:
            break;
        }
    }
}

void XWidget::paintEvent(QPaintEvent *)
{
    if (!m_overlaySet)
        setOverlay();
    windowExposed();
}

void XWidget::setOverlay()
{
    GstElement *sink = m_controller->videoSink();
    if (sink && GST_IS_X_OVERLAY(sink))
        gst_x_overlay_set_xwindow_id ( GST_X_OVERLAY(sink) ,  winId() );
    windowExposed();
    m_overlaySet = true;
}

void XWidget::windowExposed()
{
    GstElement *sink = m_controller->videoSink();
    if (sink && GST_IS_X_OVERLAY(sink))
        gst_x_overlay_expose(GST_X_OVERLAY(sink));
}

QSize XWidget::sizeHint() const
{
    return m_controller->defaultSizeHint();
}

}
} //namespace Phonon::Gstreamer

#endif // Q_WS_QWS
