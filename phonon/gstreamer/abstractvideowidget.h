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

#ifndef Phonon_UI_GSTREAMER_ABSTRACTVIDEOWIDGET_H
#define Phonon_UI_GSTREAMER_ABSTRACTVIDEOWIDGET_H

#include <QtGui/QPixmap>
#include <QtGui/QWidget>
#include <phonon/videowidget.h>
#include <phonon/videowidgetinterface.h>
#include <gst/gst.h>
#include "common.h"
#include "medianode.h"
#include "backend.h"

class QString;

namespace Phonon
{
namespace Gstreamer
{

class AbstractVideoWidget : public QObject, public Phonon::VideoWidgetInterface, public MediaNode
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoWidgetInterface Phonon::Gstreamer::MediaNode)
public:
    AbstractVideoWidget(Backend *backend, QWidget *parent = 0);
    ~AbstractVideoWidget();
    void mediaNodeEvent(const MediaNodeEvent *event);
    Phonon::VideoWidget::AspectRatio aspectRatio() const;
    void setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio);
    Phonon::VideoWidget::ScaleMode scaleMode() const;
    void setScaleMode(Phonon::VideoWidget::ScaleMode);
    qreal brightness() const;
    void setBrightness(qreal);
    qreal contrast() const;
    void setContrast(qreal);
    qreal hue() const;
    void setHue(qreal);
    qreal saturation() const;
    void setSaturation(qreal);
    void setMovieSize(const QSize &size);
    QSize defaultSizeHint() const;


    GstElement *videoElement()
    {
        Q_ASSERT(m_videoBin);
        return m_videoBin;
    }

    GstElement *videoSink()
    {
        return m_videoSink;
    }

    virtual GstElement *createVideoSink() = 0;

    void setupVideoBin();

    QSize movieSize() const {
        return m_movieSize;
    }

protected:
    GstElement *m_videoBin;
    GstElement *m_videoSink;
    QSize m_movieSize;

private:
    Phonon::VideoWidget::AspectRatio m_aspectRatio;
    qreal m_brightness, m_hue, m_contrast, m_saturation;
    Phonon::VideoWidget::ScaleMode m_scaleMode;

    GstElement *m_videoBalance;
    GstElement *m_colorspace;
};

}
} //namespace Phonon::Gstreamer

#endif // Phonon_UI_GSTREAMER_ABSTRACTVIDEOWIDGET_H
