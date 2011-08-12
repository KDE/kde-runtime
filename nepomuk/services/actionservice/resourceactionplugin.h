/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RESOURCEACTIONPLUGIN_H
#define RESOURCEACTIONPLUGIN_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QList>

#include "nepomukactions_export.h"

namespace Nepomuk {

/**
 * \class ResourceActionPlugin resourceactionplugin.h
 *
 * \brief Base class for plugins that provide actions for specific resource types.
 *
 * More documentation yet to be written...
 *
 * \author Sebastian Trueg <trueg@kde.org>, Laura Dragan <laura.dragan@deri.com>
 */
class NEPOMUKACTIONS_EXPORT ResourceActionPlugin : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    ResourceActionPlugin(QObject *parent = 0);

    /**
     * Destructor
     */
    virtual ~ResourceActionPlugin();

public:
    /**
     * By reimplementing this method plugins may check additional constraints on resources
     * like the presence of a contact in order for a chat to be started or the existance
     * of some required property.
     *
     * The default implementation simply returns \a true.
     */
    virtual bool canExecuteActionFor(const QString& actionId, const QList<QUrl>& subjectResources, const QList<QUrl>& objectResources);

    /**
     * Execute the actual action as specified by \p actionId on the resources specified in \p subjectResources and
     * \p objectResources.
     *
     * Each plugin needs to implement this method.
     */
    virtual bool executeAction(const QString& actionId, const QList<QUrl>& subjectResources, const QList<QUrl>& objectResources) = 0;

private:
    class Private;
    Private* const d;
};
}

#endif // RESOURCEACTIONPLUGIN_H
