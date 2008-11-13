/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/


#ifndef SINKITEM_H
#define SINKITEM_H

#include <Phonon/MediaNode>
#include <QtGui/QWidget>

using Phonon::MediaNode;

class SinkItem : public QWidget
{
    Q_OBJECT
    public:
        virtual MediaNode *mediaNode() = 0;
        virtual const MediaNode *mediaNode() const = 0;

    protected:
        SinkItem() {}
};

#endif // SINKITEM_H
