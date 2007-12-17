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

#ifndef PHONON_MEDIAOBJECT_H
#define PHONON_MEDIAOBJECT_H

#include <phonon/mediaobjectinterface.h>
#include <phonon/addoninterface.h>
#include <phonon/mediasource.h>

#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QObject>
#include <QtCore/QDate>
#include <QtCore/QBasicTimer>

#include "backendnode.h"
#include "mediagraph.h"

namespace Phonon
{
    namespace DS9
    {
        class VideoWidget;
        class AudioOutput;

        class MediaObject : public BackendNode, public MediaObjectInterface, public AddonInterface
        {
            friend class Stream;
            Q_OBJECT
                Q_INTERFACES(Phonon::MediaObjectInterface Phonon::AddonInterface)
        public:
            MediaObject(QObject *parent);
            ~MediaObject();
            Phonon::State state() const;
            bool hasVideo() const;
            bool isSeekable() const;
            qint64 currentTime() const;
            qint32 tickInterval() const;

            void setTickInterval(qint32 newTickInterval);
            void play();
            void pause();
            void stop();
            void seek(qint64 time);

            QString errorString() const;
            Phonon::ErrorType errorType() const;

            bool hasInterface(Interface) const { return false; }
            QVariant interfaceCall(Interface, int, const QList<QVariant> &) { return QVariant(); }

            qint64 totalTime() const;
            qint32 prefinishMark() const;
            void setPrefinishMark(qint32 newPrefinishMark);

            qint32 transitionTime() const;
            void setTransitionTime(qint32);

            qint64 remainingTime() const;

            MediaSource source() const;
            void setSource(const MediaSource &source);
            void setNextSource(const MediaSource &source);

            void handleEvents(MediaGraph *graph, long eventCode, long param1);

            //COM error management
            bool catchComError(HRESULT hr) const;

            void grabNode(BackendNode *node);
            bool connectNodes(BackendNode *source, BackendNode *sink);
            bool disconnectNodes(BackendNode *source, BackendNode *sink);


        private Q_SLOTS:
            void switchToNextSource();

        Q_SIGNALS:
            void stateChanged(Phonon::State newstate, Phonon::State oldstate);
            void tick(qint64 time);
            void metaDataChanged(QMultiMap<QString, QString>);
            void seekableChanged(bool);
            void hasVideoChanged(bool);
            void bufferStatus(int);

            // AddonInterface:
            void titleChanged(int);
            void availableTitlesChanged(int);
            void chapterChanged(int);
            void availableChaptersChanged(int);
            void angleChanged(int);
            void availableAnglesChanged(int);

            void finished();
            void prefinishMarkReached(qint32);
            void aboutToFinish();
            void totalTimeChanged(qint64 length) const;
            void currentSourceChanged(const MediaSource &);

        protected:
            void setState(State);
            void timerEvent(QTimerEvent *e);

        private:
            MediaGraph *currentGraph() const;
            MediaGraph *nextGraph() const;


            mutable QString m_errorString;
            mutable ErrorType m_errorType;

            State m_state;
            qint32 m_transitionTime;

            qint32 m_prefinishMark;

            QBasicTimer m_tickTimer;
            qint32 m_tickInterval;

            //the graph(s)
            QList<MediaGraph*> m_graphs;

            //...the videowidgets in the graph
            QSet<VideoWidget*> m_videoWidgets;
            QSet<AudioOutput*> m_audioOutputs;

            bool m_buffering;
        };
    }
} //namespace Phonon::DS9

#endif // PHONON_MEDIAOBJECT_H
