/******************************************************************
 *
 * kdbgwin - Helper application for DrKonqi
 *
 * This file is part of the KDE project
 *
 * Copyright (C) 2010 Ilie Halip <lupuroshu@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>
 *****************************************************************/

#include "msvc_generator.h"
#include "mingw_generator.h"
#include "outputters.h"
#include "process.h"
#include "suggester.h"
#include <QApplication>

int main(int argc, char *argv[], char *envp[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("kdbgwin");

    if (argc != 3)
    {
        kFatal() << "Parameters are incorrect";
        return -1;
    }

    if (!Process::EnableDebugPrivilege())
    {
        kFatal() << "Cannot enable debug privilege, exiting";
        return -1;
    }

    // ok, argv[1] is the pid of the failing process,
    // and argv[2] the current thread id - let's get the info we need
    Process proc;
    if (!proc.GetInfo(argv[1], argv[2]))
    {
        kFatal() << "Cannot attach to process, exiting";
        return -1;
    }

#if defined(_MSC_VER)
    MsvcGenerator generator(proc);
#else
#if defined(__GNUG__)
    MingwGenerator generator(proc);
#endif
#endif
    Outputter outputter;
    PackageSuggester suggester;

    QObject::connect
    (
        &generator,
        SIGNAL(DebugLine(const QString&)),
        &outputter,
        SLOT(OnDebugLine(const QString&))
    );
    QObject::connect
    (
        &generator,
        SIGNAL(MissingSymbol(const QString&)),
        &suggester,
        SLOT(OnMissingSymbol(const QString&))
    );
    QObject::connect
    (
        &generator,
        SIGNAL(Finished()),
        &suggester,
        SLOT(OnFinished())
    );

    TThreadsMap::const_iterator it;
    for (it = proc.GetThreads().constBegin(); it != proc.GetThreads().constEnd(); it++)
    {
        generator.Run(it.value(), (it.key() == proc.GetThreadId())? true : false);
    }
}
