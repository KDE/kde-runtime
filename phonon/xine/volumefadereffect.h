/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#ifndef Phonon_XINE_VOLUMEFADEREFFECT_H
#define Phonon_XINE_VOLUMEFADEREFFECT_H

#include <QTime>
#include "effect.h"
#include <QList>

#include <xine.h>
#include <Phonon/VolumeFaderEffect>
#include <Phonon/VolumeFaderInterface>

namespace Phonon
{
namespace Xine
{
class VolumeFaderEffect;

struct PluginParameters
{
    Phonon::VolumeFaderEffect::FadeCurve fadeCurve;
    double currentVolume;
    double fadeTo;
    int fadeTime;
    PluginParameters(Phonon::VolumeFaderEffect::FadeCurve a, double b, double c, int d)
        : fadeCurve(a), currentVolume(b), fadeTo(c), fadeTime(d) {}
};

class VolumeFaderEffectXT : public EffectXT
{
    friend class VolumeFaderEffect;
    public:
        VolumeFaderEffectXT();
        virtual void createInstance();

    private:
        mutable PluginParameters m_parameters;
};

class VolumeFaderEffect : public Effect, public Phonon::VolumeFaderInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VolumeFaderInterface)
    public:
        VolumeFaderEffect(QObject *parent);
        ~VolumeFaderEffect();

        float volume() const;
        void setVolume(float volume);
        Phonon::VolumeFaderEffect::FadeCurve fadeCurve() const;
        void setFadeCurve(Phonon::VolumeFaderEffect::FadeCurve curve);
        void fadeTo(float volume, int fadeTime);

        QVariant parameterValue(const EffectParameter &p) const;
        void setParameterValue(const EffectParameter &p, const QVariant &newValue);

    private:
        void getParameters() const;
};
}} //namespace Phonon::Xine

// vim: sw=4 ts=4 tw=80
#endif // Phonon_XINE_VOLUMEFADEREFFECT_H
