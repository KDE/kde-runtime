/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * krashdcopinterface.h
 *
 * Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#ifndef KRASHDCOPINTERFACE_SKEL
#define KRASHDCOPINTERFACE_SKEL

#include <dcopobject.h>

#include <qstring.h>
#include <qcstring.h>
#include <kaboutdata.h>

/**
 * Provides information about a crashed process over dcop.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KrashDCOPInterface : virtual public DCOPObject
{
  K_DCOP
public:

k_dcop:
  virtual QString programName() const = 0;
  virtual QCString appName() const = 0;
  virtual int signalNumber() const = 0;
  virtual int pid() const = 0;
  virtual bool startedByKdeinit() const = 0;
  virtual bool safeMode() const = 0;
  virtual QString signalName() const = 0;
  virtual QString signalText() const = 0;
  virtual QString whatToDoText() const = 0;
  virtual QString errorDescriptionText() const = 0;

  virtual ASYNC registerDebuggingApplication(const QString& launchName) = 0;

k_dcop_signals:
  void acceptDebuggingApplication();
};

#endif
