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

#include "audioeffect.h"
#include "common.h"
#include "audiooutput.h"
#include "backend.h"
#include "medianode.h"
#include "effectmanager.h"

#include <gst/gst.h>

namespace Phonon
{
namespace Gstreamer
{
AudioEffect::AudioEffect(Backend *backend, int effectId, QObject *parent)
        : QObject(parent),
        MediaNode(backend, AudioSink | AudioSource)
        , m_audioBin(0)
        , m_effectElement(0)
        , m_effect(0)
{

    static int count = 0;
    m_name = "Effect" + QString::number(count++);

    QList<AudioEffectInfo*> audioEffects = backend->effectManager()->audioEffects();
    if (effectId >= 0 && effectId < audioEffects.size()) {
        m_effect = audioEffects[effectId];
    } else {
        Q_ASSERT(0); // Effect ID out of range
    }
    m_audioBin = createAudioBin();
    if (m_audioBin) {
        setupEffectParams();
        gst_object_ref (m_audioBin); //keep bin alive
        m_isValid = true;
    }
}

AudioEffect::~AudioEffect()
{
    if (m_audioBin) {
        gst_element_set_state (m_audioBin, GST_STATE_NULL);
        gst_object_unref (m_audioBin);
    }
}

GstElement* AudioEffect::createAudioBin()
{
    GstElement *audioBin = gst_bin_new(NULL);

    //We need to queue to handle tee-connections from parent node
    GstElement *queue= gst_element_factory_make ("queue", NULL);
    gst_bin_add(GST_BIN(audioBin), queue);

    //### Not sure if audioconvert is required for audio processing elements
    GstElement *mconv= gst_element_factory_make ("audioconvert", NULL);
    gst_bin_add(GST_BIN(audioBin), mconv);

    m_effectElement = gst_element_factory_make (qPrintable(m_effect->name()), NULL);
    gst_bin_add(GST_BIN(audioBin), m_effectElement);

    //Link src pad
    GstPad *srcPad= gst_element_get_pad (m_effectElement, "src");
    gst_element_add_pad (audioBin, gst_ghost_pad_new ("src", srcPad));
    gst_object_unref (srcPad);

    //Link sink pad
    gst_element_link_many(queue, mconv, m_effectElement, NULL);
    GstPad *sinkpad = gst_element_get_pad (queue, "sink");
    gst_element_add_pad (audioBin, gst_ghost_pad_new ("sink", sinkpad));
    gst_object_unref (sinkpad);
    return audioBin;
}

void AudioEffect::setupEffectParams()
{
    //query and store parameters
    if (m_effectElement) {
        GParamSpec **property_specs;
        guint propertyCount, i;
        property_specs = g_object_class_list_properties(G_OBJECT_GET_CLASS (m_effectElement), &propertyCount);
        for (i = 0; i < propertyCount; i++) {
            GParamSpec *param = property_specs[i];
            if (param->flags & G_PARAM_WRITABLE) {
                //Only writable parameters that store numbers are accepted
                QString propertyName = g_param_spec_get_name (param);

                switch(param->value_type) {
                    case G_TYPE_UINT:
                        m_parameterList.append(Phonon::EffectParameter(i, propertyName,
                            0,   //hints
                            G_PARAM_SPEC_UINT(param)->default_value,
                            G_PARAM_SPEC_UINT(param)->minimum,
                            G_PARAM_SPEC_UINT(param)->maximum));
                        break;

                    case G_TYPE_INT:
                        m_parameterList.append(Phonon::EffectParameter(i, propertyName,
                            EffectParameter::IntegerHint,   //hints
                            QVariant(G_PARAM_SPEC_INT(param)->default_value),
                            QVariant(G_PARAM_SPEC_INT(param)->minimum),
                            QVariant(G_PARAM_SPEC_INT(param)->maximum)));
                        break;

                    case G_TYPE_FLOAT:
                        m_parameterList.append(Phonon::EffectParameter(i, propertyName,
                            0,   //hints
                            QVariant((double)G_PARAM_SPEC_FLOAT(param)->default_value),
                            QVariant((double)G_PARAM_SPEC_FLOAT(param)->minimum),
                            QVariant((double)G_PARAM_SPEC_FLOAT(param)->maximum)));
                        break;

                    case G_TYPE_DOUBLE:
                        m_parameterList.append(Phonon::EffectParameter(i, propertyName,
                            0,   //hints
                            QVariant(G_PARAM_SPEC_DOUBLE(param)->default_value),
                            QVariant(G_PARAM_SPEC_DOUBLE(param)->minimum),
                            QVariant(G_PARAM_SPEC_DOUBLE(param)->maximum)));
                        break;

                    default:
                        break;
                }
            }
        }
    }
}

QList<Phonon::EffectParameter> AudioEffect::parameters() const
{
    return m_parameterList;
}

QVariant AudioEffect::parameterValue(const EffectParameter &p) const
{
    Q_ASSERT(m_effectElement);

    QVariant returnVal;
    switch (p.type()) {
        case QVariant::Int:
            {
                gint val = 0;
                g_object_get(G_OBJECT(m_effectElement), qPrintable(p.name()), &val, NULL);
                returnVal = val;
            }
            break;

        case QVariant::Double:
            {
                float val = 0;
                g_object_get(G_OBJECT(m_effectElement), qPrintable(p.name()), &val, NULL);
                returnVal = QVariant((float)val);
            }
            break;

        default:
            Q_ASSERT(0); //not a supported variant type
    }
    return returnVal;
}


void AudioEffect::setParameterValue(const EffectParameter &p, const QVariant &v)
{
    Q_ASSERT(m_effectElement);

    // Note that the frontend currently calls this after creation with a null-value
    // for all parameters. This is most likely a bug...

    if (v.isValid()) {

        switch (p.type()) {
            // ### range values should really be checked by the front end, why isnt it working?
            case QVariant::Int:
                if (v.toInt() >= p.minimumValue().toInt() && v.toInt() <= p.maximumValue().toInt())
                    g_object_set(G_OBJECT(m_effectElement), qPrintable(p.name()), v.toInt(), NULL);
                break;

            case QVariant::Double:
                if (v.toDouble() >= p.minimumValue().toDouble() && v.toDouble() <= p.maximumValue().toDouble())
                    g_object_set(G_OBJECT(m_effectElement), qPrintable(p.name()), (float)v.toDouble(), NULL);
                break;

            case QVariant::UInt:
                if (v.toUInt() >= p.minimumValue().toUInt() && v.toUInt() <= p.maximumValue().toUInt())
                    g_object_set(G_OBJECT(m_effectElement), qPrintable(p.name()), (float)v.toUInt(), NULL);
                break;

            default:
                Q_ASSERT(0); //not a supported variant type
        }
    }
}

}
} //namespace Phonon::Gstreamer

#include "moc_audioeffect.cpp"
