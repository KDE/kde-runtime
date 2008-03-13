/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef Phonon_XINE_BACKEND_H
#define Phonon_XINE_BACKEND_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QStringList>
#include <QVariant>

#include <xine.h>
#include <xine/xineutils.h>

#include "xineengine.h"
#include <QObject>
#include <phonon/objectdescription.h>
#include <phonon/backendinterface.h>


namespace Phonon
{
namespace Xine
{

class Backend : public QObject, public BackendInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::BackendInterface)
    public:
        Backend(QObject *parent, const QVariantList &args);
        ~Backend();

        QObject *createObject(BackendInterface::Class, QObject *parent, const QList<QVariant> &args);

        Q_INVOKABLE bool supportsVideo() const;
        Q_INVOKABLE bool supportsOSD() const;
        Q_INVOKABLE bool supportsFourcc(quint32 fourcc) const;
        Q_INVOKABLE bool supportsSubtitles() const;

        Q_INVOKABLE void freeSoundcardDevices();

        QList<int> objectDescriptionIndexes(ObjectDescriptionType) const;
        QHash<QByteArray, QVariant> objectDescriptionProperties(ObjectDescriptionType, int) const;

        bool startConnectionChange(QSet<QObject *>);
        bool connectNodes(QObject *, QObject *);
        bool disconnectNodes(QObject *, QObject *);
        bool endConnectionChange(QSet<QObject *>);

    public slots:
        QStringList availableMimeTypes() const;

    signals:
        void objectDescriptionChanged(ObjectDescriptionType);

    private:
        mutable QStringList m_supportedMimeTypes;
};
}} // namespace Phonon::Xine

// vim: sw=4 ts=4 tw=80
#endif // Phonon_XINE_BACKEND_H
