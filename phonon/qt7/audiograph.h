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

#ifndef Phonon_QT7_AUDIOGRAPH_H
#define Phonon_QT7_AUDIOGRAPH_H

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

#include <QtCore>
#include "audioconnection.h"
#include "medianode.h"

namespace Phonon
{
namespace QT7
{
    class AudioGraph : public MediaNode
    {
        public:
            AudioGraph(MediaNode *root);
            virtual ~AudioGraph();
            AUGraph audioGraphRef();

            bool openAndInit();
            void start();
            void stop();
            void rebuildGraph();
            bool graphCannotPlay();
            void rebuildGraphIfNeeded();
            void updateStreamSpecifications();
            MediaNode *root();

        private:
            friend class MediaNode;
            friend class AudioNode;

            bool updateStreamSpecificationRecursive(AudioConnection *connection);
            void createAndConnectAuNodesRecursive(AudioConnection *connection);
            bool createAudioUnitsRecursive(AudioConnection *connection);
            void deleteAUGraph();

            void connectLate(AudioConnection *connection);
            void disconnectLate(AudioConnection *connection);

            void tryStartGraph();
            int nodeCount();

            bool m_initialized;
            bool m_startedLogically;
            bool m_rebuildLater;
            bool m_graphCannotPlay;

            AUGraph m_audioGraphRef;
            MediaNode *m_root;
    };

}} // namespace Phonon::QT7

#endif // Phonon_QT7_AUDIOGRAPH_H
