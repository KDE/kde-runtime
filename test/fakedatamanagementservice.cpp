/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "fakedatamanagementservice.h"
#include "../datamanagementmodel.h"
#include "../datamanagementadaptor.h"
#include "../classandpropertytree.h"

#include <Soprano/Soprano>
#include <Soprano/Server/DBusExportModel>
#include <Soprano/Graph>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>

#include <ktempdir.h>
#include <KDebug>

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KCmdLineOptions>

#include <QtDBus>

#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NCO>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Variant>
#include <Nepomuk/ResourceManager>

#include <signal.h>
#include <stdio.h>

using namespace Soprano;
using namespace Soprano::Vocabulary;
using namespace Nepomuk;
using namespace Nepomuk::Vocabulary;

namespace {
#ifndef Q_OS_WIN
    void signalHandler( int signal )
    {
        switch( signal ) {
        case SIGHUP:
        case SIGQUIT:
        case SIGINT:
            QCoreApplication::exit( 0 );
        }
    }
#endif

    void installSignalHandler() {
#ifndef Q_OS_WIN
        struct sigaction sa;
        ::memset( &sa, 0, sizeof( sa ) );
        sa.sa_handler = signalHandler;
        sigaction( SIGHUP, &sa, 0 );
        sigaction( SIGINT, &sa, 0 );
        sigaction( SIGQUIT, &sa, 0 );
#endif
    }
}

FakeDataManagementService::FakeDataManagementService(QObject *parent)
    : QObject(parent)
{
    // create our fake storage
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName( "virtuosobackend" );
    Q_ASSERT(backend);
    m_storageDir = new KTempDir();
    m_model = backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, m_storageDir->name()) );
    Q_ASSERT(m_model);

    // create the data management service stack connected to the fake storage
    m_nrlModel = new Soprano::NRLModel(m_model);
    m_dmModel = new Nepomuk::DataManagementModel(m_nrlModel);
    m_dmAdaptor = new Nepomuk::DataManagementAdaptor(m_dmModel);

    // register the adaptor
    QDBusConnection::sessionBus().registerObject(QLatin1String("/datamanagementmodel"), m_dmAdaptor, QDBusConnection::ExportScriptableContents);

    // register under the service name used by the Nepomuk service stub
    QDBusConnection::sessionBus().registerService(QLatin1String("org.kde.nepomuk.services.DataManagement"));

    // register our base model via dbus so the test case can access it
    Soprano::Server::DBusExportModel* dbusModel = new Soprano::Server::DBusExportModel(m_model);
    dbusModel->setParent(this);
    dbusModel->registerModel(QLatin1String("/model"));

    // init the model data
    // TODO: export the slot via DBus so the unit test can call it
    resetModel();
}

FakeDataManagementService::~FakeDataManagementService()
{
    delete m_dmAdaptor;
    delete m_dmModel;
    delete m_nrlModel;
    delete m_model;
    delete m_storageDir;
}

void FakeDataManagementService::resetModel()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph("graph:/onto");
    m_model->addStatement( graph, RDF::type(), NRL::Ontology(), graph );
    // removeResources depends on type inference
    m_model->addStatement( graph, RDF::type(), NRL::Graph(), graph );

    m_model->addStatement( QUrl("prop:/int"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int2"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int2"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int3"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int3"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int_c1"), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( QUrl("prop:/int_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    m_model->addStatement( QUrl("prop:/string"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/string"), RDFS::range(), XMLSchema::string(), graph );

    m_model->addStatement( QUrl("prop:/res"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), RDFS::range(), RDFS::Resource(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    // some ontology things the ResourceMerger depends on
    m_model->addStatement( RDFS::Class(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( RDFS::Class(), RDFS::subClassOf(), RDFS::Resource(), graph );
    m_model->addStatement( NRL::Graph(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NRL::InstanceBase(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NRL::InstanceBase(), RDFS::subClassOf(), NRL::Graph(), graph );

    // rebuild the internals of the data management model
    m_dmModel->updateTypeCachesAndSoOn();
}


int main( int argc, char** argv )
{
    KAboutData aboutData( "fakedms", "fakedms",
                          ki18n("Fake Data Management Service"),
                          "0.1",
                          ki18n("Fake Data Management Service"),
                          KAboutData::License_GPL,
                          ki18n("(c) 2011, Sebastian Trüg"),
                          KLocalizedString(),
                          "http://nepomuk.kde.org" );
    aboutData.setProgramIconName( "nepomuk" );
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer"), "trueg@kde.org");

    KCmdLineOptions options;
    KCmdLineArgs::addCmdLineOptions( options );
    KCmdLineArgs::init( argc, argv, &aboutData );

    KApplication app( false );
    app.disableSessionManagement();
    installSignalHandler();
    QApplication::setQuitOnLastWindowClosed( false );

    FakeDataManagementService fs;
    return app.exec();
}

#include "fakedatamanagementservice.moc"
