/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) version 3.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "audiodeviceaccess.h"
#include <klocale.h>

QDebug PS::operator<<(QDebug &s, const AudioDeviceAccess &a)
{
    s.nospace() << "deviceIds: " << a.m_deviceIds
        << "accessPreference: " << a.m_accessPreference
        << "driver " << a.driverName();
    if (a.m_capture) {
        s.nospace() << " capture";
    }
    if (a.m_playback) {
        s.nospace() << " playback";
    }
    return s;
}

const QString PS::AudioDeviceAccess::driverName() const
{
    switch (m_driver) {
    case InvalidDriver:
        return i18n("Invalid Driver");
    case AlsaDriver:
        return i18n("ALSA");
    case OssDriver:
        return i18n("OSS");
    case PulseAudioDriver:
        return i18n("PulseAudio");
    case JackdDriver:
        return i18n("Jack");
    case EsdDriver:
        return i18n("ESD");
    case ArtsDriver:
        return i18n("aRts");
    }
    return QString();
}
