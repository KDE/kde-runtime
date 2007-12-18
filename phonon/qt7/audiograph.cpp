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

#include "audiograph.h"
#include "quicktimeaudioplayer.h"
#include "medianode.h"

namespace Phonon
{
namespace QT7
{

AudioGraph::AudioGraph(MediaNode *root) : MediaNode(AudioGraphNode, 0, root), m_root(root)
{
    m_audioGraphRef = 0;
    m_initialized = false;
    m_startedLogically = false;
    m_graphCannotPlay = false;
}

AudioGraph::~AudioGraph()
{
    startAllOverFromScratch();
}

void AudioGraph::startAllOverFromScratch()
{
    if (m_audioGraphRef){
        DEBUG_AUDIO_GRAPH("Graph" << int(this) << "is deleted")
    	AUGraphStop(m_audioGraphRef);
	    AUGraphUninitialize(m_audioGraphRef);
        AUGraphClose(m_audioGraphRef);
        DisposeAUGraph(m_audioGraphRef);
        m_initialized = false;
        m_graphCannotPlay = false;
    }
}

MediaNode *AudioGraph::root()
{
    return m_root;
}

AUGraph AudioGraph::audioGraphRef()
{
    return m_audioGraphRef;
}

void AudioGraph::rebuildGraph()
{
    DEBUG_AUDIO_GRAPH("Graph" << int(this) << "is rebuilding")
    MediaNodeEvent event(MediaNodeEvent::AudioGraphAboutToBeDeleted, this);
    m_root->notify(&event);
    startAllOverFromScratch();
    
    if (!openAndInit()){
        m_graphCannotPlay = true;
        MediaNodeEvent e(MediaNodeEvent::AudioGraphCannotPlay, this);
        m_root->notify(&e);
    } else { 
        tryStartGraph();
        m_graphCannotPlay = false;
    }   
}

bool AudioGraph::graphCannotPlay()
{
    return m_graphCannotPlay;
}

void AudioGraph::updateStreamSpecifications()
{
    if (!m_initialized){
        if (m_graphCannotPlay)
            rebuildGraph();
        return;
    }

    AudioConnection rootConnection(m_root);
    bool updateOk = updateStreamSpecificationRecursive(&rootConnection);
    if (!updateOk){
        DEBUG_AUDIO_GRAPH("Graph" << int(this) << "could not update stream specification. Rebuild.")
        rebuildGraph();
    }
}

bool AudioGraph::updateStreamSpecificationRecursive(AudioConnection *connection)
{
    bool updateOk = connection->updateStreamSpecification();
    if (!updateOk)
        return false;

    for (int i=0; i<connection->m_sink->m_audioSinkList.size(); ++i){
        if (!updateStreamSpecificationRecursive(connection->m_sink->m_audioSinkList[i]))
            return false;
    }
    return true;
}

bool AudioGraph::openAndInit()
{
	OSStatus err;
	err = NewAUGraph(&m_audioGraphRef);
    BACKEND_ASSERT3(err == noErr, "Could not create audio graph.", NORMAL_ERROR, false)

    MediaNodeEvent eventNew(MediaNodeEvent::NewAudioGraph, this);
    m_root->notify(&eventNew);

    AudioConnection rootConnection(m_root);
    createAndConnectAuNodesRecursive(&rootConnection);
	err = AUGraphOpen(m_audioGraphRef);
    BACKEND_ASSERT3(err == noErr, "Could not create audio graph.", NORMAL_ERROR, false)

    if (!createAudioUnitsRecursive(&rootConnection))
        return false;

	err = AUGraphInitialize(m_audioGraphRef);
    BACKEND_ASSERT3(err == noErr, "Could not initialize audio graph.", NORMAL_ERROR, false)

    m_initialized = true;
    MediaNodeEvent eventInit(MediaNodeEvent::AudioGraphInitialized, this);
    m_root->notify(&eventInit);
    return true;
}

void AudioGraph::createAndConnectAuNodesRecursive(AudioConnection *connection)
{
    connection->m_sink->m_audioNode->createAndConnectAUNodes();
    for (int i=0; i<connection->m_sink->m_audioSinkList.size(); ++i){
        AudioConnection *c = connection->m_sink->m_audioSinkList[i];
        createAndConnectAuNodesRecursive(c);
        bool ok = c->connect(this);
        BACKEND_ASSERT2(ok, "Could not connect an audio nodes pair in the audio graph.", NORMAL_ERROR)
    }
}

bool AudioGraph::createAudioUnitsRecursive(AudioConnection *connection)
{
    connection->m_sink->m_audioNode->createAudioUnits();
    bool ok = connection->updateStreamSpecification();
    if (!ok)
        return false;
    for (int i=0; i<connection->m_sink->m_audioSinkList.size(); ++i){
        if (!createAudioUnitsRecursive(connection->m_sink->m_audioSinkList[i]))
            return false;
    }
    return true;
}

void AudioGraph::tryStartGraph()
{
    // The graph will only start if the background AUGraph
    // is valid. Therefore we just try. If it fails, user
    // actions like connect etc. migh make the graph valid
    // at a later point.
    Boolean running;
    AUGraphIsRunning(m_audioGraphRef, &running);

    if (m_startedLogically && !running){
        OSStatus err = AUGraphStart(m_audioGraphRef);
        if (err == noErr)
            DEBUG_AUDIO_GRAPH("Graph" << int(this) << "started")
        else
            DEBUG_AUDIO_GRAPH("Graph" << int(this) << "could not start")
    }
}

void AudioGraph::connectLate(AudioConnection *connection)
{
    MediaNodeEvent event(MediaNodeEvent::NewAudioGraph, this);
    connection->m_sink->notify(&event);

    if (!m_initialized)
        return;

    DEBUG_AUDIO_GRAPH("Graph:" << int(this) << "create and connect audio sink after init:" << int(connection->m_sink->m_audioNode))
    AudioConnection startConnection(connection->m_source);
    createAndConnectAuNodesRecursive(&startConnection);
    
    if (!createAudioUnitsRecursive(&startConnection)){
        DEBUG_AUDIO_GRAPH("Graph" << int(this) << "could not update stream specification. Rebuild.")
        rebuildGraph();
    }  
}

void AudioGraph::disconnectLate(AudioConnection *connection)
{
    if (!m_initialized)
        return;

    DEBUG_AUDIO_GRAPH("Graph:" << int(this) << "disconnect audio sink after init:" << int(connection->m_sink->m_audioNode))

    if (!connection->disconnect(this)){
        DEBUG_AUDIO_GRAPH("Graph" << int(this) << "could not disconnect audio sink. Rebuild.")
        rebuildGraph();
    }
}

void AudioGraph::update()
{
    if (!m_initialized || !m_audioGraphRef)
        return;
    AUGraphUpdate(m_audioGraphRef, 0);
    tryStartGraph();
}

int AudioGraph::nodeCount()
{
    if (!m_audioGraphRef)
        return 0;
    UInt32 count;
    AUGraphGetNodeCount(m_audioGraphRef, &count);
    return int(count);
}

void AudioGraph::start()
{
    // Start does not mean 'start to play
    // music'. It means 'prepare to receive
    // audio from the player units'.
    DEBUG_AUDIO_GRAPH("Graph" << int(this) << "asked to start (cannot play:" << m_graphCannotPlay << ")")
    m_startedLogically = true;
    
    if (m_graphCannotPlay)
        return;
        
    if (!m_initialized)
        rebuildGraph();

    if (!m_graphCannotPlay)
        tryStartGraph();
}

void AudioGraph::stop()
{
    DEBUG_AUDIO_GRAPH("Graph" << int(this) << "asked to stop")
    if (m_audioGraphRef)
	    AUGraphStop(m_audioGraphRef);
    m_startedLogically = false;
}

}} // namespace Phonon::QT7
