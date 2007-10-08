/*  This file is part of the KDE project
    Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

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

#include "effect.h"
#include <klocale.h>
#include <QVariant>
#include "xineengine.h"
#include <QMutexLocker>

namespace Phonon
{
namespace Xine
{

AudioPort EffectXT::audioPort() const
{
    const_cast<EffectXT *>(this)->ensureInstance();
    Q_ASSERT(m_plugin);
    Q_ASSERT(m_plugin->audio_input);

    AudioPort ret;
    ret.d->port = m_plugin->audio_input[0];
    Q_ASSERT(ret.d->port);

    ret.d->dontDelete = true;
    return ret;
}

xine_post_out_t *EffectXT::audioOutputPort() const
{
    const_cast<EffectXT *>(this)->ensureInstance();
    xine_post_out_t *x = xine_post_output(m_plugin, "audio out");
    Q_ASSERT(x);
    return x;
}

void EffectXT::rewireTo(SourceNodeXT *source)
{
    if (!source->audioOutputPort()) {
        return;
    }
    ensureInstance();
    xine_post_in_t *x = xine_post_input(m_plugin, "audio in");
    Q_ASSERT(x);
    xine_post_wire(source->audioOutputPort(), x);
}

// lazy initialization
void EffectXT::ensureInstance()
{
    if (m_plugin) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    if (!m_plugin) {
        createInstance();
    }
    Q_ASSERT(m_plugin);
}

void EffectXT::createInstance()
{
    kDebug(610) << "m_pluginName =" << m_pluginName;
    xine_audio_port_t *audioPort = XineEngine::nullPort();
    Q_ASSERT(m_plugin == 0 && m_pluginApi == 0);
    if (!m_pluginName) {
        kWarning(610) << "tried to create invalid Effect";
        return;
    }
    m_plugin = xine_post_init(XineEngine::xine(), m_pluginName, 1, &audioPort, 0);
    xine_post_in_t *paraInput = xine_post_input(m_plugin, "parameters");
    if (!paraInput) {
        return;
    }
    Q_ASSERT(paraInput->type == XINE_POST_DATA_PARAMETERS);
    Q_ASSERT(paraInput->data);
    m_pluginApi = reinterpret_cast<xine_post_api_t *>(paraInput->data);
    if (!m_parameterList.isEmpty()) {
        return;
    }
    xine_post_api_descr_t *desc = m_pluginApi->get_param_descr();
    Q_ASSERT(0 == m_pluginParams);
    m_pluginParams = static_cast<char *>(malloc(desc->struct_size));
    m_pluginApi->get_parameters(m_plugin, m_pluginParams);
    for (int i = 0; desc->parameter[i].type != POST_PARAM_TYPE_LAST; ++i) {
        xine_post_api_parameter_t &p = desc->parameter[i];
        switch (p.type) {
        case POST_PARAM_TYPE_INT:          /* integer (or vector of integers)    */
            if (p.enum_values) {
                // it's an enum
                QVariantList values;
                for (int j = 0; p.enum_values[j]; ++j) {
                    values << QString::fromUtf8(p.enum_values[j]);
                }
                m_parameterList << EffectParameter(i, QString::fromUtf8(p.name), 0,
                        *reinterpret_cast<int *>(m_pluginParams + p.offset),
                        0, values.count() - 1, values, QString::fromUtf8(p.description));
            } else {
                m_parameterList << EffectParameter(i, p.name, EffectParameter::IntegerHint,
                        *reinterpret_cast<int *>(m_pluginParams + p.offset),
                        static_cast<int>(p.range_min), static_cast<int>(p.range_max), QVariantList(), p.description);
            }
            break;
        case POST_PARAM_TYPE_DOUBLE:       /* double (or vector of doubles)      */
            m_parameterList << EffectParameter(i, p.name, 0,
                    *reinterpret_cast<double *>(m_pluginParams + p.offset),
                    p.range_min, p.range_max, QVariantList(), p.description);
            break;
        case POST_PARAM_TYPE_CHAR:         /* char (or vector of chars = string) */
        case POST_PARAM_TYPE_STRING:       /* (char *), ASCIIZ                   */
        case POST_PARAM_TYPE_STRINGLIST:   /* (char **) list, NULL terminated    */
            kWarning(610) << "char/string/stringlist parameter '" << p.name << "' not supported.";
            break;
        case POST_PARAM_TYPE_BOOL:         /* integer (0 or 1)                   */
            m_parameterList << EffectParameter(i, p.name, EffectParameter::ToggledHint,
                    static_cast<bool>(*reinterpret_cast<int *>(m_pluginParams + p.offset)),
                    QVariant(), QVariant(), QVariantList(), p.description);
            break;
        case POST_PARAM_TYPE_LAST:         /* terminator of parameter list       */
        default:
            abort();
        }
    }
}

#define K_XT(type) (static_cast<type *>(SinkNode::threadSafeObject().data()))
Effect::Effect(int effectId, QObject *parent)
    : QObject(parent),
    SinkNode(new EffectXT(0)),
    SourceNode(K_XT(EffectXT))
{
    const char *const *postPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_AUDIO_FILTER);
    if (effectId >= 0x7F000000) {
        effectId -= 0x7F000000;
        for(int i = 0; postPlugins[i]; ++i) {
            if (i == effectId) {
                // found it
                K_XT(EffectXT)->m_pluginName = postPlugins[i];
                break;
            }
        }
    }
}

Effect::Effect(EffectXT *xt, QObject *parent)
    : QObject(parent), SinkNode(xt), SourceNode(xt)
{
}

EffectXT::~EffectXT()
{
    if (m_plugin) {
        xine_post_dispose(XineEngine::xine(), m_plugin);
        m_plugin = 0;
    }
    free(m_pluginParams);
    m_pluginParams = 0;
}

bool Effect::isValid() const
{
    return K_XT(const EffectXT)->m_pluginName != 0;
}

MediaStreamTypes Effect::inputMediaStreamTypes() const
{
    return Phonon::Xine::Audio;
}

MediaStreamTypes Effect::outputMediaStreamTypes() const
{
    return Phonon::Xine::Audio;
}

QList<EffectParameter> Effect::parameters() const
{
    const_cast<Effect *>(this)->ensureParametersReady();
    return K_XT(const EffectXT)->m_parameterList;
}

void Effect::ensureParametersReady()
{
    K_XT(EffectXT)->ensureInstance();
}

QVariant Effect::parameterValue(const EffectParameter &p) const
{
    const int parameterIndex = p.id();
    QMutexLocker lock(&K_XT(const EffectXT)->m_mutex);
    if (!K_XT(const EffectXT)->m_plugin || !K_XT(const EffectXT)->m_pluginApi) {
        return QVariant(); // invalid
    }

    xine_post_api_descr_t *desc = K_XT(const EffectXT)->m_pluginApi->get_param_descr();
    Q_ASSERT(K_XT(const EffectXT)->m_pluginParams);
    K_XT(const EffectXT)->m_pluginApi->get_parameters(K_XT(const EffectXT)->m_plugin, K_XT(const EffectXT)->m_pluginParams);
    int i = 0;
    for (; i < parameterIndex && desc->parameter[i].type != POST_PARAM_TYPE_LAST; ++i);
    if (i == parameterIndex) {
        xine_post_api_parameter_t &p = desc->parameter[i];
        switch (p.type) {
        case POST_PARAM_TYPE_INT:          /* integer (or vector of integers)    */
            return *reinterpret_cast<int *>(K_XT(const EffectXT)->m_pluginParams + p.offset);
        case POST_PARAM_TYPE_DOUBLE:       /* double (or vector of doubles)      */
            return *reinterpret_cast<double *>(K_XT(const EffectXT)->m_pluginParams + p.offset);
        case POST_PARAM_TYPE_CHAR:         /* char (or vector of chars = string) */
        case POST_PARAM_TYPE_STRING:       /* (char *), ASCIIZ                   */
        case POST_PARAM_TYPE_STRINGLIST:   /* (char **) list, NULL terminated    */
            kWarning(610) << "char/string/stringlist parameter '" << p.name << "' not supported.";
            return QVariant();
        case POST_PARAM_TYPE_BOOL:         /* integer (0 or 1)                   */
            return static_cast<bool>(*reinterpret_cast<int *>(K_XT(const EffectXT)->m_pluginParams + p.offset));
        case POST_PARAM_TYPE_LAST:         /* terminator of parameter list       */
            break;
        default:
            abort();
        }
    }
    kError(610) << "invalid parameterIndex passed to Effect::value";
    return QVariant();
}

void Effect::setParameterValue(const EffectParameter &p, const QVariant &newValue)
{
    const int parameterIndex = p.id();
    QMutexLocker lock(&K_XT(EffectXT)->m_mutex);
    if (!K_XT(EffectXT)->m_plugin || !K_XT(EffectXT)->m_pluginApi) {
        return;
    }

    xine_post_api_descr_t *desc = K_XT(EffectXT)->m_pluginApi->get_param_descr();
    Q_ASSERT(K_XT(EffectXT)->m_pluginParams);
    int i = 0;
    for (; i < parameterIndex && desc->parameter[i].type != POST_PARAM_TYPE_LAST; ++i);
    if (i == parameterIndex) {
        xine_post_api_parameter_t &p = desc->parameter[i];
        switch (p.type) {
        case POST_PARAM_TYPE_INT:          /* integer (or vector of integers)    */
            {
                int *value = reinterpret_cast<int *>(K_XT(EffectXT)->m_pluginParams + p.offset);
                *value = newValue.toInt();
            }
            break;
        case POST_PARAM_TYPE_DOUBLE:       /* double (or vector of doubles)      */
            {
                double *value = reinterpret_cast<double *>(K_XT(EffectXT)->m_pluginParams + p.offset);
                *value = newValue.toDouble();
            }
            break;
        case POST_PARAM_TYPE_CHAR:         /* char (or vector of chars = string) */
        case POST_PARAM_TYPE_STRING:       /* (char *), ASCIIZ                   */
        case POST_PARAM_TYPE_STRINGLIST:   /* (char **) list, NULL terminated    */
            kWarning(610) << "char/string/stringlist parameter '" << p.name << "' not supported.";
            return;
        case POST_PARAM_TYPE_BOOL:         /* integer (0 or 1)                   */
            {
               int *value = reinterpret_cast<int *>(K_XT(EffectXT)->m_pluginParams + p.offset);
               *value = newValue.toBool() ? 1 : 0;
            }
            break;
        case POST_PARAM_TYPE_LAST:         /* terminator of parameter list       */
            kError(610) << "invalid parameterIndex passed to Effect::setValue";
            break;
        default:
            abort();
        }
        K_XT(EffectXT)->m_pluginApi->set_parameters(K_XT(EffectXT)->m_plugin, K_XT(EffectXT)->m_pluginParams);
    } else {
        kError(610) << "invalid parameterIndex passed to Effect::setValue";
    }
}

void Effect::addParameter(const EffectParameter &p)
{
    K_XT(EffectXT)->m_parameterList << p;
}

#undef K_XT

}} //namespace Phonon::Xine

#include "moc_effect.cpp"
