/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "fallbackcomponent.h"

#include <QDir>
#include <QFile>

#include <KStandardDirs>
#include <KDebug>


FallbackComponent::FallbackComponent(QObject *parent)
    : QObject(parent)
{
}

QString FallbackComponent::basePath() const
{
    return m_basePath;
}

void FallbackComponent::setBasePath(const QString &basePath)
{
    if (basePath != m_basePath) {
        m_basePath = basePath;
        emit onBasePathChanged();
    }
}

QStringList FallbackComponent::candidates() const
{
    return m_candidates;
}

void FallbackComponent::setCandidates(const QStringList &candidates)
{
    m_candidates = candidates;
    emit onCandidatesChanged();
}

QString FallbackComponent::filePath(const QString &key)
{
    QString resolved;

    foreach (const QString &path, m_candidates) {
        kDebug() << "Searching for!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << path;
        if (m_possiblePaths.contains(key)) {
            resolved = *m_possiblePaths.object(key);
            if (!resolved.isEmpty()) {
                break;
            } else {
                continue;
            }
        }

        QDir tmpPath(m_basePath);
        if (tmpPath.isAbsolute()) {
            resolved = m_basePath + path;
        } else {
            resolved = KStandardDirs::locate("data", m_basePath + '/' + path);
        }

        m_possiblePaths.insert(key, new QString(resolved));
        if (!resolved.isEmpty()) {
            break;
        }
    }

    return resolved;
}

#include "fallbackcomponent.moc"
