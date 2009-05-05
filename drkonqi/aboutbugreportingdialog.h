/*******************************************************************
* aboutbugreportingdialog.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#ifndef ABOUTBUGREPORTINGDIALOG__H
#define ABOUTBUGREPORTINGDIALOG__H

#include <KDialog>

class KTextBrowser;

class AboutBugReportingDialog: public KDialog
{
    Q_OBJECT

public:
    AboutBugReportingDialog(QWidget * parent = 0);
    void showSection(const QString&);

private Q_SLOTS:
    void handleInternalLinks(const QString&);

private:
    KTextBrowser * m_textBrowser;
};

#endif
