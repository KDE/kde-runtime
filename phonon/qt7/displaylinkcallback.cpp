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

#include "displaylinkcallback.h"
#include "backendheader.h"
#include "mediaobject.h"
#include <QMutexLocker>

namespace Phonon
{
namespace QT7
{

static CVDisplayLinkRef gDisplayLink = 0;
Q_GLOBAL_STATIC(QMutex, gDisplayLinkMutex);
Q_GLOBAL_STATIC(QList<DisplayLinkCallback*>, gDisplayLinkCallbacks);

static CVReturn displayLinkCallback(CVDisplayLinkRef /*displayLink*/,
								 const CVTimeStamp */*inNow*/,
								 const CVTimeStamp *inOutputTime,
								 CVOptionFlags /*flagsIn*/,
								 CVOptionFlags */*flagsOut*/,
                                 void *displayLinkContext)
{
    DisplayLinkCallback *callback = static_cast<DisplayLinkCallback *>(displayLinkContext);
    callback->handleCallback(inOutputTime);
    return kCVReturnSuccess;
}

void DisplayLinkCallback::handleCallback(const CVTimeStamp *outputTime)
{
    QMutexLocker guard(gDisplayLinkMutex());
    for (int i=0; i<gDisplayLinkCallbacks()->size(); ++i)
        gDisplayLinkCallbacks()->at(i)->mediaObject->displayLinkCallback(*outputTime);
}

DisplayLinkCallback::DisplayLinkCallback(MediaObject *aMediaObject) : QObject(0), mediaObject(aMediaObject)
{
    QMutexLocker guard(gDisplayLinkMutex());

    gDisplayLinkCallbacks()->append(this);
    if (gDisplayLink)
        return;

    OSStatus err = EnterMovies();
    BACKEND_ASSERT2(err == noErr, "Could not enter movies in QuickTime", FATAL_ERROR)
    err = CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &gDisplayLink);
	BACKEND_ASSERT2(err == noErr, "Could not create display link", FATAL_ERROR)
    err = CVDisplayLinkSetCurrentCGDisplay(gDisplayLink, kCGDirectMainDisplay);
	BACKEND_ASSERT2(err == noErr, "Could not set the current display for the display link", FATAL_ERROR)
    err = CVDisplayLinkSetOutputCallback(gDisplayLink, displayLinkCallback, 0);
	BACKEND_ASSERT2(err == noErr, "Could not set the callback for the display link", FATAL_ERROR)
    err = CVDisplayLinkStart(gDisplayLink);
	BACKEND_ASSERT2(err == noErr, "Could not start the display link", FATAL_ERROR)
}

DisplayLinkCallback::~DisplayLinkCallback()
{
    // NB. The library might have been unloaded and
    // left my statics to zero. So I have to check:
    if (gDisplayLinkMutex()){
        gDisplayLinkMutex()->lock();
            gDisplayLinkCallbacks()->removeOne(this);
        gDisplayLinkMutex()->unlock();

        // Be careful not to lock below, because QT locks the
        // callback thread to protect itself from e.g.
        // CVDisplayLinkStop while the callback is processing.
        // This might lead to deadlock.
        if (gDisplayLink && gDisplayLinkCallbacks()->isEmpty()){
            CVDisplayLinkStop(gDisplayLink);
            CFRelease(gDisplayLink);
            gDisplayLink = 0;
            ExitMovies();
        }
    }
}

}} // namespace Phonon::QT7
