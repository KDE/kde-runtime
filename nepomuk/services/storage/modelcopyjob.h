/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _MODEL_COPY_JOB_H_
#define _MODEL_COPY_JOB_H_

#include <KJob>

#include <Soprano/StatementIterator>

namespace Soprano {
    class Model;
}

namespace Nepomuk {
    class ModelCopyJob : public KJob
    {
        Q_OBJECT

    public:
        ModelCopyJob( Soprano::Model* source, Soprano::Model* dest, QObject* parent = 0 );
        ~ModelCopyJob();

        Soprano::Model* source() const;
        Soprano::Model* dest() const;

    public Q_SLOTS:
        void start();

    private Q_SLOTS:
        void slotThreadFinished();

    private:
        bool doKill();

        class Private;
        Private* const d;
    };
}

#endif
