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

#include "effectitem.h"
#include <QtCore/QModelIndex>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListView>

#include <Phonon/BackendCapabilities>
#include <Phonon/EffectWidget>
#include "pathitem.h"

using Phonon::EffectWidget;

EffectItem::EffectItem(const EffectDescription &desc, PathItem *pathItem)
    : m_effect(desc)
{
    setupUi(desc);
    pathItem->path().insertEffect(&m_effect);
    pathItem->updateChildrenPositions();
}

EffectItem::EffectItem(const EffectDescription &desc)
    : m_effect(desc)
{
    setupUi(desc);
}

void EffectItem::setupUi(const EffectDescription &desc)
{
    QVBoxLayout *hlayout = new QVBoxLayout(this);
    hlayout->setMargin(0);

    QLabel *label = new QLabel(desc.description(), this);
    label->setWordWrap(true);
    hlayout->addWidget(label);
    EffectWidget *w = new EffectWidget(&m_effect, this);
    hlayout->addWidget(w);
}
