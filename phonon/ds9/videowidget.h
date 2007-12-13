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

#ifndef PHONON_VIDEOWIDGET_H
#define PHONON_VIDEOWIDGET_H

#include <QtGui/QWidget>
#include <phonon/videowidget.h>
#include <phonon/videowidgetinterface.h>
#include "backendnode.h"

namespace Phonon
{
    namespace DS9
    {
        class VideoWindow;

        class VideoWidget : public BackendNode, public Phonon::VideoWidgetInterface
        {
            Q_OBJECT
            Q_INTERFACES(Phonon::VideoWidgetInterface)
        public:
            VideoWidget(QWidget *parent = 0);
            ~VideoWidget();

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

            void setCurrentGraph(int index);

            QWidget *widget();

        protected:
            virtual void connected(BackendNode *, const InputPin& inpin);

        private:
            //apply contrast/brightness/hue/saturation
            void applyMixerSettings() const;

            bool m_fullscreen;
            QSize m_videoSize;
            Phonon::VideoWidget::AspectRatio m_aspectRatio;
            Phonon::VideoWidget::ScaleMode m_scaleMode;

            VideoWindow *m_widget;
            qreal m_brightness, m_contrast, m_hue, m_saturation;
        };
    }
} //namespace Phonon::DS9

#endif // PHONON_VIDEOWIDGET_H
