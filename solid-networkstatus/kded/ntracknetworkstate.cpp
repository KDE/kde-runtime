/*
    Copyright (c) 2010 Sune Vuorela <sune@debian.org>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

*/

#include "ntracknetworkstate.h"

#include "networkstatus.h"

#include <Solid/Networking>

#include <KDebug>

Solid::Networking::Status ntrackstate2solidstatus(QNTrackState state)
{
    kDebug( 1222 ) << "ntrackstate2solidstatus changed status: " << state;
    switch(state) {
        case NTRACK_STATE_UNKNOWN:
        case NTRACK_STATE_UNSET: {
            return Solid::Networking::Unknown;
        }
        case NTRACK_STATE_ONLINE: {
            return Solid::Networking::Connected;
        }
        case NTRACK_STATE_BLOCKED:
        case NTRACK_STATE_OFFLINE: {
            return Solid::Networking::Unconnected;
        }
    }
    //FAILSAFE
    return Solid::Networking::Unknown;
}

NtrackNetworkState::NtrackNetworkState( NetworkStatusModule* statusmodule ) : QObject(statusmodule),m_statusmodule(statusmodule)
{
    m_statusmodule->registerNetwork("ntrack", ntrackstate2solidstatus(QNtrack::instance()->networkState()),"ntrack backend");
    connect(QNtrack::instance(),SIGNAL(stateChanged(QNTrackState,QNTrackState)),this,SLOT(ntrackStateChangedSlot(QNTrackState,QNTrackState)));
}

NtrackNetworkState::~NtrackNetworkState()
{
}



void NtrackNetworkState::ntrackStateChangedSlot(QNTrackState /*oldstate*/ , QNTrackState newstate )
{
    kDebug( 1222 ) << "ntrack changed status: " << newstate;
    m_statusmodule->setNetworkStatus("ntrack",ntrackstate2solidstatus(newstate));
}


