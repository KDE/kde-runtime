/*

Copyright 2008 Albert Astals Cid <aacid@kde.org>

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

#include "spellchecking.h"

#include <kgenericfactory.h>
#include <sonnet/configwidget.h>

#include <QBoxLayout>

K_PLUGIN_FACTORY(SpellFactory, registerPlugin<SonnetSpellCheckingModule>();)
K_EXPORT_PLUGIN(SpellFactory( "kcmspellchecking" ))

SonnetSpellCheckingModule::SonnetSpellCheckingModule(QWidget* parent, const QVariantList&):
    KCModule(SpellFactory::componentData(), parent)
{
  QBoxLayout *layout = new QVBoxLayout( this );
  m_config = new KConfig("sonnetrc");
  m_configWidget = new Sonnet::ConfigWidget( m_config, this );
  layout->addWidget(m_configWidget);
  connect(m_configWidget, SIGNAL(configChanged()), this, SLOT(changed()));
}

SonnetSpellCheckingModule::~SonnetSpellCheckingModule()
{
  delete m_config;
}

void SonnetSpellCheckingModule::save()
{
    m_configWidget->save();
}

void SonnetSpellCheckingModule::defaults()
{
    m_configWidget->slotDefault();
}

#include "spellchecking.moc"
