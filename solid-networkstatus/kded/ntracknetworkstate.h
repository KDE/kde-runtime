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

#ifndef NTRACKNETWORKSTATE_H
#define NTRACKNETWORKSTATE_H
#ifdef HAVE_QNTRACK

#include <QObject>
#include <QNtrack.h>

class NetworkStatusModule;

class NtrackNetworkState : public QObject {
  Q_OBJECT
  public:
    NtrackNetworkState(NetworkStatusModule* statusmodule /*also parent in object hierachy*/);
    virtual ~NtrackNetworkState();
private Q_SLOTS:
    /**
     * A slot to register the new state as reported by the ntrack part of things
     */
    void ntrackStateChangedSlot(QNTrackState, QNTrackState newstate);
  private:
    NetworkStatusModule* m_statusmodule;
};

#endif // HAVE_QNTRACK
#endif // NTRACKNETWORKSTATE_H
