/*
Copyright (C) 2011  Smit Shah <Who828@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "writebackjob.h"

Nepomuk::WritebackJob::WritebackJob(QObject *parent)
    : KJob(parent)
{
}

Nepomuk::WritebackJob::~WritebackJob()
{
    qDeleteAll(m_plugins);
}


void Nepomuk::WritebackJob::start()
{
    Q_ASSERT(!m_plugins.isEmpty());
    Q_ASSERT(m_resource.isValid());
    tryNextPlugin();
}

void Nepomuk::WritebackJob::setPlugins(const QList<WritebackPlugin*>& plugins)
{
    qDeleteAll(m_plugins);
    m_plugins = plugins;
}

void Nepomuk::WritebackJob::setResource(const Nepomuk::Resource &resource)
{
    m_resource = resource;
}

void Nepomuk::WritebackJob::tryNextPlugin()
{
    WritebackPlugin* p = m_plugins.takeFirst();
    connect(p, SIGNAL(finished(Nepomuk::WritebackPlugin*,bool)),
            this, SLOT(slotWritebackFinished(Nepomuk::WritebackPlugin*)));
    p->writeback(m_resource.resourceUri());
}

void Nepomuk::WritebackJob::slotWritebackFinished(Nepomuk::WritebackPlugin *plugin)
{
    plugin->deleteLater();
    if (!m_plugins.isEmpty()) {
        tryNextPlugin();
    }
    else {
        emitResult();
    }
}

#include "writebackjob.moc"
