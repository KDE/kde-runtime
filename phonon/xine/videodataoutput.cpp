/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

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

#include "videodataoutput.h"
#include <kdebug.h>
#include "sourcenode.h"
#include <Phonon/Experimental/AbstractVideoDataOutput>

#define K_XT(type) (static_cast<type *>(SinkNode::threadSafeObject().data()))

namespace Phonon
{
namespace Xine
{
class VideoDataOutputXT : public SinkNodeXT
{
    public:
        VideoDataOutputXT();
        ~VideoDataOutputXT();
        xine_video_port_t *videoPort() const { return m_videoPort; }
        void rewireTo(SourceNodeXT *);

        Phonon::Experimental::AbstractVideoDataOutput *m_frontend;
    private:
        struct Frame
        {
            int format;
            int width;
            int height;
            double aspectRatio;
            void *data0;
            void *data1;
            void *data2;
        };
        static void raw_output_cb(void *user_data, int frame_format, int frame_width,
                int frame_height, double frame_aspect, void *data0, void *data1, void *data2);
        static void raw_overlay_cb(void *user_data, int num_ovl, raw_overlay_t *overlay_array);

#ifdef XINE_VISUAL_TYPE_RAW
        raw_visual_t m_visual;
#endif
        xine_video_port_t *m_videoPort;
};

void VideoDataOutputXT::raw_output_cb(void *user_data, int format, int width,
        int height, double aspect, void *data0, void *data1, void *data2)
{
    VideoDataOutputXT* vw = reinterpret_cast<VideoDataOutputXT *>(user_data);
    const Experimental::VideoFrame f = {
        width,
        height,
        aspect,
        ((format == XINE_VORAW_YV12) ? Experimental::VideoFrame::Format_YV12 :
         (format == XINE_VORAW_YUY2) ? Experimental::VideoFrame::Format_YUY2 :
         (format == XINE_VORAW_RGB ) ? Experimental::VideoFrame::Format_RGB888 :
                                       Experimental::VideoFrame::Format_Invalid),
        QByteArray::fromRawData(reinterpret_cast<const char *>(data0), ((format == XINE_VORAW_RGB) ? 3 : (format == XINE_VORAW_YUY2) ? 2 : 1) * width * height),
        QByteArray::fromRawData(reinterpret_cast<const char *>(data1), (format == XINE_VORAW_YV12) ? (width >> 1) + (height >> 1) : 0),
        QByteArray::fromRawData(reinterpret_cast<const char *>(data2), (format == XINE_VORAW_YV12) ? (width >> 1) + (height >> 1) : 0)
    };
    if (vw->m_frontend) {
        //kDebug(610) << "send frame to frontend";
        vw->m_frontend->frameReady(f);
    }
}

void VideoDataOutputXT::raw_overlay_cb(void *user_data, int num_ovl, raw_overlay_t *overlay_array)
{
    VideoDataOutputXT* vw = reinterpret_cast<VideoDataOutputXT *>(user_data);
    Q_UNUSED(vw);
    Q_UNUSED(num_ovl);
    Q_UNUSED(overlay_array);
}

VideoDataOutputXT::VideoDataOutputXT()
    : m_frontend(0),
    m_videoPort(0)
{
#ifdef XINE_VISUAL_TYPE_RAW
    m_visual.user_data = static_cast<void *>(this);
    m_visual.raw_output_cb = &Phonon::Xine::VideoDataOutputXT::raw_output_cb;
    m_visual.raw_overlay_cb = &Phonon::Xine::VideoDataOutputXT::raw_overlay_cb;
    m_visual.supported_formats = /*XINE_VORAW_YV12 | XINE_VORAW_YUY2 |*/ XINE_VORAW_RGB; // TODO
    m_videoPort = xine_open_video_driver(XineEngine::xine(), "auto", XINE_VISUAL_TYPE_RAW, static_cast<void *>(&m_visual));
#endif
}

VideoDataOutputXT::~VideoDataOutputXT()
{
    if (m_videoPort) {
        xine_video_port_t *vp = m_videoPort;
        m_videoPort = 0;

        xine_close_video_driver(XineEngine::xine(), vp);
    }
}

VideoDataOutput::VideoDataOutput(QObject *parent)
    : QObject(parent),
    SinkNode(new VideoDataOutputXT)
{
}

VideoDataOutput::~VideoDataOutput()
{
}

void VideoDataOutputXT::rewireTo(SourceNodeXT *source)
{
    if (!source->videoOutputPort()) {
        return;
    }
    xine_post_wire_video_port(source->videoOutputPort(), videoPort());
}

Experimental::AbstractVideoDataOutput *VideoDataOutput::frontendObject() const
{
    return K_XT(const VideoDataOutputXT)->m_frontend;
}

void VideoDataOutput::setFrontendObject(Experimental::AbstractVideoDataOutput *x)
{
    K_XT(VideoDataOutputXT)->m_frontend = x;
}

}} //namespace Phonon::Xine

#undef K_XT

#include "videodataoutput.moc"
