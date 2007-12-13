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

#include <QtGui/QPainter>
#include <gst/gst.h>
#include "common.h"
#include "message.h"
#include "mediaobject.h"
#include "qwidgetvideosink.h"
#include "videowidget.h"
#include "qrgb.h"

static void frameRendered()
{
    static QString displayFps = qgetenv("PHONON_GST_FPS");
    if (displayFps.isEmpty())
        return;

    static int frames = 0;
    static QTime lastTime = QTime::currentTime();
    QTime time = QTime::currentTime();

    int delta = lastTime.msecsTo(time);
    if (delta > 2000) {
        printf("FPS: %f\n", 1000.0 * frames / qreal(delta));
        lastTime = time;
        frames = 0;
    }

    ++frames;
}

namespace Phonon
{
namespace Gstreamer
{

VideoWidget::VideoWidget(Backend *backend, QWidget *parent, bool openglHint)
        : AbstractVideoWidget(backend, parent)
{
    Q_UNUSED(openglHint);
    static int count = 0;
    m_name = "VideoWidget" + QString::number(count++);

#ifndef QT_NO_OPENGL
    // We default to opengl if available
    if (QGLFormat::hasOpenGL() && openglHint) {
        QGLFormat format = QGLFormat::defaultFormat();
        // Enable vertical sync on draw to avoid tearing
        format.setSwapInterval(1);
        m_renderWidget = new GLRenderWidget(this, format, parent);

        m_usingOpenGL = true;
    } else
#endif
    {
        m_renderWidget = new RenderWidget(this, parent);
        m_usingOpenGL = false;
    }

    if ((m_videoSink = createVideoSink())) {
        QWidgetVideoSinkBase*  sink = reinterpret_cast<QWidgetVideoSinkBase*>(m_videoSink);
        // Let the videosink know which widget to direct frame updates to
        sink->control = this;
        setupVideoBin();
    }

    // Clear the background with black by default
    // ### Investigate why the Xine backend used Qt::blue for this
    QPalette palette;
    palette.setColor(QPalette::Background, Qt::black);
    m_renderWidget->setPalette(palette);
    m_renderWidget->setAutoFillBackground(true);

}

GstElement* VideoWidget::createVideoSink()
{
#ifndef QT_NO_OPENGL
    if (m_usingOpenGL && qobject_cast<GLRenderWidget *>(m_renderWidget)->hasYUVSupport())
        return GST_ELEMENT(g_object_new(get_type_YUV(), NULL));
    else
#endif
        return GST_ELEMENT(g_object_new(get_type_RGB(), NULL));
}

void VideoWidget::setNextFrame(const QByteArray &array, int w, int h)
{
    m_frame = QImage();

#ifndef QT_NO_OPENGL
    if (m_usingOpenGL) {
        GLRenderWidget *widget = static_cast<GLRenderWidget*>(m_renderWidget);

        if (widget->hasYUVSupport())
            widget->updateTexture(array, w, h);
        else
            m_frame = QImage((uchar *)array.constData(), w, h, QImage::Format_RGB32);
    } else
#endif
    {
        m_frame = QImage((uchar *)array.constData(), w, h, QImage::Format_RGB32);
    }

    m_array = array;
    m_width = w;
    m_height = h;

    m_renderWidget->update();
}

static QImage convertFromYUV(const QByteArray &array, int w, int h)
{
    QImage result(w, h, QImage::Format_RGB32);

    // TODO: bilinearly interpolate the U and V channels for better result

    for (int y = 0; y < h; ++y) {
        uint *sp = (uint *)result.scanLine(y);

        const uchar *yp = (const uchar *)(array.constData() + y * w);
        const uchar *up = (const uchar *)(array.constData() + w * h + (y/2)*(w/2));
        const uchar *vp = (const uchar *)(array.constData() + w * h * 5/4 + (y/2)*(w/2));

        for (int x = 0; x < w; ++x) {
            const int sy = *yp;
            const int su = *up;
            const int sv = *vp;

            const int R = int(1.164 * (sy - 16) + 1.596 * (sv - 128));
            const int G = int(1.164 * (sy - 16) - 0.813 * (sv - 128) - 0.391 * (su - 128));
            const int B = int(1.164 * (sy - 16)                      + 2.018 * (su - 128));

            *sp = qRgb(qBound(0, R, 255),
                       qBound(0, G, 255),
                       qBound(0, B, 255));

            ++yp;
            ++sp;
            if (x & 1) {
                ++up;
                ++vp;
            }
        }
    }
    return result;
}

const QImage &VideoWidget::currentFrame() const
{
    if (m_frame.isNull() && !m_array.isNull())
        m_frame = convertFromYUV(m_array, m_width, m_height);

    return m_frame;
}

QRect VideoWidget::scaleToAspect(QRect srcRect, int w, int h)
{
    float width = srcRect.width();
    float height = srcRect.width() * (float(h) / float(w));
    if (height > srcRect.height()) {
        height = srcRect.height();
        width = srcRect.height() * (float(w) / float(h));
    }
    return QRect(0, 0, (int)width, (int)height);
}

/***
 * Calculates the actual rectangle the movie will be presented with
 *
 * ### This function does currently asumes a 1:1 pixel aspect
 **/
void VideoWidget::calculateDrawFrameRect()
{
    QRect widgetRect = widget()->rect();

    // Set m_drawFrameRect to be the size of the smallest possible
    // rect conforming to the aspect and containing the whole frame:
    switch (aspectRatio()) {

    case Phonon::VideoWidget::AspectRatioWidget:
        m_drawFrameRect = widgetRect;
        // No more calculations needed.
        return;

    case Phonon::VideoWidget::AspectRatio4_3:
        m_drawFrameRect = scaleToAspect(widgetRect, 4, 3);
        break;

    case Phonon::VideoWidget::AspectRatio16_9:
        m_drawFrameRect = scaleToAspect(widgetRect, 16, 9);
        break;

    case Phonon::VideoWidget::AspectRatioAuto:
    default:
        m_drawFrameRect = QRect(0, 0, movieSize().width(), movieSize().height());
        break;
    }

    // Scale m_drawFrameRect to fill the widget
    // without breaking aspect:
    float widgetWidth = widgetRect.width();
    float widgetHeight = widgetRect.height();
    float frameWidth = widgetWidth;
    float frameHeight = m_drawFrameRect.height() * float(widgetWidth) / float(m_drawFrameRect.width());

    switch (scaleMode()) {
    case Phonon::VideoWidget::ScaleAndCrop:
        if (frameHeight < widgetHeight) {
            frameWidth *= float(widgetHeight) / float(frameHeight);
            frameHeight = widgetHeight;
        }
        break;
    case Phonon::VideoWidget::FitInView:
    default:
        if (frameHeight > widgetHeight) {
            frameWidth *= float(widgetHeight) / float(frameHeight);
            frameHeight = widgetHeight;
        }
        break;
    }
    m_drawFrameRect.setSize(QSize(int(frameWidth), int(frameHeight)));
    m_drawFrameRect.moveTo(int((widgetWidth - frameWidth) / 2.0f),
                           int((widgetHeight - frameHeight) / 2.0f));
}

bool VideoWidget::event(QEvent * event)
{
    if (event->type() == QEvent::User) {
        NewFrameEvent *frameEvent= static_cast <NewFrameEvent *>(event);
        setNextFrame(frameEvent->frame, frameEvent->width, frameEvent->height);
        return true;
    }
    return AbstractVideoWidget::event(event);
}

// --- No-GL render widget ---
void RenderWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_controller->calculateDrawFrameRect();
    painter.drawImage(m_controller->drawFrameRect(), m_controller->currentFrame());

    frameRendered();
}

QSize RenderWidget::sizeHint() const
{
    return m_controller->defaultSizeHint();
}

// --- Open-GL render widget ---
#ifndef QT_NO_OPENGL

#ifndef GL_FRAGMENT_PROGRAM_ARB
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#endif

// arbfp1 fragment program for converting yuv to rgb
const char *yuvToRgb =
    "!!ARBfp1.0"
    "PARAM c[3] = { { 0.5, 0.0625 },"
    "{ 1.164, 0, 1.596, 2.0179999 },"
    "{ 1.164, -0.391, -0.81300002 } };"
    "TEMP R0;"
    "TEMP R1;"
    "TEX R0.x, fragment.texcoord[0], texture[2], 2D;"
    "ADD R1.z, R0.x, -c[0].x;"
    "TEX R1.x, fragment.texcoord[0], texture[0], 2D;"
    "TEX R0.x, fragment.texcoord[0], texture[1], 2D;"
    "ADD R1.x, R1, -c[0].y;"
    "ADD R1.y, R0.x, -c[0].x;"
    "DP3 result.color.x, R1, c[1];"
    "DP3 result.color.y, R1, c[2];"
    "DP3 result.color.z, R1, c[1].xwyw;"
    "END";

GLRenderWidget::GLRenderWidget(VideoWidget *control, const QGLFormat &format, QWidget *parent) :
        QGLWidget(format, parent), m_controller(control)
{
    makeCurrent();
    glGenTextures(3, m_texture);

    setAutoFillBackground(false);

    glProgramStringARB = (_glProgramStringARB) context()->getProcAddress(QLatin1String("glProgramStringARB"));
    glBindProgramARB = (_glBindProgramARB) context()->getProcAddress(QLatin1String("glBindProgramARB"));
    glDeleteProgramsARB = (_glDeleteProgramsARB) context()->getProcAddress(QLatin1String("glDeleteProgramsARB"));
    glGenProgramsARB = (_glGenProgramsARB) context()->getProcAddress(QLatin1String("glGenProgramsARB"));
    glActiveTexture = (_glActiveTexture) context()->getProcAddress(QLatin1String("glActiveTexture"));

    m_hasPrograms = glProgramStringARB && glBindProgramARB && glDeleteProgramsARB && glGenProgramsARB && glActiveTexture;

    m_yuvSupport = false;
    if (m_hasPrograms) {
        glGenProgramsARB(1, &m_program);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_program);

        const GLbyte *gl_src = reinterpret_cast<const GLbyte *>(yuvToRgb);
        glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           int(strlen(yuvToRgb)), gl_src);

        if (glGetError() != GL_NO_ERROR) {
            glDeleteProgramsARB(1, &m_program);
            m_hasPrograms = false;
        } else {
            m_yuvSupport = true;
        }
    }
}

bool GLRenderWidget::hasYUVSupport() const
{
    return m_yuvSupport;
}

void GLRenderWidget::updateTexture(const QByteArray &array, int width, int height)
{
    m_width = width;
    m_height = height;

    makeCurrent();

    int w[3] = { width, width/2, width/2 };
    int h[3] = { height, height/2, height/2 };
    int offs[3] = { 0, width*height, width*height*5/4 };

    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_texture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w[i], h[i], 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, array.data() + offs[i]);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
}

void GLRenderWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_controller->calculateDrawFrameRect();

    if (m_yuvSupport) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_program);

        const float tx_array[] = { 0, 0, 1, 0, 1, 1, 0, 1};
        const QRect r = m_controller->drawFrameRect();
        const float v_array[] = { r.left(), r.top(), r.right(), r.top(), r.right(), r.bottom(), r.left(), r.bottom() };

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_texture[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_texture[2]);
        glActiveTexture(GL_TEXTURE0);

        glVertexPointer(2, GL_FLOAT, 0, v_array);
        glTexCoordPointer(2, GL_FLOAT, 0, tx_array);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDrawArrays(GL_QUADS, 0, 4);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glDisable(GL_FRAGMENT_PROGRAM_ARB);
    } else {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(m_controller->drawFrameRect(), m_controller->currentFrame());
    }

    frameRendered();
}

QSize GLRenderWidget::sizeHint() const
{
    return m_controller->defaultSizeHint();
}
#endif // QT_NO_OPENGL

}
} //namespace Phonon::Gstreamer
