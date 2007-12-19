/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "audioeffects.h"
#include <QtCore>

namespace Phonon
{
namespace QT7
{

AudioEffectAudioNode::AudioEffectAudioNode(int effectType, QString name, QString description)
    : AudioNode(1, 1), m_effectType(effectType), m_name(name), m_description(description)
{
}

ComponentDescription AudioEffectAudioNode::getAudioNodeDescription()
{
	ComponentDescription d;
	d.componentType = kAudioUnitType_Effect;
	d.componentSubType = m_effectType;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
    return d;
}

void AudioEffectAudioNode::initializeAudioUnit()
{
    // OSStatus err;
    // float value;
    // kDelayParam_DelayTime {0->2, 1}
    // value = 1;
    // err = AudioUnitSetParameter(m_audioUnit,
    //     kDelayParam_DelayTime, kAudioUnitScope_Global, 0, value, 0);
    //     BACKEND_ASSERT2(err == noErr, "Could not set delay time for delay effect.", NORMAL_ERROR)
}

///////////////////////////////////////////////////////////////////////

AudioEffect::AudioEffect(int effectType, QObject *parent)
    : MediaNode(AudioSink | AudioSource, 0, parent)
{
    switch (effectType){
    case kAudioUnitSubType_Delay:
        m_audioNode = new AudioEffectAudioNode(effectType, "Delay");
        break;
    case kAudioUnitSubType_LowPassFilter:
        m_audioNode = new AudioEffectAudioNode(effectType, "LowPassFilter");
        break;
    case kAudioUnitSubType_HighPassFilter:
        m_audioNode = new AudioEffectAudioNode(effectType, "HighPassFilter");
        break;
    case kAudioUnitSubType_BandPassFilter:
        m_audioNode = new AudioEffectAudioNode(effectType, "BandPassFilter");
        break;
    case kAudioUnitSubType_HighShelfFilter:
        m_audioNode = new AudioEffectAudioNode(effectType, "HighShelfFilter");
        break;
    case kAudioUnitSubType_LowShelfFilter:
        m_audioNode = new AudioEffectAudioNode(effectType, "LowShelfFilter");
        break;
    case kAudioUnitSubType_ParametricEQ:
        m_audioNode = new AudioEffectAudioNode(effectType, "ParametricEQ");
        break;
    case kAudioUnitSubType_GraphicEQ:
        m_audioNode = new AudioEffectAudioNode(effectType, "GraphicEQ");
        break;
    case kAudioUnitSubType_PeakLimiter:
        m_audioNode = new AudioEffectAudioNode(effectType, "PeakLimiter");
        break;
    case kAudioUnitSubType_DynamicsProcessor:
        m_audioNode = new AudioEffectAudioNode(effectType, "DynamicsProcessor");
        break;
    case kAudioUnitSubType_MultiBandCompressor:
        m_audioNode = new AudioEffectAudioNode(effectType, "MultiBandCompressor");
        break;
    case kAudioUnitSubType_MatrixReverb:
        m_audioNode = new AudioEffectAudioNode(effectType, "MatrixReverb");
        break;
    case kAudioUnitSubType_SampleDelay:
        m_audioNode = new AudioEffectAudioNode(effectType, "SampleDelay");
        break;
    case kAudioUnitSubType_Pitch:
        m_audioNode = new AudioEffectAudioNode(effectType, "Pitch");
        break;
    case kAudioUnitSubType_AUFilter:
        m_audioNode = new AudioEffectAudioNode(effectType, "AUFilter");
        break;
    case kAudioUnitSubType_NetSend:
        m_audioNode = new AudioEffectAudioNode(effectType, "NetSend");
        break;
    case FOUR_CHAR_CODE('dist'):
        m_audioNode = new AudioEffectAudioNode(effectType, "Distortion");
        break;
    case FOUR_CHAR_CODE('rogr'):
        m_audioNode = new AudioEffectAudioNode(effectType, "RogerBeep");
        break;
    default:
        m_audioNode = new AudioEffectAudioNode(effectType, "Delay");
        BACKEND_ASSERT2(false, "Could not create unknown audio effect.", NORMAL_ERROR)
    }
    setAudioNode(m_audioNode);
}

QList<Phonon::EffectParameter> AudioEffect::parameters() const
{
    QList<Phonon::EffectParameter> ret;
    return ret;
}

QString AudioEffect::name()
{
    return m_audioNode ? m_audioNode->m_name : QString();
}

QString AudioEffect::description()
{
    return m_audioNode ? m_audioNode->m_description : QString();
}

QVariant AudioEffect::parameterValue(const Phonon::EffectParameter &/*value*/) const
{
    return QVariant();
}

void AudioEffect::setParameterValue(const Phonon::EffectParameter &/*parameter*/, const QVariant &/*newValue*/)
{
}

bool AudioEffect::effectAwailable(int effectType)
{
	ComponentDescription d;
	d.componentType = kAudioUnitType_Effect;
	d.componentSubType = effectType;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
    return FindNextComponent(0, &d) != 0;
}

QList<int> AudioEffect::effectList()
{
    QList<int> effects;
    effects
    << kAudioUnitSubType_Delay
    << kAudioUnitSubType_LowPassFilter
    << kAudioUnitSubType_HighPassFilter
    << kAudioUnitSubType_BandPassFilter
    << kAudioUnitSubType_HighShelfFilter
    << kAudioUnitSubType_LowShelfFilter
    << kAudioUnitSubType_ParametricEQ
    << kAudioUnitSubType_GraphicEQ
    << kAudioUnitSubType_PeakLimiter
    << kAudioUnitSubType_DynamicsProcessor
    << kAudioUnitSubType_MultiBandCompressor
    << kAudioUnitSubType_MatrixReverb
    << kAudioUnitSubType_SampleDelay
    << kAudioUnitSubType_Pitch
    << kAudioUnitSubType_AUFilter
    << kAudioUnitSubType_NetSend;
    
    if (effectAwailable(FOUR_CHAR_CODE('dist')))
        effects << FOUR_CHAR_CODE('dist');
    if (effectAwailable(FOUR_CHAR_CODE('rogr')))
        effects << FOUR_CHAR_CODE('rogr');

    return effects;
}

}} //namespace Phonon::QT7

#include "moc_audioeffects.cpp"
