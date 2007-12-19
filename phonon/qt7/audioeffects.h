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

#ifndef Phonon_QT7_AUDIOEFFECTS_H
#define Phonon_QT7_AUDIOEFFECTS_H

#include <QtCore>
#include <phonon/effectinterface.h>
#include <phonon/effectparameter.h>
#include "medianode.h"
#include "audionode.h"

namespace Phonon
{
namespace QT7
{
    class AudioEffectAudioNode : public AudioNode
    {
        public:
            AudioEffectAudioNode(int effectType, QString name, QString description = QString());
            int m_effectType;
            QString m_name;
            QString m_description;
        protected:
            ComponentDescription getAudioNodeDescription();
            void initializeAudioUnit();
    };

///////////////////////////////////////////////////////////////////////

    class AudioEffect : public MediaNode, Phonon::EffectInterface
    {
        Q_OBJECT
        Q_INTERFACES(Phonon::EffectInterface)

        public:
            AudioEffect(int effectType, QObject *parent = 0);
            AudioEffectAudioNode *m_audioNode;

            QString name();
            QString description();
         
            // EffectInterface:
            virtual QList<Phonon::EffectParameter> parameters() const;
            virtual QVariant parameterValue(const Phonon::EffectParameter &parameter) const;
            virtual void setParameterValue(const Phonon::EffectParameter &parameter, const QVariant &newValue);

            static bool effectAwailable(int effectType);
            static QList<int> effectList();
    };


}} //namespace Phonon::QT7

#endif // Phonon_QT7_AUDIOEFFECTS_H
