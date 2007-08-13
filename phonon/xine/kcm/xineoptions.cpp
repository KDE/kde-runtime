/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "xineoptions.h"
#include <kgenericfactory.h>
#include <kconfiggroup.h>

K_EXPORT_COMPONENT_FACTORY(kcm_phononxine, KGenericFactory<XineOptions>("kcm_phononxine"))

XineOptions::XineOptions(QWidget *parent, const QStringList &args)
    : KCModule(KGenericFactory<XineOptions>::componentData(), parent, args)
{
    setupUi(this);

    m_config = KSharedConfig::openConfig("xinebackendrc");
    connect(m_ossCheckbox, SIGNAL(toggled(bool)), SLOT(changed()));
    load();
}

void XineOptions::load()
{
    KConfigGroup cg(m_config, "Settings");
    m_ossCheckbox->setChecked(cg.readEntry("showOssDevices", false));
}

void XineOptions::save()
{
    KConfigGroup cg(m_config, "Settings");
    cg.writeEntry("showOssDevices", m_ossCheckbox->isChecked());
}

void XineOptions::defaults()
{
    m_ossCheckbox->setChecked(false);
}

#include "xineoptions.moc"
// vim: sw=4 sts=4 et tw=100
