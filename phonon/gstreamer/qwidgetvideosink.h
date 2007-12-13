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

#ifndef __GSTREAMER_VIDEOSINK_H
#define __GSTREAMER_VIDEOSINK_H

#include <gst/video/gstvideosink.h>

class NewFrameEvent : public QEvent
{
public:
    NewFrameEvent(const QByteArray &newFrame, int w, int h) :
        QEvent(QEvent::User),
        frame(newFrame),
        width(w),
        height(h)
    {
    }

    QByteArray frame;
    int width;
    int height;
};

namespace Phonon
{
namespace Gstreamer
{

enum VideoFormat {
    VideoFormat_YUV,
    VideoFormat_RGB
};

class QWidgetVideoSinkBase
{
public:
    GstVideoSink    videoSink;

    VideoWidget*    control;
    gint            width;
    gint            height;
    gint            bpp;
    gint            depth;
};

template <VideoFormat FMT>
class QWidgetVideoSink : public QWidgetVideoSinkBase
{
public:
    static GstCaps* get_caps(GstBaseSink* sink);
    static gboolean set_caps(GstBaseSink* sink, GstCaps* caps);
    static GstStateChangeReturn change_state(GstElement* element, GstStateChange transition);
    static GstFlowReturn render(GstBaseSink* sink, GstBuffer* buf);
    static void base_init(gpointer g_class);
    static void instance_init(GTypeInstance *instance, gpointer g_class);
};

template <VideoFormat FMT>
struct QWidgetVideoSinkClass
{
    GstVideoSinkClass   parent_class;
    static void class_init(gpointer g_class, gpointer class_data);
    static GType get_type();
};

GType get_type_YUV();
GType get_type_RGB();

}
} //namespace Phonon::Gstreamer

#endif  // __GSTREAMER_VIDEOSINK_H
