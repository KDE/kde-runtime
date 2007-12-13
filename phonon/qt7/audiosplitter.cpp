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

#include "audiosplitter.h"

namespace Phonon
{
namespace QT7
{

AudioNodeSplitter::AudioNodeSplitter() : AudioNode(1, 2)
{
}

ComponentDescription AudioNodeSplitter::getAudioNodeDescription()
{
	ComponentDescription description;
	description.componentType = kAudioUnitType_FormatConverter;
	description.componentSubType = kAudioUnitSubType_Splitter;
	description.componentManufacturer = kAudioUnitManufacturer_Apple;
	description.componentFlags = 0;
	description.componentFlagsMask = 0;
    return description;
}

AudioSplitter::AudioSplitter(QObject *parent) : MediaNode(AudioSink | AudioSource, new AudioNodeSplitter(), parent)
{
}

AudioSplitter::~AudioSplitter()
{
}

}} //namespace Phonon::QT7

