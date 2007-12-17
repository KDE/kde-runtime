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

#include "videowidget.h"

#include <d3d9.h>
#include <vmr9.h>

#include "mediaobject.h"

namespace Phonon
{
    namespace DS9
    {
        //class used internally to return the widget where the video is shown on
        class VideoWindow : public QWidget
        {
        public:
            explicit VideoWindow(QWidget *parent, VideoWidget *vw)
                : QWidget(parent), m_node(vw), m_dstX(0), m_dstY(0),
                m_dstWidth(0), m_dstHeight(0)
            {
                setAttribute(Qt::WA_OpaquePaintEvent, true);
                setAttribute(Qt::WA_PaintOnScreen, true);
                setAttribute(Qt::WA_NoSystemBackground, true);
                setAutoFillBackground(false);

                //default background color
                setPalette( QPalette(Qt::black));
            }

            void setCurrentFilter(const Filter &filter)
            {
                if (m_windowlessControl) {
                    RECT dummyRect = { 0, 0, 0, 0};
                    m_windowlessControl->SetVideoPosition(&dummyRect, &dummyRect);
                }

                m_windowlessControl = ComPointer<IVMRWindowlessControl9>(filter, IID_IVMRWindowlessControl9);
                if (m_windowlessControl) {
                    updateVideoSize();
                    update(); //be sure to update the current video frame
                }
            }

            void resizeEvent(QResizeEvent *e)
            {
                updateVideoSize();
                QWidget::resizeEvent(e);
            }

            void updateVideoSize()
            {
                if (!m_windowlessControl) {
                    return;
                }

                //update the video rects
                qreal ratio = -1.;
                HRESULT hr = S_OK;
                switch(m_node->aspectRatio())
                {
                case Phonon::VideoWidget::AspectRatioAuto:
                    {
                        //preserve the aspect ration of the video
                        LONG width = 0, height = 0, arWidth = 0, arHeight = 0;
                        hr = m_windowlessControl->GetNativeVideoSize(&width, &height, &arWidth, &arHeight);
                        Q_ASSERT(SUCCEEDED(hr));
                        ratio = qreal(arWidth) / qreal(arHeight);
                    }
                    break;
                case Phonon::VideoWidget::AspectRatio4_3:
                    ratio = 4. / 3.;
                    break;
                case Phonon::VideoWidget::AspectRatio16_9:
                    ratio = 16. / 9.;
                    break;
                case Phonon::VideoWidget::AspectRatioWidget:
                default:
                    break;
                }

                LONG vWidth = 0, vHeight = 0, vARWidth = 0, vARHeight = 0;
                m_windowlessControl->GetNativeVideoSize(&vWidth, &vHeight, &vARWidth, &vARHeight);

                const qreal rwidth = width(), rheight = height();

                //reinitialization
                m_dstWidth = width();
                m_dstHeight = height();
                m_dstX = m_dstY = 0;

                if (ratio > 0) {
                    if (rwidth / rheight > ratio && m_node->scaleMode() == Phonon::VideoWidget::FitInView
                        || rwidth / rheight < ratio && m_node->scaleMode() == Phonon::VideoWidget::ScaleAndCrop) {
                            //the height is correct, let's change the width
                            m_dstWidth = qRound(rheight * ratio);
                            m_dstX = qRound((rwidth - rheight * ratio) / 2.);
                    } else {
                        m_dstHeight = qRound(rwidth / ratio);
                        m_dstY = qRound((rheight - rwidth / ratio) / 2.);
                    }
                }

                RECT dstRectWin = { m_dstX, m_dstY, m_dstWidth + m_dstX, m_dstHeight + m_dstY};
                RECT srcRectWin = { 0, 0, vWidth, vHeight};
                hr = m_windowlessControl->SetVideoPosition(&srcRectWin, &dstRectWin);
                Q_ASSERT(SUCCEEDED(hr));
            }

            QSize minimumSizeHint() const
            {
                LONG w = 0,
                    h = 0;
                if (m_windowlessControl) {
                    m_windowlessControl->GetMinIdealVideoSize( &w, &h);
                }
                return QSize(w, h).expandedTo(QWidget::minimumSizeHint());
            }

            QSize sizeHint() const
            {
                LONG w = 0,
                    h = 0,
                    vARWidth = 0, vARHeight = 0; //useless
                if (m_windowlessControl) {
                    m_windowlessControl->GetNativeVideoSize( &w, &h, &vARWidth, &vARHeight);
                }
                return QSize(w, h).expandedTo(QWidget::sizeHint());
            }

            void paintEvent(QPaintEvent *)
            {
                HDC hDC = getDC();
                // repaint the video
                HRESULT hr = m_windowlessControl ? m_windowlessControl->RepaintVideo(winId(), hDC) : E_POINTER;
                if (FAILED(hr) || m_dstY > 0 || m_dstX > 0) {
                    const QColor c = palette().color(backgroundRole());
                    COLORREF color = RGB(c.red(), c.green(), c.blue());
                    HPEN hPen = ::CreatePen(PS_SOLID, 1, color);
                    HBRUSH hBrush = ::CreateSolidBrush(color);
                    ::SelectObject(hDC, hPen);
                    ::SelectObject(hDC, hBrush);
                    // repaint the video
                    if (FAILED(hr)) {
                        //black background : we use the Win32 API to avoid the ghost effect of the backing store
                        ::Rectangle(hDC, 0, 0, width(), height());
                    } else {
                        if (m_dstY > 0) {
                            ::Rectangle(hDC, 0, 0, width(), m_dstY); //top
                            ::Rectangle(hDC, 0, height() - m_dstY, width(), height()); //bottom
                        }
                        if (m_dstX > 0) {
                            ::Rectangle(hDC, 0, m_dstY, m_dstX, m_dstHeight + m_dstY); //left
                            ::Rectangle(hDC, m_dstWidth + m_dstX, m_dstY, width(), m_dstHeight + m_dstY); //right
                        }
                    }
                    ::DeleteObject(hPen);
                    ::DeleteObject(hBrush);
                }
                releaseDC(hDC);
            }

        private:
            ComPointer<IVMRWindowlessControl9> m_windowlessControl;
            VideoWidget *m_node;
            int m_dstX, m_dstY, m_dstWidth, m_dstHeight;

        };

        VideoWidget::VideoWidget(QWidget *parent)
            : BackendNode(parent), m_brightness(0.), m_contrast(0.), m_hue(0.), m_saturation(0.),
            m_aspectRatio(Phonon::VideoWidget::AspectRatioAuto), m_scaleMode(Phonon::VideoWidget::FitInView)
        {
            //initialisation of the widget
            m_widget = new VideoWindow(parent, this);

            //initialization of the widgets
            for(QVector<Filter>::iterator it = m_filters.begin(); it != m_filters.end(); ++it) {
                Filter &filter = *it;
                // create the VMR9 filter COM instance and configure it
                filter = getVideoRenderer();
                if (!filter) {
                    qWarning("the video widget could not be initialized correctly");
                } else {
                    ComPointer<IVMRFilterConfig9> config(filter, IID_IVMRFilterConfig9);
                    Q_ASSERT(config);
                    HRESULT hr = config->SetRenderingMode(VMR9Mode_Windowless);
                    Q_ASSERT(SUCCEEDED(hr));
                    hr = config->SetNumberOfStreams(1); //for now we limit it to 1 input stream
                    Q_ASSERT(SUCCEEDED(hr));
                    ComPointer<IVMRWindowlessControl9> windowless(filter, IID_IVMRWindowlessControl9);
                    Q_ASSERT(config);
                    hr = windowless->SetVideoClippingWindow(reinterpret_cast<HWND>(m_widget->winId()));
                    Q_ASSERT(SUCCEEDED(hr));
                }
            }

            //by default, we take the first VideoWindow object
            setCurrentGraph(0);
        }

        VideoWidget::~VideoWidget()
        {
        }

        Filter VideoWidget::getVideoRenderer()
        {
            return Filter(CLSID_VideoMixingRenderer9, IID_IBaseFilter);
        }


        void VideoWidget::setCurrentGraph(int index)
        {
            m_widget->setCurrentFilter( filter(index) );
        }


        Phonon::VideoWidget::AspectRatio VideoWidget::aspectRatio() const
        {
            return m_aspectRatio;
        }

        void VideoWidget::setAspectRatio(Phonon::VideoWidget::AspectRatio aspectRatio)
        {
            m_aspectRatio = aspectRatio;
            if (m_widget) {
                m_widget->updateVideoSize();
            }
        }

        Phonon::VideoWidget::ScaleMode VideoWidget::scaleMode() const
        {
            return m_scaleMode;
        }


        QWidget *VideoWidget::widget()
        {
            return m_widget;
        }


        void VideoWidget::setScaleMode(Phonon::VideoWidget::ScaleMode scaleMode)
        {
            m_scaleMode = scaleMode;
            if (m_widget) {
                m_widget->updateVideoSize();
            }
        }

        void VideoWidget::setBrightness(qreal b)
        {
            m_brightness = b;
            applyMixerSettings();
        }

        void VideoWidget::setContrast(qreal c)
        {
            m_contrast = c;
            applyMixerSettings();
        }

        void VideoWidget::setHue(qreal h)
        {
            m_hue = h;
            applyMixerSettings();
        }

        void VideoWidget::setSaturation(qreal s)
        {
            m_saturation = s;
            applyMixerSettings();
        }

        qreal VideoWidget::brightness() const
        {
            return m_brightness;
        }


        qreal VideoWidget::contrast() const
        {
            return m_contrast;
        }

        qreal VideoWidget::hue() const
        {
            return m_hue;
        }

        qreal VideoWidget::saturation() const
        {
            return m_saturation;
        }


        //this must be called whe nthe node is actually connected
        void  VideoWidget::applyMixerSettings() const
        {
            foreach(const Filter filter, m_filters) {
                if (!filter) {
                    continue; //happens in case it couldn't instanciate the VMR9
                }

                InputPin sink = BackendNode::pins(filter, PINDIR_INPUT).first();
                OutputPin source;
                if (FAILED(sink->ConnectedTo(&source))) {
                    continue; //it must be connected to work
                }

                //get the mixer (used for brightness/contrast/saturation/hue)
                ComPointer<IVMRMixerControl9> mixer(filter, IID_IVMRMixerControl9);
                Q_ASSERT(mixer);

                VMR9ProcAmpControl ctrl;
                ctrl.dwSize = sizeof(ctrl);
                ctrl.dwFlags = ProcAmpControl9_Contrast | ProcAmpControl9_Brightness | ProcAmpControl9_Saturation | ProcAmpControl9_Hue;
                VMR9ProcAmpControlRange range;
                range.dwSize = sizeof(range);

                range.dwProperty = ProcAmpControl9_Contrast;
                HRESULT hr = mixer->GetProcAmpControlRange(0, &range);
                if (FAILED(hr)) {
                    return;
                }
                ctrl.Contrast = ((m_contrast<0 ? range.MinValue : range.MaxValue) - range.DefaultValue) * qAbs(m_contrast) + range.DefaultValue;

                //brightness
                range.dwProperty = ProcAmpControl9_Brightness;
                hr = mixer->GetProcAmpControlRange(0, &range);
                if (FAILED(hr)) {
                    return;
                }
                ctrl.Brightness = ((m_brightness<0 ? range.MinValue : range.MaxValue) - range.DefaultValue) * qAbs(m_brightness) + range.DefaultValue;

                //saturation
                range.dwProperty = ProcAmpControl9_Saturation;
                hr = mixer->GetProcAmpControlRange(0, &range);
                if (FAILED(hr)) {
                    return;
                }
                ctrl.Saturation = ((m_saturation<0 ? range.MinValue : range.MaxValue) - range.DefaultValue) * qAbs(m_saturation) + range.DefaultValue;

                //hue
                range.dwProperty = ProcAmpControl9_Hue;
                hr = mixer->GetProcAmpControlRange(0, &range);
                if (FAILED(hr)) {
                    return;
                }
                ctrl.Hue = ((m_hue<0 ? range.MinValue : range.MaxValue) - range.DefaultValue) * qAbs(m_hue) + range.DefaultValue;

                //finally set the settings
                hr = mixer->SetProcAmpControl(0, &ctrl);
                m_mediaObject->catchComError(hr); //just try to catch the errors
            }
        }

        void VideoWidget::connected(BackendNode *, const InputPin&)
        {
            //in case of a connection, we simply reapply the mixer settings
            applyMixerSettings();
            if (m_widget) {
                m_widget->updateVideoSize();
            }
        }

    }
} //namespace Phonon::DS9

#include "moc_videowidget.cpp"
