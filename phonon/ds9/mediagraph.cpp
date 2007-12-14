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

#include "fakesource.h"
#include "iodevicereader.h"

#include "mediagraph.h"
#include "mediaobject.h"

#include <QtCore/QUrl>
#include <QtGui/QWidget>

#include <qnetwork.h>

//#define GRAPH_DEBUG

uint qHash (const Phonon::DS9::Filter &f)
{
    return uint(static_cast<IBaseFilter*>(f));
}

namespace Phonon
{
    namespace DS9
    {
        class DumbWidget;

        static const long WM_GRAPHNOTIFY = WM_APP + 1;
        static DumbWidget *common = 0;

        //this class is used to get the events and redirect them to the objects
        class DumbWidget : public QWidget
        {
        public:
            //makes sure it doesn't get counted on the top level widgets
            DumbWidget()
            {
            }

            bool winEvent(MSG *msg, long *result)
            {
                if (msg->message == WM_GRAPHNOTIFY) {
                    MediaGraph *mg = reinterpret_cast<MediaGraph*>(msg->lParam);
                    if (mg && activeGraphs.contains(mg)) {
                        mg->handleEvents();
                    }
                }
                return QWidget::winEvent(msg, result);
            }

            QSet<MediaGraph*> activeGraphs;
        };

        MediaGraph::MediaGraph(MediaObject *mo, short index) : m_fakeSource(new FakeSource()),
            m_hasVideo(false), m_hasAudio(false), m_connectionsDirty(false),
            m_mediaObject(mo), m_index(index), m_clockDelta(0)
        {
            //creates the graph
            HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, 
                CLSCTX_INPROC_SERVER, IID_IGraphBuilder, m_graph.pdata());
            if (m_mediaObject->catchComError(hr)) {
                return;
            }

            m_mediaControl = ComPointer<IMediaControl>(m_graph);
            Q_ASSERT(m_mediaControl);
            m_mediaSeeking = ComPointer<IMediaSeeking>(m_graph);
            Q_ASSERT(m_mediaSeeking);
            m_mediaEvent = ComPointer<IMediaEventEx>(m_graph);
            Q_ASSERT(m_mediaEvent);

            //event management
            if (common == 0) {
                common = new DumbWidget;
            }

            common->activeGraphs += this;

            //we route the events to the same widget
            hr = m_mediaEvent->SetNotifyWindow(reinterpret_cast<OAHWND>(common->winId()), WM_GRAPHNOTIFY,
                reinterpret_cast<LONG_PTR>(this) );
            if (m_mediaObject->catchComError(hr)) {
                return;
            }

            hr = m_graph->AddFilter(m_fakeSource, 0);
            if (m_mediaObject->catchComError(hr)) {
                return;
            }
        }

        MediaGraph::~MediaGraph()
        {
            common->activeGraphs -= this;
        }

        short MediaGraph::index() const
        {
            return m_index;
        }

        void MediaGraph::grabNode(BackendNode *node)
        {
            FILTER_INFO info;
            const Filter filter = node->filter(m_index);
            if (filter) {
                filter->QueryFilterInfo(&info);
                if (info.pGraph == 0) {
                    HRESULT hr = m_graph->AddFilter(filter, 0);
                    m_mediaObject->catchComError(hr);
                } else {
                    info.pGraph->Release();
                }
            }
        }

        void MediaGraph::ensureSourceDisconnected()
        {
            foreach(BackendNode *node, m_sinkConnections) {
                const Filter currentFilter = node->filter(m_index);
                foreach(const InputPin inpin, BackendNode::pins(currentFilter, PINDIR_INPUT)) {
                    foreach(const OutputPin outpin, BackendNode::pins(m_fakeSource, PINDIR_OUTPUT)) {
                        tryDisconnect(outpin, inpin);
                    }

                    foreach(const Filter &filter, m_decoders) {
                        const OutputPin outpin = BackendNode::pins(filter, PINDIR_OUTPUT).first();
                        tryDisconnect(outpin, inpin);
                    }
                }
            }
        }

        void MediaGraph::ensureSourceConnectedTo(bool force)
        {
            if (m_connectionsDirty == false && force == false) {
                return;
            }

            m_connectionsDirty = false;
            ensureSourceDisconnected();

            //reconnect the pins
            foreach(BackendNode *node, m_sinkConnections) {
                const Filter currentFilter = node->filter(m_index);
                foreach(const InputPin &inpin, BackendNode::pins(currentFilter, PINDIR_INPUT)) {
                    //we ensure the filter belongs to the graph
                    FILTER_INFO info;
                    currentFilter->QueryFilterInfo(&info);
                    if (info.pGraph == 0) {
                        m_graph->AddFilter(currentFilter, 0);
                    } else {
                        info.pGraph->Release();
                    }

                    foreach(const Filter &filter, m_decoders) {
                        //a decoder has only one output
                        const OutputPin outpin = BackendNode::pins(filter, PINDIR_OUTPUT).first();
                        if (tryConnect(outpin, inpin)) {
                            break;
                        }
                    }
                }
            }
        }

        QSet<Filter> MediaGraph::getAllFilters() const
        {
            QSet<Filter> ret;
            ComPointer<IEnumFilters> enumFilters;
            HRESULT hr = m_graph->EnumFilters(enumFilters.pobject());
            if (m_mediaObject->catchComError(hr)) {
                return ret;
            }
            Filter current;
            while( enumFilters->Next(1, current.pobject(), 0) == S_OK) {
                ret += current;
            }
            return ret;
        }


        bool MediaGraph::isSeekable() const
        {
            if (m_mediaSeeking == 0) {
                return false;
            }

            DWORD caps = AM_SEEKING_CanSeekAbsolute;
            HRESULT hr = m_mediaSeeking->CheckCapabilities(&caps);
            return SUCCEEDED(hr);
        }

        qint64 MediaGraph::totalTime() const
        {
            qint64 ret = -1;
            if (m_mediaSeeking) {
                HRESULT hr = m_mediaSeeking->GetDuration(&ret);
                if (FAILED(hr)) {
                    ret = currentTime(); //that's the least we know
                } else {
                    ret /= 10000; //convert to milliseconds
                }
            }
            return ret;
        }

        qint64 MediaGraph::currentTime() const
        {
            qint64 ret = -1;
            if (m_mediaSeeking) {
                HRESULT hr = m_mediaSeeking->GetCurrentPosition(&ret);
                if (FAILED(hr)) {
                    return ret;
                }
                ret -= m_clockDelta;
                ret /= 10000; //convert to milliseconds
            }
            return ret;
        }

        MediaSource MediaGraph::mediaSource() const
        {
            return m_mediaSource;
        }

        HRESULT MediaGraph::play()
        {
            removeFilter(m_fakeSource);
            ensureSourceConnectedTo();
            removeUnusedFilters();
            m_mediaControl->Run();
            return S_OK;
        }

        HRESULT MediaGraph::pause()
        {
            ensureSourceConnectedTo();
            removeUnusedFilters();
            return m_mediaControl->Pause();
        }

        HRESULT MediaGraph::stop()
        {
            HRESULT hr = m_mediaControl->Stop();
            restoreUnusedFilters();
            return hr;
        }

        HRESULT MediaGraph::seek(qint64 time)
        {
            qint64 newtime = time * 10000;
            HRESULT hr = m_mediaSeeking->SetPositions(&newtime, AM_SEEKING_AbsolutePositioning,
                0, AM_SEEKING_NoPositioning);
            if (SUCCEEDED(hr)) {
                m_clockDelta = 0;
            }
            return hr;
        }

        HRESULT MediaGraph::removeFilter(const Filter& filter)
        {
            FILTER_INFO info;
            filter->QueryFilterInfo(&info);
#ifdef GRAPH_DEBUG
            qDebug() << "removeFilter" << QString::fromUtf16(info.achName);
#endif
            if (info.pGraph) {
                info.pGraph->Release();
                return m_graph->RemoveFilter(filter);
            }

            //already removed
            return S_OK;
        }

        HRESULT MediaGraph::cleanup()
        {
            HRESULT hr = stop();
            if (FAILED(hr)) {
                return hr; //ensures it is stopped
            }

            ensureSourceDisconnected();

            QSet<Filter> list = m_decoders;
            if (m_demux) {
                list << m_demux;
            }
            if (m_realSource) {
                list << m_realSource;
            }

            foreach(const Filter &decoder, m_decoders) {
                list += getFilterChain(m_demux, decoder);
            }

            foreach(const Filter &filter, list) {
                hr = removeFilter(filter);
                if(FAILED(hr)) {
                    return hr;
                }
            }

            //Let's reinitialize the internal lists
            m_decoders.clear();
            m_demux = Filter();
            m_realSource = Filter();
            m_mediaSource = MediaSource();
            return S_OK;
        }


        bool MediaGraph::disconnectNodes(BackendNode *source, BackendNode *sink)
        {
            const Filter sinkFilter = sink->filter(m_index);
            const QList<InputPin> inputs = BackendNode::pins(sinkFilter, PINDIR_INPUT);

            QList<OutputPin> outputs;
            if (source == m_mediaObject) {
                outputs = BackendNode::pins(m_fakeSource, PINDIR_OUTPUT);
                foreach(const Filter dec, m_decoders) {
                    outputs += BackendNode::pins(dec, PINDIR_OUTPUT);
                }
            } else {
                outputs = BackendNode::pins(source->filter(m_index), PINDIR_OUTPUT);
            }


            foreach(InputPin inPin, inputs) {
                foreach(OutputPin outPin, outputs) {
                    tryDisconnect(outPin, inPin);
                }
            }

            if (m_sinkConnections.remove(sink)) {
                m_connectionsDirty = true;
            }
            return true;
        }

        bool MediaGraph::tryDisconnect(const OutputPin &out, const InputPin &in)
        {
            bool ret = false;

            OutputPin output;
            if (SUCCEEDED(in->ConnectedTo(output.pobject()))) {

                if (output == out) {
                    //we need a simple disconnection
                    ret = SUCCEEDED(out->Disconnect()) && SUCCEEDED(in->Disconnect());
                } else {
                    InputPin in2;
                    if (SUCCEEDED(out->ConnectedTo(in2.pobject()))) {
                        PIN_INFO info;
                        in2->QueryPinInfo(&info);
                        Filter tee(info.pFilter);
                        CLSID clsid;
                        tee->GetClassID(&clsid);
                        if (clsid == CLSID_InfTee) {
                            //we have to remove all intermediate filters between the tee and the sink
                            PIN_INFO info;
                            in->QueryPinInfo(&info);
                            Filter sink(info.pFilter);
                            QSet<Filter> list = getFilterChain(tee, sink);
                            list -= sink;
                            list -= tee;
                            out->QueryPinInfo(&info);
                            Filter source(info.pFilter);

                            if (list.isEmpty()) {
                                output->QueryPinInfo(&info);
                                if (Filter(info.pFilter) == tee) {
                                    ret = SUCCEEDED(output->Disconnect()) && SUCCEEDED(in->Disconnect());
                                }
                            } else {
                                ret = true;
                                foreach(Filter f, list) {
                                    ret = ret && SUCCEEDED(removeFilter(f));
                                }
                            }

                            //Let's try to see if the Tee filter is still useful
                            if (ret) {
                                int connections = 0;
                                foreach (OutputPin out, BackendNode::pins(tee, PINDIR_OUTPUT)) {
                                    InputPin p;
                                    if ( SUCCEEDED(out->ConnectedTo(p.pobject()))) {
                                        connections++;
                                    }
                                }
                                if (connections == 0) {
                                    //this avoids a crash if the filter is destroyed
                                    //by the subsequent call to removeFilter
                                    output = OutputPin();
                                    removeFilter(tee); //there is no more output for the tee, we remove it
                                }
                            }
                        }
                    }
                }
            }
            return ret;
        }

        bool MediaGraph::tryConnect(const OutputPin &out, const InputPin &newIn)
        {
            ///The management of the creation of the Tees is done here (this is the only place where we call IPin::Connect
            InputPin inPin;
            if (SUCCEEDED(out->ConnectedTo(inPin.pobject()))) {

                //the fake source has another mechanism for the connection
                if (BackendNode::pins(m_fakeSource, PINDIR_OUTPUT).contains(out)) {
                    return false;
                }

                //the output pin is already connected
                PIN_INFO info;
                inPin->QueryPinInfo(&info);
                Filter filter(info.pFilter); //this will ensure the interface is "Release"d
                CLSID clsid;
                filter->GetClassID(&clsid);
                if (clsid == CLSID_InfTee) {
                    //there is already a Tee (namely 'filter') in use
                    foreach(OutputPin pin, BackendNode::pins(filter, PINDIR_OUTPUT)) {
                        if (VFW_E_NOT_CONNECTED == pin->ConnectedTo(inPin.pobject())) {
                            return SUCCEEDED(pin->Connect(newIn, 0));
                        }
                    }

                    //we shoud never go here
                    return false;
                } else {
                    AM_MEDIA_TYPE type;
                    out->ConnectionMediaType(&type);

                    //first we disconnect the current connection (and we save the current media type)
                    if (!tryDisconnect(out, inPin)) {
                        return false;
                    }

                    //..then we try to connect the new node
                    if (SUCCEEDED(out->Connect(newIn, 0))) {

                        //we have to insert the Tee
                        if (!tryDisconnect(out, newIn)) {
                            return false;
                        }

                        Filter filter;
                        HRESULT hr = CoCreateInstance(CLSID_InfTee, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, filter.pdata());
                        if (m_mediaObject->catchComError(hr)) {
                            m_mediaObject->catchComError(m_graph->Connect(out, inPin));
                            return false;
                        }

                        if (m_mediaObject->catchComError(m_graph->AddFilter(filter, 0))) {
                            return false;
                        }


                        InputPin teeIn = BackendNode::pins(filter, PINDIR_INPUT).first(); //a Tee has always one input
                        hr = out->Connect(teeIn, &type);
                        if (FAILED(hr)) {
                            hr = m_graph->Connect(out, teeIn);
                        }
                        if (m_mediaObject->catchComError(hr)) {
                            m_mediaObject->catchComError(m_graph->Connect(out, inPin));
                            return false;
                        }

                        OutputPin teeOut = BackendNode::pins(filter, PINDIR_OUTPUT).last(); //the last is always the one that's not connected

                        hr = m_graph->Connect(teeOut, inPin);
                        if (m_mediaObject->catchComError(hr)) {
                            m_mediaObject->catchComError(m_graph->Connect(out, inPin));
                            return false;
                        }

                        teeOut = BackendNode::pins(filter, PINDIR_OUTPUT).last(); //the last is always the one that's not connected
                        if (m_mediaObject->catchComError(m_graph->Connect(teeOut, newIn))) {
                            m_mediaObject->catchComError(m_graph->Connect(out, inPin));
                            return false;
                        }

                        return true;
                    } else {
                        //we simply reconnect the pins as they
                        m_mediaObject->catchComError(m_graph->Connect(out, inPin));
                        return false;
                    }
                }

            } else {
                return SUCCEEDED(m_graph->Connect(out, newIn));
            }
        }

        bool MediaGraph::connectNodes(BackendNode *source, BackendNode *sink)
        {
            bool ret = false;
            const QList<InputPin> inputs = BackendNode::pins(sink->filter(m_index), PINDIR_INPUT);
            const QList<OutputPin> outputs = BackendNode::pins(source == m_mediaObject ? m_fakeSource : source->filter(m_index), PINDIR_OUTPUT);

#ifdef GRAPH_DEBUG
            qDebug() << Q_FUNC_INFO << source << sink << this;
#endif

            foreach(OutputPin outPin, outputs) {
                foreach(InputPin inPin, inputs) {
                    if (tryConnect(outPin, inPin)) {
                        //tell the sink node that it just got a new input
                        sink->connected(source, inPin);
                        ret = true;
                        if (source == m_mediaObject) {
                            m_connectionsDirty = true;
                            m_sinkConnections += sink;
#ifdef GRAPH_DEBUG
                            qDebug() << "found a sink connection" << sink << m_sinkConnections.count();
#endif
                        }
                        break;
                    }
                }
            }

            return ret;
        }


        HRESULT MediaGraph::loadSource(const MediaSource &source)
        {
            m_hasVideo = false;
            m_hasAudio = false;

            //we try to reset the clock here
            if (m_mediaSeeking) {
                m_mediaSeeking->GetCurrentPosition(&m_clockDelta);
            }


            //cleanup of the previous filters
            HRESULT hr = cleanup();
            if (FAILED(hr)) {
                return hr;
            }


            const QSet<Filter> previous = getAllFilters();
            switch (source.type())
            {
            case MediaSource::Disc:
                //TODO
                return hr;
            case MediaSource::Invalid:
                return hr;
            case MediaSource::Url:
                if (source.url().isValid()) {
                    hr = m_graph->RenderFile(source.url().toString().utf16(), 0);
                    if (FAILED(hr)) {
                        return hr;
                    }
                }
                break;
            case MediaSource::LocalFile:
                if (!source.fileName().isEmpty()) {
                    hr = m_graph->RenderFile(source.fileName().utf16(), 0);
                    if (FAILED(hr)) {
                        return hr;
                    }
                }
                break;
            case MediaSource::Stream:
                {
                    IODeviceReader *rdr = new IODeviceReader(source);
                    HRESULT hr = m_graph->AddFilter(rdr, 0);
                    if (FAILED(hr)) {
                        return hr;
                    }

                    Filter f(rdr);
                    hr = m_graph->Render(BackendNode::pins(f, PINDIR_OUTPUT).first());
                    if (FAILED(hr)) {
                        return hr;
                    }
                }
                break;
            }

            const QSet<Filter> newlyCreated = getAllFilters() - previous;

            //we keep the source and all the way down to the decoders
            QSet<Filter> keptFilters;
            foreach(const Filter &filter, newlyCreated) {
                if (isSourceFilter(filter)) {
                    m_realSource = filter; //save the source filter
                    if (!m_demux ) {
                        m_demux = filter; //in the WMV case, the demuxer is the source filter itself
                    }
                    keptFilters << filter;
                } else if (isDecoderFilter(filter)) {
                    m_decoders += filter;
                    keptFilters << filter;
                } else if (isDemuxerFilter(filter)) {
                    m_demux = filter;
                    keptFilters << filter;
                }
            }

            //hack to make the unencoded wav file play
            ///we need to do something smarter that checks the output pins of the demuxer for unencoded streams.
            if (m_decoders.isEmpty() && m_demux &&
                BackendNode::pins(m_demux, PINDIR_OUTPUT).count() == 1) {
                m_decoders += m_demux;
            }

            foreach(const Filter &decoder, m_decoders) {
                keptFilters += getFilterChain(m_demux, decoder);
            }

            Q_ASSERT(m_realSource);
            //now we need to decide what are the filters we want to keep
            //usually these are the demuxer and the decoders

            foreach(const Filter &filter, newlyCreated - keptFilters) {
                hr = removeFilter(filter);
                if (FAILED(hr)) {
                    return hr;
                }
            }

            ensureSourceConnectedTo(true);
            hr = removeUselessDecoders();
            if (SUCCEEDED(hr)) {
                m_mediaSource = source;
            }

            return hr;
        }

        void MediaGraph::restoreUnusedFilters()
        {
#ifdef GRAPH_DEBUG
            qDebug() << Q_FUNC_INFO << m_unusedFilters;
#endif
            ComPointer<IGraphConfig> graphConfig(m_graph);
            foreach(const Filter filter, m_unusedFilters) {
                m_graph->AddFilter(filter, 0);
            }
            m_unusedFilters.clear();
        }

        void MediaGraph::removeUnusedFilters()
        {
            //called from play and pause to 'clean' the graph
            QSet<Filter> usedFilters;
            const Filter root = m_realSource ? m_realSource : m_fakeSource;
            usedFilters += root;
            usedFilters += connectedFilters( root);

            m_unusedFilters += getAllFilters() - usedFilters;
            ComPointer<IGraphConfig> graphConfig(m_graph);
            foreach(const Filter filter, m_unusedFilters) {
                FILTER_INFO info;
                filter->QueryFilterInfo(&info);
                if (info.pGraph) {
                    info.pGraph->Release();
                    graphConfig->RemoveFilterEx(filter, REMFILTERF_LEAVECONNECTED);
                }
            }
#ifdef GRAPH_DEBUG
            qDebug() << Q_FUNC_INFO << m_unusedFilters;
#endif
        }

        QSet<Filter> MediaGraph::connectedFilters(const Filter &filter)
        {
            QSet<Filter> ret;
            foreach(OutputPin out, BackendNode::pins(filter, PINDIR_OUTPUT)) {
                InputPin in;
                if (SUCCEEDED(out->ConnectedTo(in.pobject()))) {
                    PIN_INFO info;
                    in->QueryPinInfo(&info);
                    const Filter current(info.pFilter);
                    if (current) {
                        ret += current;
                        ret += connectedFilters(current);
                    }
                }
            }
            return ret;
        }

        HRESULT MediaGraph::removeUselessDecoders()
        {
            foreach(const Filter &decoder, m_decoders) {
                OutputPin pin = BackendNode::pins(decoder, PINDIR_OUTPUT).first();
                InputPin input;
                if (pin->ConnectedTo(input.pobject()) == VFW_E_NOT_CONNECTED) {
                    //"we should remove this filter from the graph
                    HRESULT hr = removeFilter(decoder);
                    if (FAILED(hr)) {
                        return hr;
                    }
                }
            }
            return S_OK;
        }

        //utility functions
        QSet<Filter> MediaGraph::getFilterChain(const Filter &source, const Filter &sink)
        {
            QSet<Filter> ret;
            Filter current = sink;
            while (current && BackendNode::pins(current, PINDIR_INPUT).count() == 1 && current != source) {
                ret += current;
                InputPin pin = BackendNode::pins(current, PINDIR_INPUT).first();
                current = Filter();
                OutputPin output;
                if (pin->ConnectedTo(output.pobject()) == S_OK) {
                    PIN_INFO info;
                    if (SUCCEEDED(output->QueryPinInfo(&info)) && info.pFilter) {
                        current = Filter(info.pFilter); //this will take care of releasing the interface pFilter
                    }
                }
            }
            if (current != source) {
                //the soruce and sink don't seem to be connected
                ret.clear();
            }
            return ret;
        }

        bool MediaGraph::isDecoderFilter(const Filter &filter)
        {
            if (filter == 0) {
                return false;
            }
#ifdef GRAPH_DEBUG
            {
                FILTER_INFO info;
                filter->QueryFilterInfo(&info);
                qDebug() << Q_FUNC_INFO << QString::fromUtf16(info.achName);
                if (info.pGraph) {
                    info.pGraph->Release();
                }
            }
#endif


            QList<InputPin> inputs = BackendNode::pins(filter, PINDIR_INPUT);
            QList<OutputPin> outputs = BackendNode::pins(filter, PINDIR_OUTPUT);

            //TODO: find a better way to detect if a node is a decoder
            if (inputs.count() == 0 || outputs.count() ==0) {
                return false;
            }

            //the input pin must be encoded data
            AM_MEDIA_TYPE type;
            HRESULT hr = inputs.first()->ConnectionMediaType(&type);
            if (FAILED(hr)) {
                return false;
            }

            if (type.majortype != MEDIATYPE_Video &&
                type.majortype != MEDIATYPE_Audio) {
                return false;
            }

            //...and the output must be decoded
            AM_MEDIA_TYPE type2;
            hr = outputs.first()->ConnectionMediaType(&type2);
            if (FAILED(hr)) {
                return false;
            }

            if (type.majortype != type2.majortype) {
                return false;
            }

            if (type.majortype == MEDIATYPE_Video) {
                m_hasVideo = true;
            } else {
                m_hasAudio = true;
            }

#ifdef GRAPH_DEBUG
            {
                FILTER_INFO info;
                filter->QueryFilterInfo(&info);
                qDebug() << "found a decoder filter" << QString::fromUtf16(info.achName);
                if (info.pGraph) {
                    info.pGraph->Release();
                }
            }
#endif

            return true;
        }

        bool MediaGraph::isSourceFilter(const Filter &filter) const
        {
            //a source filter is one that has no input
            return BackendNode::pins(filter, PINDIR_INPUT).isEmpty();
        }

        bool MediaGraph::isDemuxerFilter(const Filter &filter) const
        {
            QList<InputPin> inputs = BackendNode::pins(filter, PINDIR_INPUT);
            QList<OutputPin> outputs = BackendNode::pins(filter, PINDIR_OUTPUT);

#ifdef GRAPH_DEBUG
            {
                FILTER_INFO info;
                filter->QueryFilterInfo(&info);
                qDebug() << Q_FUNC_INFO << QString::fromUtf16(info.achName);
                if (info.pGraph) {
                    info.pGraph->Release();
                }
            }
#endif

            if (inputs.count() != 1 || outputs.count() == 0) {
                return false; //a demuxer has only one input
            }

            AM_MEDIA_TYPE type;
            HRESULT hr = inputs.first()->ConnectionMediaType(&type);
            if (FAILED(hr)) {
                return false;
            }

            if (type.majortype != MEDIATYPE_Stream) {
                return false;
            }

            foreach(const OutputPin &outpin, outputs) {
                AM_MEDIA_TYPE type;
                //for now we support only video and audio
                hr = outpin->ConnectionMediaType(&type);
                if (SUCCEEDED(hr) && 
                    type.majortype != MEDIATYPE_Video && type.majortype != MEDIATYPE_Audio) {
                    return false;
                }
            }
#ifdef GRAPH_DEBUG
            {
                FILTER_INFO info;
                filter->QueryFilterInfo(&info);
                qDebug() << "found a demuxer filter" << QString::fromUtf16(info.achName);
                if (info.pGraph) {
                    info.pGraph->Release();
                }
            }
#endif
            return true;
        }

        QMultiMap<QString, QString> MediaGraph::metadata() const
        {
            QMultiMap<QString, QString> ret;
            ComPointer<IAMMediaContent> mediaContent(m_demux, IID_IAMMediaContent);
            if (mediaContent) {
                //let's get the meta data
                BSTR str;
                HRESULT hr = mediaContent->get_AuthorName(&str);
                if (SUCCEEDED(hr)) {
                    ret.insert(QLatin1String("ARTIST"), QString::fromUtf16(str));
                    SysFreeString(str);
                }
                hr = mediaContent->get_Title(&str);
                if (SUCCEEDED(hr)) {
                    ret.insert(QLatin1String("TITLE"), QString::fromUtf16(str));
                    SysFreeString(str);
                }
                hr = mediaContent->get_Description(&str);
                if (SUCCEEDED(hr)) {
                    ret.insert(QLatin1String("DESCRIPTION"), QString::fromUtf16(str));
                    SysFreeString(str);
                }
                hr = mediaContent->get_Copyright(&str);
                if (SUCCEEDED(hr)) {
                    ret.insert(QLatin1String("COPYRIGHT"), QString::fromUtf16(str));
                    SysFreeString(str);
                }
                hr = mediaContent->get_MoreInfoText(&str);
                if (SUCCEEDED(hr)) {
                    ret.insert(QLatin1String("MOREINFO"), QString::fromUtf16(str));
                    SysFreeString(str);
                }
            }
            return ret;
        }

        void MediaGraph::handleEvents()
        {
            long eventCode, param1, param2;
            while (m_mediaEvent && SUCCEEDED(m_mediaEvent->GetEvent(&eventCode, &param1, &param2, 0))) {
                m_mediaObject->handleEvents(this, eventCode, param1);
                m_mediaEvent->FreeEventParams(eventCode, param1, param2);
            }
        }

        HRESULT MediaGraph::saveToFile(const QString &filepath) const
        {
            const WCHAR wszStreamName[] = L"ActiveMovieGraph";
            HRESULT hr;
            ComPointer<IStorage> pStorage;

            // First, create a document file that will hold the GRF file
            hr = StgCreateDocfile(filepath.utf16(),
                STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE |
                STGM_SHARE_EXCLUSIVE,
                0, pStorage.pobject());

            if (FAILED(hr)) {
                return hr;
            }

            // Next, create a stream to store.
            ComPointer<IStream> pStream;
            hr = pStorage->CreateStream(wszStreamName,
                STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                0, 0, pStream.pobject());

            if (FAILED(hr)) {
                return hr;
            }

            // The IpersistStream::Save method converts a stream into a persistent object.
            ComPointer<IPersistStream> pPersist(m_graph);
            hr = pPersist->Save(pStream, TRUE);
            if (SUCCEEDED(hr)) {
                hr = pStorage->Commit(STGC_DEFAULT);
            }

            return hr;
        }

    }
}
