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

#include "backend.h"
#include "backendnode.h"

#include "audiooutput.h"
#include "effect.h"
#include "mediaobject.h"
#include "videowidget.h"
#include "volumeeffect.h"

//windows specific (DirectX Media Object)
#include <dmo.h>

#include <QtCore/QSettings>
#include <QtCore/QSet>
#include <QtCore/QVariant>

#include <QtCore/QtPlugin>
// export as Qt/KDE factory as required
#ifdef PHONON_MAKE_QT_ONLY_BACKEND
Q_EXPORT_PLUGIN2(phonon_ds9, Phonon::DS9::Backend);
#else
#include <kpluginfactory.h>
K_PLUGIN_FACTORY(DS9BackendFactory, registerPlugin<Phonon::DS9::Backend>();)
K_EXPORT_PLUGIN(DS9BackendFactory("ds9backend"))
#endif

namespace Phonon
{
    namespace DS9
    {
        //helper function
        ComPointer<IMalloc> getIMalloc()
        {
            ComPointer<IMalloc> ret;
            CoGetMalloc(1, ret.pobject());
            return ret;
        }

        Backend::Backend(QObject *parent, const QVariantList &)
            : QObject(parent)
        {
        }

        Backend::~Backend()
        {
        }

        QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
        {
            switch (c)
            {
            case MediaObjectClass:
                return new MediaObject(parent);
            case AudioOutputClass:
                return new AudioOutput(this, parent);
            case EffectClass:
                return new Effect(m_audioEffects[ args[0].toInt() ], parent);
            case VideoWidgetClass:
                return new VideoWidget(qobject_cast<QWidget *>(parent));
            case VolumeFaderEffectClass:
                return new VolumeEffect(parent);
            default:
                return 0;
            }
        }

        bool Backend::supportsVideo() const
        {
            return VideoWidget::getVideoRenderer() != 0;
        }

        QStringList Backend::availableMimeTypes() const
        {
            QSet<QString> ret;
            HKEY key;

            for(int j = 0; j < 2; j++) {
                if (j == 0) {
                    ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Multimedia\\mplayer2\\mime types", 0, KEY_READ, &key);
                } else {
                    ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Multimedia\\wmplayer\\mime types", 0, KEY_READ, &key);
                }

                TCHAR name[255];
                DWORD nameSize = sizeof(name)/sizeof(TCHAR);
                FILETIME ft;
                for(int i=0; ::RegEnumKeyEx(key, i, name, &nameSize, NULL, NULL, NULL, &ft) == ERROR_SUCCESS; ++i) {
                    QString str = QT_WA_INLINE( QString::fromWCharArray(name), QString::fromLocal8Bit(reinterpret_cast<char*>(name)));
                    ret += str;
                    nameSize = sizeof(name)/sizeof(TCHAR);

                }

                ::RegCloseKey(key);
            }
            
            return ret.toList();
        }

        ComPointer<IMoniker> Backend::getAudioOutputMoniker(int index) const
        {
            ComPointer<IMoniker> ret;
            if (index >= 0) {
                ret = m_audioOutput[index];
            }
            return ret;
        }


        QList<int> Backend::objectDescriptionIndexes(ObjectDescriptionType type) const
        {
            QList<int> ret;
            ComPointer<ICreateDevEnum> devEnum;
            HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                IID_ICreateDevEnum, devEnum.pdata());
            if (FAILED(hr)) {
                return ret; //it is impossible to enumerate the devices
            }

            switch(type)
            {
            case Phonon::AudioOutputDeviceType:
                {
                    m_audioOutput.clear();
                    ComPointer<IEnumMoniker> enumMon;
                    hr = devEnum->CreateClassEnumerator(CLSID_AudioRendererCategory, enumMon.pobject(), 0);
                    if (FAILED(hr)) {
                        break;
                    }
                    ComPointer<IMoniker> mon;

                    //let's reorder the devices so that directshound appears first
                    int nbds = 0; //number of directsound devices

                    while (S_OK == enumMon->Next(1, mon.pobject(), 0)) {
                        LPOLESTR str = 0;
                        mon->GetDisplayName(0,0,&str);
                        if (SUCCEEDED(hr)) {
                            const QString name = QString::fromUtf16(str);
                            getIMalloc()->Free(str);
                            //that's the only way to test of the device is a DirectSound device
                            if (name.contains(QLatin1String("DirectSound"))) {
                                ret.insert(nbds++, m_audioOutput.count());
                            } else {
                                ret.append(m_audioOutput.count());
                            }
                            m_audioOutput.append(mon);
                        }
                    }
                    break;
                }
            case Phonon::EffectType:
                {
                    m_audioEffects.clear();
                    ComPointer<IEnumDMO> pEnumDMO;
                    hr = DMOEnum(DMOCATEGORY_AUDIO_EFFECT, DMO_ENUMF_INCLUDE_KEYED, 0, 0, 0, 0, pEnumDMO.pobject());
                    if (SUCCEEDED(hr)) {
                        CLSID clsid;
                        while (S_OK == pEnumDMO->Next(1, &clsid, 0, 0)) {
                            ret += m_audioEffects.count();
                            m_audioEffects.append(clsid);
                        }
                    }
                    break;
                }
                break;
            default:
                break;
            }
            return ret;
        }

        QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
        {
            QHash<QByteArray, QVariant> ret;
            switch (type)
            {
            case Phonon::AudioOutputDeviceType:
                {
                    const ComPointer<IMoniker> &mon = m_audioOutput[index];
                    LPOLESTR str = 0;
                    HRESULT hr = mon->GetDisplayName(0,0, &str);
                    if (SUCCEEDED(hr)) {
                        QString name = QString::fromUtf16(str); 
                        getIMalloc()->Free(str);
                        ret["name"] = name.mid(name.indexOf('\\') + 1);
                    }
                }
                break;
            case Phonon::EffectType:
                {
                    WCHAR name[80]; // 80 is clearly stated in the MSDN doc
                    HRESULT hr = DMOGetName(m_audioEffects[index], name);
                    if (SUCCEEDED(hr)) {
                        ret["name"] = QString::fromWCharArray(name);
                    }
                }
                break;
            }
            return ret;
        }

        bool Backend::endConnectionChange(QSet<QObject *> objects)
        {
            //end of a transaction
            HRESULT hr = E_FAIL;
            if (!objects.isEmpty()) {
                foreach(QObject *o, objects) {
                    if (BackendNode *node = qobject_cast<BackendNode*>(o)) {
                        MediaObject *mo = node->mediaObject();
                        if (mo && m_graphState.contains(mo)) {
                            switch(m_graphState[mo])
                            {
                            case Phonon::ErrorState:
                            case Phonon::StoppedState:
                                //nothing to do
                                break;
                            case Phonon::PausedState:
                                mo->pause();
                                break;
                            default:
                                mo->play();
                                break;
                            }
                            if (mo->state() != Phonon::ErrorState) {
                                hr = S_OK;
                            }
                            m_graphState.remove(mo);
                            break;
                        }
                    }
                }
            }
            return SUCCEEDED(hr);
        }


        bool Backend::startConnectionChange(QSet<QObject *> objects)
        {
            //start a transaction
            HRESULT hr = E_FAIL;

            //let's save the state of the graph (before we stop it)
            foreach(QObject *o, objects) {
                if (BackendNode *node = qobject_cast<BackendNode*>(o)) {
                    if (MediaObject *mo = node->mediaObject()) {
                        m_graphState[mo] = mo->state();
                        mo->stop(); //we have to stop the graph..
                        if (mo->state() != Phonon::ErrorState)
                            hr = S_OK;
                        break;
                    }
                }
            }
            return SUCCEEDED(hr);
        }

        bool Backend::connectNodes(QObject *_source, QObject *_sink)
        {
            BackendNode *source = qobject_cast<BackendNode*>(_source);
            if (!source) {
                return false;
            }
            BackendNode *sink = qobject_cast<BackendNode*>(_sink);
            if (!sink) {
                return false;
            }

            //setting the graph if needed
            if ((source->mediaObject() && sink->mediaObject() && source->mediaObject() != sink->mediaObject())
                || (source->mediaObject() == 0 && sink->mediaObject() == 0)) {
                //error: not in the same graph or no graph at all
                return false;
            } else if (source->mediaObject() && source->mediaObject() != sink->mediaObject()) {
                //this' graph becomes the common one
                source->mediaObject()->grabNode(sink);
            } else if (source->mediaObject() == 0) {
                //sink's graph becomes the common one
                sink->mediaObject()->grabNode(source);
            }

            return source->mediaObject()->connectNodes(source, sink);
        }

        bool Backend::disconnectNodes(QObject *_source, QObject *_sink)
        {
            BackendNode *source = qobject_cast<BackendNode*>(_source);
            if (!source) {
                return false;
            }
            BackendNode *sink = qobject_cast<BackendNode*>(_sink);
            if (!sink) {
                return false;
            }

            return source->mediaObject() == 0 ||
                source->mediaObject()->disconnectNodes(source, sink);
        }
    }
}

#include "moc_backend.cpp"
