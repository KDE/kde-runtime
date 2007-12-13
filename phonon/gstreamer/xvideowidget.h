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

#ifndef Phonon_UI_GSTREAMER_XVIDEOWIDGET_H
#define Phonon_UI_GSTREAMER_XVIDEOWIDGET_H

#include "abstractvideowidget.h"

#ifndef Q_WS_QWS

class QString;

namespace Phonon
{
namespace Gstreamer
{

class XVideoWidget;
class XWidget : public QWidget
{
public:
    XWidget(XVideoWidget *controller, QWidget *parent) :
            QWidget(parent)
            , m_controller(controller)
            , m_overlaySet(false)
    { }
    void paintEvent(QPaintEvent *event);
    QSize sizeHint() const;
    bool overlaySet() const
    {
        return m_overlaySet;
    }
    void setOverlay();
    void windowExposed();

private:
    XVideoWidget *m_controller;
    bool m_overlaySet;
};

class XVideoWidget : public AbstractVideoWidget
{
public:
    XVideoWidget(Backend *backend, QWidget *parent = 0);
protected:
    GstElement *createVideoSink();
    void setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio);
    QWidget *widget()
    {
        return m_renderWidget;
    }
    void mediaNodeEvent(const MediaNodeEvent *event);
    XWidget *m_renderWidget;
};

}
} //namespace Phonon::Gstreamer

#endif // Q_WS_QWS
#endif // Phonon_UI_GSTREAMER_XVIDEOWIDGET_H
