/*  This file is part of the KDE project
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "videowidget.h"
#include "events.h"
#include <QPalette>
#include <QImage>
#include <QPainter>
#include <kdebug.h>

#ifndef PHONON_XINE_NO_VIDEOWIDGET
#include <QX11Info>
#endif
#include "xineengine.h"
#include "mediaobject.h"
#include <QApplication>
#include <QVBoxLayout>

#include <QDesktopWidget>
#include <QMouseEvent>

namespace Phonon
{
namespace Xine
{
#define K_XT(type) (static_cast<type *>(SinkNode::threadSafeObject.data()))

#ifndef PHONON_XINE_NO_VIDEOWIDGET
static void dest_size_cb( void* user_data, int video_width, int video_height, double video_pixel_aspect,
		int *dest_width, int *dest_height, double *dest_pixel_aspect )
{
	Phonon::Xine::VideoWidget* vw = static_cast<VideoWidget*>( user_data );
	int win_x, win_y;

	vw->xineCallback( win_x, win_y, *dest_width, *dest_height, *dest_pixel_aspect, video_width,
			video_height, video_pixel_aspect, false );
}

static void frame_output_cb( void* user_data, int video_width, int video_height,
		double video_pixel_aspect, int *dest_x, int *dest_y,
		int *dest_width, int *dest_height,
		double *dest_pixel_aspect, int *win_x, int *win_y )
{
	Phonon::Xine::VideoWidget* vw = static_cast<VideoWidget*>( user_data );

	vw->xineCallback( *win_x, *win_y, *dest_width, *dest_height, *dest_pixel_aspect, video_width,
			video_height, video_pixel_aspect, true );

	*dest_x            = 0;
	*dest_y            = 0;
}
#endif // PHONON_XINE_NO_VIDEOWIDGET

void VideoWidget::xineCallback( int &x, int &y, int &width, int &height, double &ratio,
		int videoWidth, int videoHeight, double videoRatio, bool mayResize )
{
	Q_UNUSED( videoRatio );
	Q_UNUSED( videoWidth );
	Q_UNUSED( videoHeight );
	Q_UNUSED( mayResize );

	x = this->x();
	y = this->y();
	width = this->width();
	height = this->height();

	// square pixels
	ratio = 1.0;
}

VideoWidgetXT::VideoWidgetXT(QWidget *w)
    : m_videoPort(0)
{
#ifndef PHONON_XINE_NO_VIDEOWIDGET
    // make a new XCB connection for xine
    int preferredScreen = 0;
    m_xcbConnection = xcb_connect(NULL, &preferredScreen);//DisplayString(x11Info().display()), NULL);
    if (m_xcbConnection) {
        m_visual.connection = m_xcbConnection;
        xcb_screen_iterator_t screenIt = xcb_setup_roots_iterator(xcb_get_setup(m_xcbConnection));
        while ((screenIt.rem > 1) && (preferredScreen > 0)) {
            xcb_screen_next(&screenIt);
            --preferredScreen;
        }
        m_visual.screen = screenIt.data;
        m_visual.window = w->winId();
        m_visual.user_data = static_cast<void *>(w);
        m_visual.dest_size_cb = Phonon::Xine::dest_size_cb;
        m_visual.frame_output_cb = Phonon::Xine::frame_output_cb;

        // make sure all Qt<->X communication is done, else xine_open_video_driver will crash
        QApplication::syncX();

        Q_ASSERT(w->testAttribute(Qt::WA_WState_Created));
        m_videoPort = xine_open_video_driver(XineEngine::xine(), "auto", XINE_VISUAL_TYPE_XCB, static_cast<void*>(&m_visual));
        if (!m_videoPort) {
#endif // PHONON_XINE_NO_VIDEOWIDGET
            kError(610) << "No xine video output plugin using libxcb for threadsafe access to the X server found. No video for you." << endl;
#ifndef PHONON_XINE_NO_VIDEOWIDGET
        }
    }
#endif // PHONON_XINE_NO_VIDEOWIDGET
}

VideoWidget::VideoWidget( QWidget* parent )
    : QWidget(parent),
    SinkNode(new VideoWidgetXT(this)),
    m_fullScreen(false),
    m_empty(true),
    m_brightness(0.0),
    m_contrast(0.0),
    m_hue(0.0),
    m_saturation(0.0)
{
	// for some reason it can hang if the widget is 0x0
	setMinimumSize( 1, 1 );

	QPalette palette = this->palette();
	palette.setColor( backgroundRole(), Qt::black );
	setPalette( palette );

	// when resizing fill with black (backgroundRole color) the rest is done by paintEvent
	setAttribute( Qt::WA_OpaquePaintEvent, true );

	// disable Qt composition management as Xine draws onto the widget directly using X calls
	setAttribute( Qt::WA_PaintOnScreen, true );

    // required for dvdnav
    setMouseTracking(true);
}

VideoWidget::~VideoWidget()
{
    // tell the xine stream to stop using this videoPort
    MediaObject *mo = findMediaObject();
    if (mo) {
        XineStream &xs = mo->stream();
        xs.aboutToDeleteVideoWidget();
    } else {
        kDebug(610) << k_funcinfo << "no MediaObject, hoping that nobody still needs the video port" << endl;
    }
}

VideoWidgetXT::~VideoWidgetXT()
{
    if (m_videoPort) {
        xine_port_send_gui_data(m_videoPort, XINE_GUI_SEND_WILL_DESTROY_DRAWABLE, 0);

        xine_video_port_t *vp = m_videoPort;
        m_videoPort = 0;

        xine_close_video_driver(XineEngine::xine(), vp);
    }

#ifndef PHONON_XINE_NO_VIDEOWIDGET
    xcb_disconnect(m_xcbConnection);
    m_xcbConnection = 0;
#endif // PHONON_XINE_NO_VIDEOWIDGET
}

Phonon::VideoWidget::AspectRatio VideoWidget::aspectRatio() const
{
	return m_aspectRatio;
}

void VideoWidget::setAspectRatio( Phonon::VideoWidget::AspectRatio aspectRatio )
{
    m_aspectRatio = aspectRatio;
    MediaObject *mo = findMediaObject();
    if (mo) {
        XineStream &xs = mo->stream();
        switch (m_aspectRatio) {
            case Phonon::VideoWidget::AspectRatioWidget:
                xs.setParam(XINE_PARAM_VO_ASPECT_RATIO, XINE_VO_ASPECT_SQUARE);
                break;
            case Phonon::VideoWidget::AspectRatioAuto:
                xs.setParam(XINE_PARAM_VO_ASPECT_RATIO, XINE_VO_ASPECT_AUTO);
                break;
//X             case Phonon::VideoWidget::AspectRatioSquare:
//X                 xs.setParam(XINE_PARAM_VO_ASPECT_RATIO, XINE_VO_ASPECT_SQUARE);
//X                 break;
            case Phonon::VideoWidget::AspectRatio4_3:
                xs.setParam(XINE_PARAM_VO_ASPECT_RATIO, XINE_VO_ASPECT_4_3);
                break;
            case Phonon::VideoWidget::AspectRatio16_9:
                xs.setParam(XINE_PARAM_VO_ASPECT_RATIO, XINE_VO_ASPECT_ANAMORPHIC);
                break;
//X             case Phonon::VideoWidget::AspectRatioDvb:
//X                 xs.setParam(XINE_PARAM_VO_ASPECT_RATIO, XINE_VO_ASPECT_DVB);
//X                 break;
        }
        updateZoom();
    }
}

Phonon::VideoWidget::ScaleMode VideoWidget::scaleMode() const
{
    return m_scaleMode;
}

void VideoWidget::setScaleMode(Phonon::VideoWidget::ScaleMode mode)
{
    m_scaleMode = mode;
    updateZoom();
}

qreal VideoWidget::brightness() const
{
    return m_brightness;
}

void VideoWidget::setBrightness(qreal newBrightness)
{
    newBrightness = qBound(-1.0, newBrightness, 1.0);
    if (m_brightness != newBrightness) {
        m_brightness = newBrightness;
        MediaObject *mo = findMediaObject();
        if (mo) {
            XineStream &xs = mo->stream();
            xs.setParam(XINE_PARAM_VO_BRIGHTNESS, static_cast<int>(0x7fff * (m_brightness + 1.0)));
        }
    }
}

qreal VideoWidget::contrast() const
{
    return m_contrast;
}

void VideoWidget::setContrast(qreal newContrast)
{
    newContrast = qBound(-1.0, newContrast, 1.0);
    if (m_contrast != newContrast) {
        m_contrast = newContrast;
    }
}

qreal VideoWidget::hue() const
{
    return m_hue;
}

void VideoWidget::setHue(qreal newHue)
{
    newHue = qBound(-1.0, newHue, 1.0);
    if (m_hue != newHue) {
        m_hue = newHue;
    }
}

qreal VideoWidget::saturation() const
{
    return m_saturation;
}

void VideoWidget::setSaturation(qreal newSaturation)
{
    newSaturation = qBound(-1.0, newSaturation, 1.0);
    if (m_saturation != newSaturation) {
        m_saturation = newSaturation;
    }
}

/*
int VideoWidget::overlayCapabilities() const
{
	return Phonon::Experimental::OverlayApi::OverlayOpaque;
}

bool VideoWidget::createOverlay(QWidget *widget, int type)
{
	if ((overlay != 0) || (type != Phonon::Experimental::OverlayApi::OverlayOpaque))
		return false;

	if (layout() == 0) {
		QLayout *layout = new QVBoxLayout(this);
		layout->setMargin(0);
		setLayout(layout);
	}

	layout()->addWidget(widget);
	overlay = widget;

	return true;
}

void VideoWidget::childEvent(QChildEvent *event)
{
	if (event->removed() && (event->child() == overlay))
		overlay = 0;
	QWidget::childEvent(event);
}
*/

void VideoWidget::updateZoom()
{
    MediaObject *mo = findMediaObject();
    if (mo) {
        XineStream &xs = mo->stream();
        if (m_aspectRatio == Phonon::VideoWidget::AspectRatioWidget) {
            const QSize s = size();
            QSize imageSize = m_sizeHint;
            imageSize.scale(s, Qt::KeepAspectRatio);
            if (imageSize.width() < s.width()) {
                const int zoom = s.width() * 100 / imageSize.width();
                xs.setParam(XINE_PARAM_VO_ZOOM_X, zoom);
                xs.setParam(XINE_PARAM_VO_ZOOM_Y, 100);
            } else {
                const int zoom = s.height() * 100 / imageSize.height();
                xs.setParam(XINE_PARAM_VO_ZOOM_X, 100);
                xs.setParam(XINE_PARAM_VO_ZOOM_Y, zoom);
            }
        } else if (m_scaleMode == Phonon::VideoWidget::ScaleAndCrop) {
            const QSize s = size();
            QSize imageSize = m_sizeHint;
            // the image size is in square pixels
            // first transform it to the current aspect ratio
            kDebug(610) << imageSize;
            switch (m_aspectRatio) {
                case Phonon::VideoWidget::AspectRatioAuto:
                    // FIXME: how can we find out the ratio xine decided on? the event?
                    break;
                case Phonon::VideoWidget::AspectRatio4_3:
                    imageSize.setWidth(imageSize.height() * 4 / 3);
                    break;
                case Phonon::VideoWidget::AspectRatio16_9:
                    imageSize.setWidth(imageSize.height() * 16 / 9);
                    break;
//X                 case Phonon::VideoWidget::AspectRatioDvb:
//X                     imageSize.setWidth(imageSize.height() * 2);
//X                     break;
                default:
                    // correct ratio already
                    break;
            }
            kDebug(610) << imageSize;
            imageSize.scale(s, Qt::KeepAspectRatioByExpanding);
            kDebug(610) << imageSize << s;
            int zoom;
            if (imageSize.width() > s.width()) {
                zoom = imageSize.width() * 100 / s.width();
            } else {
                zoom = imageSize.height() * 100 / s.height();
            }
            xs.setParam(XINE_PARAM_VO_ZOOM_X, zoom);
            xs.setParam(XINE_PARAM_VO_ZOOM_Y, zoom);
        } else {
            xs.setParam(XINE_PARAM_VO_ZOOM_X, 100);
            xs.setParam(XINE_PARAM_VO_ZOOM_Y, 100);
        }
    }
}

void VideoWidget::resizeEvent(QResizeEvent *ev)
{
    updateZoom();
    QWidget::resizeEvent(ev);
}

bool VideoWidget::event(QEvent *ev)
{
    switch (ev->type()) {
        case Events::NavButtonIn:
            setCursor(QCursor(Qt::PointingHandCursor));
            ev->accept();
            return true;
        case Events::NavButtonOut:
            unsetCursor();
            ev->accept();
            return true;
        case Events::FrameFormatChange:
            ev->accept();
            {
                XineFrameFormatChangeEvent *e = static_cast<XineFrameFormatChangeEvent *>(ev);
                kDebug(610) << k_funcinfo << "XineFrameFormatChangeEvent " << e->size;
                m_sizeHint = e->size;
                updateGeometry();
            }
            return true;
        default:
            return QWidget::event(ev);
    }
}

void VideoWidget::mouseMoveEvent(QMouseEvent *mev)
{
    MediaObject *mo = findMediaObject();
    if (mo) {
        XineStream &xs = mo->stream();
        if (cursor().shape() == Qt::BlankCursor) {
            setCursor(QCursor(Qt::ArrowCursor));
        }

        x11_rectangle_t   rect;
        xine_event_t      *event = new xine_event_t;
        xine_input_data_t *input = new xine_input_data_t;

        rect.x = mev->x();
        rect.y = mev->y();
        rect.w = 0;
        rect.h = 0;

        xine_port_send_gui_data(K_XT(VideoWidgetXT)->m_videoPort, XINE_GUI_SEND_TRANSLATE_GUI_TO_VIDEO, (void*)&rect);

        event->type        = XINE_EVENT_INPUT_MOUSE_MOVE;
        event->data        = input;
        event->data_length = sizeof(*input);
        input->button      = 0;
        input->x           = rect.x;
        input->y           = rect.y;
        xs.eventSend(event);
    }
    QWidget::mouseMoveEvent(mev);
}

void VideoWidget::mousePressEvent(QMouseEvent *mev)
{
    MediaObject *mo = findMediaObject();
    if (mev->button() == Qt::LeftButton && mo) {
        XineStream &xs = mo->stream();
        x11_rectangle_t   rect;
        xine_event_t      *event = new xine_event_t;
        xine_input_data_t *input = new xine_input_data_t;

        rect.x = mev->x();
        rect.y = mev->y();
        rect.w = 0;
        rect.h = 0;

        xine_port_send_gui_data(K_XT(VideoWidgetXT)->m_videoPort, XINE_GUI_SEND_TRANSLATE_GUI_TO_VIDEO, (void*)&rect);

        event->type        = XINE_EVENT_INPUT_MOUSE_BUTTON;
        event->data        = input;
        event->data_length = sizeof(*input);
        input->button      = 1;
        input->x           = rect.x;
        input->y           = rect.y;
        xs.eventSend(event);
    }
    QWidget::mousePressEvent(mev);
}

bool VideoWidget::isValid() const
{
    return K_XT(const VideoWidgetXT)->m_videoPort != 0;
}

void VideoWidget::setVideoEmpty(bool b)
{
    m_empty = b;
    if (b) {
        update();
    }
}

xine_video_port_t *VideoWidgetXT::videoPort() const
{
    return m_videoPort;
}

void VideoWidgetXT::rewireTo(SourceNodeXT *source)
{
    if (!source->videoOutputPort()) {
        return;
    }
    xine_post_wire_video_port(source->videoOutputPort(), videoPort());
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    //kDebug(610) << k_funcinfo << "m_empty = " << m_empty;
    if (m_empty || !source()) {// || m_path->mediaObject()->state() == Phonon::LoadingState) {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
#ifndef PHONON_XINE_NO_VIDEOWIDGET
    } else if (K_XT(VideoWidgetXT)->m_videoPort) {
        const QRect &rect = event->rect();

        xcb_expose_event_t xcb_event;
        memset(&xcb_event, 0, sizeof(xcb_event));

        xcb_event.window = winId();
        xcb_event.x = rect.x();
        xcb_event.y = rect.y();
        xcb_event.width = rect.width();
        xcb_event.height = rect.height();
        xcb_event.count = 0;

        xine_port_send_gui_data(K_XT(VideoWidgetXT)->m_videoPort, XINE_GUI_SEND_EXPOSE_EVENT, &xcb_event);
#endif // PHONON_XINE_NO_VIDEOWIDGET
    } else {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
    }
    QWidget::paintEvent(event);
}

void VideoWidget::showEvent( QShowEvent* )
{
	//xine_port_send_gui_data( K_XT(VideoWidgetXT)->m_videoPort, XINE_GUI_SEND_VIDEOWIN_VISIBLE, static_cast<void*>( 1 ) );
}

void VideoWidget::hideEvent( QHideEvent* )
{
	//xine_port_send_gui_data( K_XT(VideoWidgetXT)->m_videoPort, XINE_GUI_SEND_VIDEOWIN_VISIBLE, static_cast<void*>( 0 ) );
}

void VideoWidget::changeEvent( QEvent* event )
{
	if( event->type() == QEvent::ParentAboutToChange )
	{
		kDebug( 610 ) << k_funcinfo << "ParentAboutToChange";
	}
	else if( event->type() == QEvent::ParentChange )
	{
        kDebug(610) << k_funcinfo << "ParentChange" << winId();
#ifndef PHONON_XINE_NO_VIDEOWIDGET
        if (K_XT(VideoWidgetXT)->m_visual.window != winId()) {
            K_XT(VideoWidgetXT)->m_visual.window = winId();
            if (K_XT(VideoWidgetXT)->m_videoPort) {
                // make sure all Qt<->X communication is done, else winId() might not be known at the
                // X-server yet
                QApplication::syncX();
                xine_port_send_gui_data(K_XT(VideoWidgetXT)->m_videoPort, XINE_GUI_SEND_DRAWABLE_CHANGED, reinterpret_cast<void*>(K_XT(VideoWidgetXT)->m_visual.window));
                kDebug(610) << "XINE_GUI_SEND_DRAWABLE_CHANGED done.";
            }
        }
#endif // PHONON_XINE_NO_VIDEOWIDGET
    }
}

MediaObject *VideoWidget::findMediaObject() const
{
    SourceNode *s = source();
    while (s) {
        MediaObject *mo = s->mediaObjectInterface();
        if (mo) {
            return mo;
        }
        SinkNode *ss = s->sinkInterface();
        if (!ss) {
            return 0;
        }
        s = ss->source();
    }
    return 0;
}

#undef K_XT
}} //namespace Phonon::Xine

#include "videowidget.moc"
// vim: sw=4 ts=4
