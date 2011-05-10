/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include <QtCore/QString>
#include <QtCore/QMap>

#include <Soprano/BackendSettings>
#include <Soprano/FilterModel>


namespace Soprano {
    class Model;
    class Backend;
    class NRLModel;
}

class KJob;
class CrappyInferencer2;

namespace Nepomuk {
    class RemovableMediaModel;
    class ModelCopyJob;
    class DataManagementModel;
    class DataManagementAdaptor;
    class ClassAndPropertyTree;

    /**
     * Represents the main Nepomuk model. While it looks as if there could be more than
     * one instance of Repository there is only the one.
     *
     * Repository uses a whole stack of Soprano models to provide its functionality. The
     * following list shows the layering from top to bottom:
     *
     * \li The DataManagementModel provides the actual data modification interface. For this
     *     purpose it is exported via DBus.
     * \li The Soprano::NRLModel provides query prefix expansion and graph cleanup features
     *     that are required by the DMM.
     * \li The Soprano::Utils::SignalCacheModel is used to compact the several statementsAdded()
     *     and statementsRemoved() signals.
     * \li RemovableMediaModel is used to automatically convert the URLs of files
     *     on USB keys, network shares, and so on from and into mount-point independant URLs
     *     like nfs://<HOST>/<HOST-PATH>/local/path.ext.
     * \li CrappyInferencer2 keeps rdfs:subClassOf and nao:userVisible inference up-to-date.
     *
     * On construction it checks for and optionally performs conversion from an old repository
     * type (pre-Virtuoso times) and runs CrappyInferencer2::updateAllResources() which is
     * performed in a separate thread.
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class Repository : public Soprano::FilterModel
    {
        Q_OBJECT

    public:
        Repository( const QString& name );
        ~Repository();

        QString name() const { return m_name; }

        enum State {
            CLOSED,
            OPENING,
            OPEN
        };

        State state() const { return m_state; }

        QString usedSopranoBackend() const;

    public Q_SLOTS:
        /**
         * Will emit the opened signal
         */
        void open();
        void close();

        void updateInference();

    Q_SIGNALS:
        void opened( Repository*, bool success );

    private Q_SLOTS:
        void copyFinished( KJob* job );

    private:
        Soprano::BackendSettings readVirtuosoSettings() const;

        QString m_name;
        State m_state;

        Soprano::Model* m_model;
        Nepomuk::ClassAndPropertyTree* m_classAndPropertyTree;
        CrappyInferencer2* m_inferencer;
        RemovableMediaModel* m_removableStorageModel;
        DataManagementModel* m_dataManagementModel;
        Nepomuk::DataManagementAdaptor* m_dataManagementAdaptor;
        Soprano::NRLModel* m_nrlModel;
        const Soprano::Backend* m_backend;

        // only used during opening
        // ------------------------------------------
        ModelCopyJob* m_modelCopyJob;
        const Soprano::Backend* m_oldStorageBackend;
        QString m_oldStoragePath;

        // the base path for the data. Will contain subfolder:
        // "data" for the data
        QString m_basePath;
        // ------------------------------------------
    };

    typedef QMap<QString, Repository*> RepositoryMap;
}

#endif
