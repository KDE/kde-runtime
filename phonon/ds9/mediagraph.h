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

#ifndef PHONON_MEDIAGRAPH_H
#define PHONON_MEDIAGRAPH_H

#include "backendnode.h"
#include <QtCore/QSet>
#include <QtCore/QMultiMap>

#include <phonon/mediasource.h>

#define GRAPH_DEBUG

namespace Phonon
{
    class MediaSource; //from phonon's frontend

    namespace DS9
    {
        class MediaObject;

        //in the end we should probably have no more inheritance here: everything should be in the interface of the class
        //could be nice to then remove all the "*this" in the code of this class
        class MediaGraph
        {
        public:
            MediaGraph(MediaObject *mo, short index);
            ~MediaGraph();
            bool isSeekable() const;
            qint64 totalTime() const;
            qint64 currentTime() const;
            HRESULT play();
            HRESULT stop();
            HRESULT pause();
            HRESULT seek(qint64);
            HRESULT saveToFile(const QString &filepath) const;
            QMultiMap<QString, QString> metadata() const;

            QSet<Filter> getAllFilters() const;

            HRESULT loadSource(const MediaSource &);

            bool hasVideo() const { return m_hasVideo; }
            void grabNode(BackendNode *node);

            //connections of the nodes
            bool connectNodes(BackendNode *source, BackendNode *sink);
            bool disconnectNodes(BackendNode *source, BackendNode *sink);

            void handleEvents();

            MediaSource mediaSource() const;

            //before loading a source, and after its playback this will be called
            HRESULT cleanup();

            short index() const;

            Filter realSource() const;

        private:
            //utility functions
            bool isSourceFilter(const Filter &filter) const;
            bool isDemuxerFilter(const Filter &filter) const;
            bool isDecoderFilter(const Filter &filter);
            static QSet<Filter> getFilterChain(const Filter &source, const Filter &sink);
            void ensureSourceConnectedTo(bool force = false);
            void ensureSourceDisconnected();
            bool tryConnect(const OutputPin &, const InputPin &);
            bool tryDisconnect(const OutputPin &, const InputPin &);
            HRESULT removeFilter(const Filter& filter);

            void restoreUnusedFilters();
            void removeUnusedFilters(); //remove all the nodes that are not connected to the source
            QSet<Filter> connectedFilters(const Filter &filter);

            //after loading, removes the decoders that are not linked to a sink
            HRESULT removeUselessDecoders();

            //COM objects
            ComPointer<IGraphBuilder> m_graph;
            ComPointer<IMediaControl> m_mediaControl;
            ComPointer<IMediaSeeking> m_mediaSeeking;
            ComPointer<IMediaEventEx> m_mediaEvent;
            Filter m_fakeSource, m_realSource;
            Filter m_demux;
            QSet<Filter> m_decoders;

            bool m_hasVideo;
            bool m_hasAudio;
            bool m_connectionsDirty;
            short m_index;
            qint64 m_clockDelta;

            MediaObject *m_mediaObject;
            MediaSource m_mediaSource;
            QSet<BackendNode*> m_sinkConnections; //connections to the source
            QSet<Filter> m_unusedFilters;

            Q_DISABLE_COPY(MediaGraph);
        };

    }
} //namespace Phonon::DS9

#endif // PHONON_MEDIAGRAPH_H
