/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

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

#include "deviceserviceaction.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <klocale.h>
#include <kmacroexpander.h>
#include <krun.h>
#include <solid/storageaccess.h>
#include <solid/block.h>


class MacroExpander : public KMacroExpanderBase
{
public:
    MacroExpander(const Solid::Device &device)
        : KMacroExpanderBase('%'), m_device(device) {}

protected:
    virtual int expandEscapedMacro(const QString &str, int pos, QStringList &ret);

private:
    Solid::Device m_device;
};


DeviceServiceAction::DeviceServiceAction()
    : DeviceAction()
{
    DeviceAction::setIconName("dialog-cancel");
    DeviceAction::setLabel(i18n("Unknown"));
}

QString DeviceServiceAction::id() const
{
    if (m_service.isEmpty()) {
        return QString();
    } else {
        return "#Service:"+m_service.m_strName+m_service.m_strExec;
    }
}

void DeviceServiceAction::setIconName(const QString &icon)
{
    m_service.m_strIcon = icon;
    DeviceAction::setIconName(icon);
}

void DeviceServiceAction::setLabel(const QString &label)
{
    m_service.m_strName = label;
    DeviceAction::setLabel(label);
}

void DeviceServiceAction::execute(Solid::Device &device)
{
    QString exec = m_service.m_strExec;
    MacroExpander mx(device);

    if (!mx.expandMacrosShellQuote(exec)) {
        kWarning() << ", Syntax error:" << m_service.m_strExec ;
        return;
    }

    KRun::runCommand(exec, QString(), m_service.m_strIcon, 0);
}

void DeviceServiceAction::setService(KDesktopFileActions::Service service)
{
    DeviceAction::setIconName(service.m_strIcon);
    DeviceAction::setLabel(service.m_strName);

    m_service = service;
}

KDesktopFileActions::Service DeviceServiceAction::service() const
{
    return m_service;
}

int MacroExpander::expandEscapedMacro(const QString &str, int pos, QStringList &ret)
{
    uint option = str[pos+1].unicode();

    switch (option) {

    case 'f': // Filepath
    case 'F': // case insensitive
        if (m_device.is<Solid::StorageAccess>()) {
            ret << m_device.as<Solid::StorageAccess>()->filePath();
        } else {
            kWarning() << "DeviceServiceAction::execute: " << m_device.udi()
                       << " is not a StorageAccess device" << endl;
        }
        break;
    case 'd': // Device node
    case 'D': // case insensitive
        if (m_device.is<Solid::Block>()) {
            ret << m_device.as<Solid::Block>()->device();
        } else {
            kWarning() << "DeviceServiceAction::execute: " << m_device.udi()
                       << " is not a Block device" << endl;
        }
        break;
    case 'i': // UDI
    case 'I': // case insensitive
        ret << m_device.udi();
        break;
    case '%':
        ret = QStringList(QLatin1String("%"));
        break;
    default:
        return -2; // subst with same and skip
    }
    return 2;
}
