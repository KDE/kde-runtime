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

#ifndef PHONON_BACKEND_H
#define PHONON_BACKEND_H

#include <phonon/objectdescription.h>
#include <phonon/backendinterface.h>
#include <phonon/phononnamespace.h>

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>

#include "backendnode.h" //for ComPointer

//windows declaractions
struct IMoniker;

namespace Phonon
{
    namespace DS9
    {
        class AudioOutput;

        class Backend : public QObject, public BackendInterface
        {
            Q_OBJECT
                Q_INTERFACES(Phonon::BackendInterface)
        public:
            Backend(QObject *parent = 0, const QStringList & = QStringList());
            virtual ~Backend();

            QObject *createObject(BackendInterface::Class, QObject *parent, const QList<QVariant> &args);

            bool supportsVideo() const;
            QStringList availableMimeTypes() const;

            QList<int> objectDescriptionIndexes(ObjectDescriptionType type) const;
            QHash<QByteArray, QVariant> objectDescriptionProperties(ObjectDescriptionType type, int index) const;

            bool connectNodes(QObject *, QObject *);
            bool disconnectNodes(QObject *, QObject *);

            //transaction management
            bool startConnectionChange(QSet<QObject *>);
            bool endConnectionChange(QSet<QObject *>);

            ComPointer<IMoniker> getAudioOutputMoniker(int index) const;

        Q_SIGNALS:
            void objectDescriptionChanged(ObjectDescriptionType);

        private:
            mutable QVector<ComPointer<IMoniker> > m_audioOutput;
            mutable QVector<CLSID> m_audioEffects;
            QHash<MediaObject*, Phonon::State> m_graphState;
        };
    }
} // namespace Phonon::DS9

#endif // PHONON_BACKEND_H
