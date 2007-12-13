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

#ifndef Phonon_QT7_VIDEOFRAME_H
#define Phonon_QT7_VIDEOFRAME_H

#include <QuickTime/QuickTime.h>
#undef check // avoid name clash;

#include <QtCore>

namespace Phonon
{
namespace QT7
{
    class QuickTimeVideoPlayer;

    class VideoFrame
    {
        public:
            VideoFrame();
            VideoFrame(QuickTimeVideoPlayer *videoPlayer, const CVTimeStamp &timeStamp);
            VideoFrame(const VideoFrame& frame);
            void operator=(const VideoFrame& frame);
            ~VideoFrame();

            CVOpenGLTextureRef cvTextureRef() const;
            GLuint glTextureRef() const;
            bool isEmpty();

            QRect frameRect() const;
            void drawFrame(const QRect &rect) const;
            void applyCoreImageFilter(void *filter);
            void setColors(qreal brightness, qreal contrast, qreal hue, qreal saturation);
            void setOpacity(qreal opacity);
            void setBackgroundFrame(const VideoFrame &frame);

            bool m_clearbg;
            qreal m_opacity;

            void retain() const;
            void release() const;

#if 0 // will be awailable in a later version of phonon.
        private:
            void *m_ciFrameRef;
            void *coreImageContext;

            void *coreImageRef() const;
            void *createCoreImage();
            bool hasCoreImage();
#endif

        private:
            CVOpenGLTextureRef m_cvTextureRef;
            QuickTimeVideoPlayer *m_videoPlayer;
            CVTimeStamp m_timeStamp;
            VideoFrame *m_backgroundFrame;

            qreal m_brightness;
            qreal m_contrast;
            qreal m_hue;
            qreal m_saturation;

            void initMembers();
            void copyMembers(const VideoFrame& frame);
            void drawGlTexture(const QRect &rect) const;
    };

}} //namespace Phonon::QT7

#endif // Phonon_QT7_VIDEOFRAME_H
