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

#ifndef Phonon_QT7_VIDEOWIDGET_H
#define Phonon_QT7_VIDEOWIDGET_H

#include <QtGui>
#include <QGLWidget>
#include <QGLContext>
#include <QtOpenGL/private/qgl_p.h>
#include <QPointer>
#include <QMutex>

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include <phonon/videowidgetinterface.h>
#include "medianode.h"

namespace Phonon
{
namespace QT7
{
    class MediaNodeEvent;

    class VideoRenderWidget : public QGLWidget
    {

    public:
        VideoRenderWidget(QWidget *parent, const QGLFormat &format);
        virtual ~VideoRenderWidget();
        void setVideoFrame(VideoFrame &frame);
        void updateVideoFrame();

        static AGLContext sharedOpenGLContext();
        static AGLPixelFormat sharedOpenGLPixelFormat();

    protected:
        bool event(QEvent *event);
        void initializeGL();
        void resizeGL(int w, int h);
        void paintGL();
        QSize sizeHint() const;

    private:
        friend class VideoWidget;
        void setMovieRect(const QRect &mrect);
        void calculateDrawFrameRect();
        QRect scaleToAspect(QRect srcRect, int w, int h);

        VideoFrame m_currentFrame;
        QRect m_movieFrameRect;
        QRect m_drawFrameRect;
        Phonon::VideoWidget::ScaleMode m_scaleMode;
        Phonon::VideoWidget::AspectRatio m_aspect;
        QMutex m_glContextMutex;

        qreal m_brightness;
        qreal m_contrast;
        qreal m_hue;
        qreal m_saturation;

        static QGLWidget *m_sharedQGLWidget;
        static QGLWidget *sharedQGLWidget();
    };

/////////////////////////////////////////////////////////////////////////////////////////

    class VideoWidget : public MediaNode, public Phonon::VideoWidgetInterface
    {
        Q_OBJECT
        Q_INTERFACES(Phonon::VideoWidgetInterface)

     public:
        VideoWidget(QObject *parent);
        virtual ~VideoWidget();

        Phonon::VideoWidget::AspectRatio aspectRatio() const;
        void setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio);
        qreal brightness() const;
        void setBrightness(qreal);
        Phonon::VideoWidget::ScaleMode scaleMode() const;
        void setScaleMode(Phonon::VideoWidget::ScaleMode scaleMode);
        qreal contrast() const;
        void setContrast(qreal);
        qreal hue() const;
        void setHue(qreal);
        qreal saturation() const;
        void setSaturation(qreal);

        QWidget *widget();

        void updateVideo(VideoFrame &frame);

    protected:
        void mediaNodeEvent(const MediaNodeEvent *event);

    private:
        VideoRenderWidget *m_videoRenderWidget;
    };

}} // namespace Phonon::QT7

#endif // Phonon_QT7_VIDEOWIDGET_H
