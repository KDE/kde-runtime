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

#include <xine.h>

Q_EXPORT_PLUGIN2(kcm_phononxine, KGenericFactory<XineOptions>("kcm_phononxine"))

XineOptions::XineOptions(QWidget *parent, const QStringList &args)
    : KCModule(KGenericFactory<XineOptions>::componentData(), parent, args)
{
    setupUi(this);

    m_config = KSharedConfig::openConfig("xinebackendrc");

    connect(m_ossCheckbox, SIGNAL(toggled(bool)), SLOT(changed()));
    connect(deinterlaceMediaList, SIGNAL(clicked(const QModelIndex &)), SLOT(changed()));
    connect(deinterlaceMethodList, SIGNAL(clicked(const QModelIndex &)), SLOT(changed()));

    QListWidgetItem *item = new QListWidgetItem(i18n("DVD"), deinterlaceMediaList);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::Checked);

    item = new QListWidgetItem(i18n("VCD"), deinterlaceMediaList);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::Unchecked);

    item = new QListWidgetItem(i18n("File"), deinterlaceMediaList);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::Unchecked);

    {
        xine_t *xine = xine_new();
        Q_ASSERT(xine);
        xine_init(xine);
        xine_video_port_t *nullVideoPort = xine_open_video_driver(xine, "auto", XINE_VISUAL_TYPE_NONE, 0);
        xine_post_t *deinterlacer = xine_post_init(xine, "tvtime", 1, 0, &nullVideoPort);
        Q_ASSERT(deinterlacer);
        xine_post_in_t *paraInput = xine_post_input(deinterlacer, "parameters");
        Q_ASSERT(paraInput);
        Q_ASSERT(paraInput->data);
        xine_post_api_t *api = reinterpret_cast<xine_post_api_t *>(paraInput->data);
        xine_post_api_descr_t *desc = api->get_param_descr();
        for (int i = 0; desc->parameter[i].type != POST_PARAM_TYPE_LAST; ++i) {
            xine_post_api_parameter_t &p = desc->parameter[i];
            switch (p.type) {
            case POST_PARAM_TYPE_INT:          /* integer (or vector of integers)    */
                if (0 == strcmp(p.name, "method") && p.enum_values) {
                    for (int j = 0; p.enum_values[j]; ++j) {
                        item = new QListWidgetItem(p.enum_values[j], deinterlaceMethodList);
                    }
                }
                break;
            case POST_PARAM_TYPE_DOUBLE:       /* double (or vector of doubles)      */
            case POST_PARAM_TYPE_CHAR:         /* char (or vector of chars = string) */
            case POST_PARAM_TYPE_STRING:       /* (char *), ASCIIZ                   */
            case POST_PARAM_TYPE_STRINGLIST:   /* (char **) list, NULL terminated    */
            case POST_PARAM_TYPE_BOOL:         /* integer (0 or 1)                   */
                break;
            case POST_PARAM_TYPE_LAST:         /* terminator of parameter list       */
            default:
                kFatal() << "invalid type";
            }
        }

        xine_post_dispose(xine, deinterlacer);
        xine_close_video_driver(xine, nullVideoPort);
        xine_exit(xine);
    }
    load();
}

void XineOptions::load()
{
    KConfigGroup cg(m_config, "Settings");
    m_ossCheckbox->setChecked(cg.readEntry("showOssDevices", false));
    deinterlaceMediaList->item(0)->setCheckState(cg.readEntry("deinterlaceDVD", true) ? Qt::Checked : Qt::Unchecked);
    deinterlaceMediaList->item(1)->setCheckState(cg.readEntry("deinterlaceVCD", false) ? Qt::Checked : Qt::Unchecked);
    deinterlaceMediaList->item(2)->setCheckState(cg.readEntry("deinterlaceFile", false) ? Qt::Checked : Qt::Unchecked);
    deinterlaceMethodList->setCurrentRow(cg.readEntry("deinterlaceMethod", 0));
}

void XineOptions::save()
{
    KConfigGroup cg(m_config, "Settings");
    cg.writeEntry("showOssDevices", m_ossCheckbox->isChecked());
    cg.writeEntry("deinterlaceDVD", deinterlaceMediaList->item(0)->checkState() == Qt::Checked);
    cg.writeEntry("deinterlaceVCD", deinterlaceMediaList->item(1)->checkState() == Qt::Checked);
    cg.writeEntry("deinterlaceFile", deinterlaceMediaList->item(2)->checkState() == Qt::Checked);
    cg.writeEntry("deinterlaceMethod", deinterlaceMethodList->currentRow());
}

void XineOptions::defaults()
{
    m_ossCheckbox->setChecked(false);
    deinterlaceMediaList->item(0)->setCheckState(Qt::Checked);
    deinterlaceMediaList->item(1)->setCheckState(Qt::Unchecked);
    deinterlaceMediaList->item(2)->setCheckState(Qt::Unchecked);
    deinterlaceMethodList->setCurrentRow(0);
}

#include "xineoptions.moc"
// vim: sw=4 sts=4 et tw=100
