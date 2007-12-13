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

#ifndef Phonon_GSTREAMER_AUDIOEFFECT_H
#define Phonon_GSTREAMER_AUDIOEFFECT_H

#include <QtCore/QObject>
#include <phonon/effectparameter.h>
#include <phonon/effectinterface.h>
#include "medianode.h"
#include <gst/gst.h>

namespace Phonon
{
namespace Gstreamer
{
    class AudioOutput;
    class AudioEffectInfo;
    class AudioEffect : public QObject, public Phonon::EffectInterface, public MediaNode
    {
        Q_OBJECT
        Q_INTERFACES(Phonon::EffectInterface Phonon::Gstreamer::MediaNode)
        public:
            AudioEffect (Backend *backend, int effectId, QObject *parent);
            ~AudioEffect ();

            QList<EffectParameter> parameters() const;
            QVariant parameterValue(const EffectParameter &) const;
            void setParameterValue(const EffectParameter &, const QVariant &);

            GstElement *audioElement() { return m_audioBin; }
            GstElement* createAudioBin();
            void setupEffectParams();

        private:
            GstElement *m_audioBin;
            GstElement *m_effectElement;
            QList<Phonon::EffectParameter> m_parameterList;
            AudioEffectInfo *m_effect;
    };
}} //namespace Phonon::Gstreamer

#endif // Phonon_GSTREAMER_AUDIOEFFECT_H
