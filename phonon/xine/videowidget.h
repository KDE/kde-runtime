/*  This file is part of the KDE project
    Copyright (C) 2005-2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#ifndef PHONON_XINE_VIDEOWIDGET_H
#define PHONON_XINE_VIDEOWIDGET_H

#include <QWidget>
#include "sinknode.h"
#include <QPixmap>
#include <xine.h>

#ifndef PHONON_XINE_NO_VIDEOWIDGET
#include <xcb/xcb.h>
#endif // PHONON_XINE_NO_VIDEOWIDGET

#include <Phonon/VideoWidget>
#include <Phonon/VideoWidgetInterface>
#include "connectnotificationinterface.h"

class QMouseEvent;

namespace Phonon
{
namespace Xine
{
class VideoWidget;

class VideoWidgetXT : public SinkNodeXT
{
    friend class VideoWidget;
    public:
        VideoWidgetXT(VideoWidget *);
        ~VideoWidgetXT();
        void rewireTo(SourceNodeXT *);

        VideoWidget *videoWidget() const { return m_videoWidget; }
        xine_video_port_t *videoPort() const;
    private:
#ifndef PHONON_XINE_NO_VIDEOWIDGET
        xcb_visual_t m_visual;
        xcb_connection_t *m_xcbConnection;
#endif // PHONON_XINE_NO_VIDEOWIDGET
        xine_video_port_t *m_videoPort;
        VideoWidget *m_videoWidget;
};

class VideoWidget : public QWidget, public Phonon::VideoWidgetInterface, public Phonon::Xine::SinkNode, public ConnectNotificationInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoWidgetInterface Phonon::Xine::SinkNode Phonon::Xine::ConnectNotificationInterface)
    public:
        VideoWidget(QWidget *parent = 0);
        ~VideoWidget();

        Phonon::VideoWidget::AspectRatio aspectRatio() const;
        void setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio);
        Phonon::VideoWidget::ScaleMode scaleMode() const;
        void setScaleMode(Phonon::VideoWidget::ScaleMode mode);

        QWidget *widget() { return this; }

        qreal brightness() const;
        void  setBrightness(qreal);

        qreal contrast() const;
        void  setContrast(qreal);

        qreal hue() const;
        void  setHue(qreal);

        qreal saturation() const;
        void  setSaturation(qreal);

        void xineCallback(int &x, int &y, int &width, int &height,
                double &ratio, int videoWidth, int videoHeight, double videoRatio, bool mayResize);

        bool isValid() const;

        MediaStreamTypes inputMediaStreamTypes() const { return Phonon::Xine::Video | Phonon::Xine::Subtitle; }
        void downstreamEvent(Event *e);

        void graphChanged();

    signals:
        void videoPortChanged();

    protected:
        //virtual void childEvent(QChildEvent *);
        virtual void resizeEvent(QResizeEvent *);
        virtual bool event(QEvent *);
        virtual void mouseMoveEvent(QMouseEvent *);
        virtual void mousePressEvent(QMouseEvent *);
        virtual void showEvent(QShowEvent *);
        virtual void hideEvent(QHideEvent *);
        virtual void paintEvent(QPaintEvent *);
        virtual void changeEvent(QEvent *);
        virtual QSize sizeHint() const { return m_sizeHint; }

    private:
        void updateZoom();
        Phonon::VideoWidget::AspectRatio m_aspectRatio;
        Phonon::VideoWidget::ScaleMode m_scaleMode;

        QSize m_sizeHint;
        int m_videoWidth;
        int m_videoHeight;
        bool m_fullScreen;
        /**
         * No video should be shown, all paint events should draw black
         */
        bool m_empty;

        qreal m_brightness;
        qreal m_contrast;
        qreal m_hue;
        qreal m_saturation;
};

}} //namespace Phonon::Xine

// vim: sw=4 ts=4 tw=80
#endif // PHONON_XINE_VIDEOWIDGET_H
