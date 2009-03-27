/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DRKONQI_GLOBALS_H
#define DRKONQI_GLOBALS_H

#include <KGuiItem>

/** This class provides a custom constructor to fill the "toolTip"
 * and "whatsThis" texts of KGuiItem with the same text.
 */
class KGuiItem2 : public KGuiItem
{
public:
    inline KGuiItem2(const QString &text, const KIcon &icon, const QString &toolTip)
        : KGuiItem(text,icon,toolTip,toolTip) {}
};

/* Urls are defined globally here, so that they can change easily */
#define KDE_BUGZILLA_URL "https://bugs.kde.org"
#define KDE_BUGZILLA_CREATE_ACCOUNT_URL "https://bugs.kde.org/createaccount.cgi"
#define KDE_BUGZILLA_SHORT_URL "bugs.kde.org"
#define TECHBASE_HOWTO_DOC "http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports#Preparing_your_KDE_packages"

#endif
