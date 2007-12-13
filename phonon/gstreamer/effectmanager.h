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

#ifndef EffectManager_H
#define EffectManager_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <gst/gst.h>

namespace Phonon
{
namespace Gstreamer
{
class Backend;
class EffectManager;

class AudioEffectInfo
{
public :
    AudioEffectInfo(const QString &name,
                    const QString &description,
                    const QString &author);

    QString name() const
    {
        return m_name;
    }
    QString description() const
    {
        return m_description;
    }
    QString author() const
    {
        return m_author;
    }
    QStringList properties() const
    {
        return m_properties;
    }
    void addProperty(QString propertyName)
    {
        m_properties.append(propertyName);
    }

private:
    QString m_name;
    QString m_description;
    QString m_author;
    QStringList m_properties;
};

class EffectManager : public QObject
{
    Q_OBJECT
public:
    EffectManager(Backend *parent);
    virtual ~EffectManager();
    const QList<AudioEffectInfo*> audioEffects() const;

private:
    Backend *m_backend;
    QList <AudioEffectInfo*> m_audioEffectList;
};
}
} // namespace Phonon::Gstreamer

#endif // EffectManager_H
