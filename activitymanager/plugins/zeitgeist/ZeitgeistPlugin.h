/*
 *   Copyright (C) 2011 Trever Fischer <tdfischer@fedoraproject.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZEITGEISTPLUGIN_H
#define ZEITGEISTPLUGIN_H

#include "../../Plugin.h"

namespace QtZeitgeist
{
    class Log;

    namespace DataModel
    {
        class Event;
    } // namespace DataModel

} // namespace QtZeitgeist

class ZeitgeistPlugin : public Plugin
{
    Q_OBJECT
    public:
        ZeitgeistPlugin(QObject *parent, const QVariantList &args);
        ~ZeitgeistPlugin();

        virtual void addEvents(const EventList &events);
        static ZeitgeistPlugin *self();
    private:
        static ZeitgeistPlugin *s_instance;
        QtZeitgeist::Log *m_log;
};

#endif // ZEITGEISTPLUGIN_H
