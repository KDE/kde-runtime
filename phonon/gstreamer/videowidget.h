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

#ifndef Phonon_UI_GSTREAMER_VIDEOWIDGET_H
#define Phonon_UI_GSTREAMER_VIDEOWIDGET_H

#include "abstractvideowidget.h"

#ifndef QT_NO_OPENGL
#include <QGLFormat>
#include <QGLWidget>
#endif

QT_BEGIN_NAMESPACE

class QString;

namespace Phonon
{
namespace Gstreamer
{

class VideoWidget;

#ifndef QT_NO_OPENGL
class GLRenderWidget : public QGLWidget
{
    Q_OBJECT

    // ARB_fragment_program
    typedef void (*_glProgramStringARB) (GLenum, GLenum, GLsizei, const GLvoid *);
    typedef void (*_glBindProgramARB) (GLenum, GLuint);
    typedef void (*_glDeleteProgramsARB) (GLsizei, const GLuint *);
    typedef void (*_glGenProgramsARB) (GLsizei, GLuint *);
    typedef void (*_glActiveTexture) (GLenum);

public:
    GLRenderWidget(VideoWidget *control, const QGLFormat &format, QWidget *parent = 0);
    void paintEvent(QPaintEvent *event);
    QSize sizeHint() const;

    void updateTexture(const QByteArray &array, int width, int height);

    bool hasYUVSupport() const;

private:
    VideoWidget *m_controller;
    GLuint m_texture[3];
    int m_width;
    int m_height;

    _glProgramStringARB glProgramStringARB;
    _glBindProgramARB glBindProgramARB;
    _glDeleteProgramsARB glDeleteProgramsARB;
    _glGenProgramsARB glGenProgramsARB;
    _glActiveTexture glActiveTexture;

    bool m_hasPrograms;
    GLuint m_program;

    bool m_yuvSupport;
};
#endif

class RenderWidget : public QWidget
{
    Q_OBJECT
public:
    RenderWidget(VideoWidget *control, QWidget *parent = 0):
            QWidget(parent), m_controller(control) { }
    void paintEvent(QPaintEvent *event);
    QSize sizeHint() const;
private:
    VideoWidget *m_controller;
};

class VideoWidget : public AbstractVideoWidget
{
    Q_OBJECT
public:
    VideoWidget(Backend *backend, QWidget *parent = 0, bool openglHint = true);
    GstElement *createVideoSink();
    QRect scaleToAspect(QRect srcRect, int w, int h);
    QWidget *widget()
    {
        return m_renderWidget;
    }
    const QImage& currentFrame() const;
    QRect drawFrameRect()
    {
        return m_drawFrameRect;
    }
    void calculateDrawFrameRect();
    void setNextFrame(const QByteArray &array, int width, int height);
    bool event( QEvent * event );
    void mediaNodeEvent(const MediaNodeEvent *event);
    void clearFrame();
    bool frameIsSet() { return !m_array.isNull(); }
private:
    mutable QImage m_frame;
    QByteArray m_array;
    int m_width;
    int m_height;
    QRect m_drawFrameRect;
    QWidget *m_renderWidget;
    bool m_usingOpenGL;
};

}
} //namespace Phonon::Gstreamer

QT_END_NAMESPACE

#endif // Phonon_UI_GSTREAMER_VIDEOWIDGET_H
