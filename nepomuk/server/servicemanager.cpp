/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include "servicemanager.h"
#include "servicecontroller.h"

#include <QtCore/QHash>
#include <QtDBus/QtDBus>

#include <KService>
#include <KServiceTypeTrader>
#include <KDebug>


//extern QDBUS_EXPORT void qDBusAddSpyHook(void (*)(const QDBusMessage&));

Nepomuk::ServiceManager* Nepomuk::ServiceManager::s_self = 0;

namespace {
    class DependencyTree : public QHash<QString, QStringList>
    {
    public:
        /**
         * Cleanup the tree. Remove all services
         * that have an invalid dependency branch
         */
        void cleanup();

        /**
         * \return true if service depends on dependency
         */
        bool dependsOn( const QString& service, const QString& dependency );

        /**
         * Remove the service and all those depending on it
         * from the services list
         */
        void removeService( const QString& service );

        QStringList servicesDependingOn( const QString& service );
    };

    void DependencyTree::cleanup()
    {
        // cleanup dependency tree
        QHash<QString, QStringList> tmpTree( *this );
        for( QHash<QString, QStringList>::const_iterator it = tmpTree.constBegin();
             it != tmpTree.constEnd(); ++it ) {
            QString service = it.key();
            QStringList dependencies = it.value();

            foreach( const QString &dep, dependencies ) {
                // check if the service depends on a non-existing service
                if( !contains( dep ) ) {
                    kDebug() << "Found invalid dependency:" << service << "depends on non-existing service" << dep;
                    // ignore the service and all those depending on it
                    removeService( service );
                    break;
                }

                // check if the service itself is a dependency of any of its
                // dependencies
                else if( dependsOn( dep, service ) ) {
                    kDebug() << "Found dependency loop:" << service << "depends on" << dep << "and vice versa";
                    // ignore the service and all those depending on it
                    removeService( service );
                    break;
                }
            }
        }
    }

    bool DependencyTree::dependsOn( const QString& service, const QString& dependency )
    {
        foreach( const QString &dep, value( service ) ) {
            if( dep == dependency ||
                dependsOn( dep, dependency ) ) {
                return true;
            }
        }
        return false;
    }


    void DependencyTree::removeService( const QString& service )
    {
        if( contains( service ) ) {
            remove( service );

            // remove any service depending on the removed one
            QHash<QString, QStringList> tmpTree( *this );
            for( QHash<QString, QStringList>::const_iterator it = tmpTree.constBegin();
             it != tmpTree.constEnd(); ++it ) {
                if( it.value().contains( service ) ) {
                    removeService( it.key() );
                }
            }
        }
    }

    QStringList DependencyTree::servicesDependingOn( const QString& service )
    {
        QStringList sl;
        for( QHash<QString, QStringList>::const_iterator it = constBegin();
             it != constEnd(); ++it ) {
            if( it.value().contains( service ) ) {
                sl.append( it.key() );
            }
        }
        return sl;
    }
}


class Nepomuk::ServiceManager::Private
{
public:
    Private( ServiceManager* p )
        : m_initialized(false),
          q(p) {
    }

    // map of all services, started and stopped ones
    QHash<QString, ServiceController*> services;

    // clean dependency tree
    DependencyTree dependencyTree;

    // services that wait for dependencies to initialize
    QSet<ServiceController*> pendingServices;

    // services that were stopped and are waiting for services to stop that depend on them
    QSet<ServiceController*> stoppedServices;

    ServiceController* findService( const QString& name );
    void buildServiceMap();

    /**
     * Try to start a service including all its
     * dependecies. Async.
     */
    void startService( ServiceController* );

    /**
     * Schedule a service and all services depending on it to be stopped.
     */
    void stopService( ServiceController* );

    /**
     * Slot connected to all ServiceController::serviceInitialized
     * signals.
     */
    void _k_serviceInitialized( ServiceController* );

    /**
     * Slot connected to all ServiceController::serviceStopped
     * signals.
     */
    void _k_serviceStopped( ServiceController* );

private:
    bool m_initialized;
    ServiceManager* q;
};


void Nepomuk::ServiceManager::Private::buildServiceMap()
{
    if( !m_initialized ) {
        const KService::List modules = KServiceTypeTrader::self()->query( "NepomukService" );
        for( KService::List::ConstIterator it = modules.constBegin(); it != modules.constEnd(); ++it ) {
            KService::Ptr service = *it;
            QStringList deps = service->property( "X-KDE-Nepomuk-dependencies", QVariant::StringList ).toStringList();
            if ( deps.isEmpty() ) {
                deps.append( "nepomukstorage" );
            }
            deps.removeAll( service->desktopEntryName() );
            dependencyTree.insert( service->desktopEntryName(), deps );
        }

        dependencyTree.cleanup();

        for( KService::List::ConstIterator it = modules.constBegin(); it != modules.constEnd(); ++it ) {
            KService::Ptr service = *it;
            if( dependencyTree.contains( service->desktopEntryName() ) ) {
                ServiceController* sc = new ServiceController( service, q );
                connect( sc, SIGNAL(serviceInitialized(ServiceController*)),
                         q, SLOT(_k_serviceInitialized(ServiceController*)) );
                connect( sc, SIGNAL(serviceStopped(ServiceController*)),
                         q, SLOT(_k_serviceStopped(ServiceController*)) );
                services.insert( sc->name(), sc );
            }
        }

        m_initialized = true;
    }
}


Nepomuk::ServiceController* Nepomuk::ServiceManager::Private::findService( const QString& name )
{
    QHash<QString, ServiceController*>::iterator it = services.find( name );
    if( it != services.end() ) {
        return it.value();
    }
    return 0;
}


void Nepomuk::ServiceManager::Private::startService( ServiceController* sc )
{
    kDebug() << sc->name();

    stoppedServices.remove(sc);

    if( !sc->isRunning() ) {
        // start dependencies if possible
        bool needToQueue = false;
        foreach( const QString &dependency, dependencyTree[sc->name()] ) {
            ServiceController* depSc = findService( dependency );
            if ( !needToQueue && !depSc->isInitialized() ) {
                kDebug() << "Queueing" << sc->name() << "due to dependency" << dependency;
                pendingServices.insert( sc );
                needToQueue = true;
            }

            if ( !depSc->isRunning() ) {
                startService( depSc );
            }
        }

        // start service
        if ( !needToQueue ) {
            sc->start();
        }
    }
}


void Nepomuk::ServiceManager::Private::stopService( ServiceController* service )
{
    pendingServices.remove(service);

    if( service->isRunning() ) {
        // shut down any service depending of this one first
        bool haveRunningRevDeps = false;
        foreach(const QString& dep, dependencyTree.servicesDependingOn( service->name() )) {
            ServiceController* sc = services[dep];
            if( sc->isRunning() ) {
                kDebug() << "Revdep still running:" << sc->name() << "Queuing to be stopped:" << service->name();
                stoppedServices.insert( service );
                haveRunningRevDeps = true;

                stopService( sc );
                pendingServices.insert( sc );
            }
        }

        // if we did not queue it to be stopped we can do it now
        if(!haveRunningRevDeps) {
            stoppedServices.remove(service);
            service->stop();
        }
    }
}


void Nepomuk::ServiceManager::Private::_k_serviceInitialized( ServiceController* sc )
{
    kDebug() << "Service initialized:" << sc->name();

    // check the list of pending services and start as many as possible
    // (we can start services whose dependencies are initialized)
    QList<ServiceController*> sl = pendingServices.toList();
    foreach( ServiceController* service, sl ) {
        if ( service->dependencies().contains( sc->name() ) ) {
            // try to start the service again
            pendingServices.remove( service );
            startService( service );
        }
    }

    emit q->serviceInitialized( sc->name() );
}


void Nepomuk::ServiceManager::Private::_k_serviceStopped( ServiceController* sc )
{
    kDebug() << "Service stopped:" << sc->name();

    // 1. The standard case: we stopped a service and waited for the rev deps to go down
    //
    // try to stop all stopped services that are still waiting for reverse deps to shut down
    QSet<ServiceController*> ss = stoppedServices;
    foreach(ServiceController* sc, ss) {
        stoppedServices.remove(sc);
        stopService(sc);
    }


    // 2. The other case where a service was stopped from the outside and we have to shut down its
    //    reverse deps
    //
    // stop and queue all services depending on the stopped one
    // this will re-trigger this method until all reverse-deps are stopped
    foreach( const QString &dep, dependencyTree.servicesDependingOn( sc->name() ) ) {
        ServiceController* depsc = services[dep];
        if( depsc->isRunning() ) {
            kDebug() << "Stopping and queuing rev-dep" << depsc->name();
            stopService(depsc);
            pendingServices.insert( depsc );
        }
    }


}


Nepomuk::ServiceManager::ServiceManager( QObject* parent )
    : QObject(parent),
      d(new Private(this))
{
    s_self = this;
//    qDBusAddSpyHook(messageFilter);
}


Nepomuk::ServiceManager::~ServiceManager()
{
    stopAllServices();
    delete d;
}


void Nepomuk::ServiceManager::startAllServices()
{
    d->buildServiceMap();

    for( QHash<QString, ServiceController*>::iterator it = d->services.begin();
         it != d->services.end(); ++it ) {
        ServiceController* serviceControl = it.value();

        if( serviceControl->autostart() ) {
            d->startService( serviceControl );
        }
    }
}


void Nepomuk::ServiceManager::stopAllServices()
{
    d->pendingServices.clear();
    for( QHash<QString, ServiceController*>::iterator it = d->services.begin();
         it != d->services.end(); ++it ) {
        ServiceController* serviceControl = it.value();
        d->stopService( serviceControl );
    }
}


bool Nepomuk::ServiceManager::startService( const QString& name )
{
    if( ServiceController* sc = d->findService( name ) ) {
        d->startService( sc );
        return sc->waitForInitialized();
    }
    else {
        // could not find service
        return false;
    }
}


bool Nepomuk::ServiceManager::stopService( const QString& name )
{
    if( ServiceController* sc = d->findService( name ) ) {
        d->stopService( sc );
        sc->waitForStoppedAndTerminate();
        return true;
    }
    return false;
}


QStringList Nepomuk::ServiceManager::runningServices() const
{
    QStringList sl;
    for( QHash<QString, ServiceController*>::iterator it = d->services.begin();
         it != d->services.end(); ++it ) {
        ServiceController* serviceControl = it.value();
        if( serviceControl->isRunning() ) {
            sl.append( serviceControl->name() );
        }
    }
    return sl;
}


QStringList Nepomuk::ServiceManager::availableServices() const
{
    return d->services.keys();
}


bool Nepomuk::ServiceManager::isServiceInitialized( const QString& service ) const
{
    if ( ServiceController* sc = d->findService( service ) ) {
        return sc->isInitialized();
    }
    else {
        return false;
    }
}


bool Nepomuk::ServiceManager::isServiceRunning( const QString& service ) const
{
    if ( ServiceController* sc = d->findService( service ) ) {
        return sc->isRunning();
    }
    else {
        return false;
    }
}


bool Nepomuk::ServiceManager::isServiceAutostarted( const QString& name )
{
    if ( ServiceController* sc = d->findService( name ) ) {
        return sc->autostart();
    }
    else {
        return false;
    }
}


void Nepomuk::ServiceManager::setServiceAutostarted( const QString& name, bool autostart )
{
    if ( ServiceController* sc = d->findService( name ) ) {
        sc->setAutostart( autostart );
    }
}


//#define MODULES_PATH "/modules/"

// FIXME: Use our own system and dbus path
// on-demand module loading
// this function is called by the D-Bus message processing function before
// calls are delivered to objects
// void Nepomuk::ServiceManager::messageFilter( const QDBusMessage& message )
// {
//     if ( message.type() != QDBusMessage::MethodCallMessage )
//         return;

//     QString obj = message.path();
//     if ( !obj.startsWith( MODULES_PATH ) )
//         return;

//     QString name = obj.mid( strlen( MODULES_PATH ) );
//     if ( name == "ksycoca" )
//         return; // Ignore this one.

//     if( ServiceController* sc = self()->d->findService( name ) ) {
//         if( sc->startOnDemand() ) {
//             self()->d->startService( sc );
//             sc->waitForInitialized();
//         }
//     }
// }

#include "servicemanager.moc"
