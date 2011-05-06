/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef REMOVABLEDEVICEINDEXNOTIFICATION_H
#define REMOVABLEDEVICEINDEXNOTIFICATION_H

#include <KNotification>

#include <Solid/Device>

class RemovableDeviceIndexNotification : public KNotification
{
    Q_OBJECT

public:
    RemovableDeviceIndexNotification(const Solid::Device& dev, QObject *parent = 0);

private slots:
    void slotActionActivated(uint action);
    void slotActionDoIndexActivated();
    void slotActionDoNotIndexActivated();
    void slotActionDoIndexAllActivated();
    void slotActionDoNotIndexAnyActivated();
    void slotActionIgnoreActivated();

private:
    Solid::Device m_device;
};

#endif // REMOVABLEDEVICEINDEXNOTIFICATION_H
