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

#ifndef PHONON_VOLUMEEFFECT_H
#define PHONON_VOLUMEEFFECT_H

#include "effect.h"
#include <phonon/volumefaderinterface.h>

class QObject;

namespace Phonon
{
    namespace DS9
    {
        class VolumeEffectFilter;
        class VolumeEffect : public Effect, public Phonon::VolumeFaderInterface
        {
            Q_OBJECT
            Q_INTERFACES(Phonon::VolumeFaderInterface)
        public:
            VolumeEffect(QObject *parent);
            float volume() const;
            void setVolume(float);

            public Q_SLOTS:
                void test(int);
        private:
            QList<VolumeEffectFilter*> m_volumeFilters;
        };
    }
}
#endif
