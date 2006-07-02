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

#include <QString>
#include <kaboutdata.h>

#include <QtDBus/QtDBus>

/**
 * Provides information about a crashed process over dcop.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KrashAdaptor : public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.Krash")

  public:
    KrashAdaptor(QObject *parent);
    virtual ~KrashAdaptor();

  public slots:
    QString programName();
    QByteArray appName();
    int signalNumber();
    int pid();
    bool startedByKdeinit();
    bool safeMode();
    QString signalName();
    QString signalText();
    QString whatToDoText();
    QString errorDescriptionText();

    Q_NOREPLY void registerDebuggingApplication(const QString& launchName);

  signals:
    void acceptDebuggingApplication();
};

#endif
