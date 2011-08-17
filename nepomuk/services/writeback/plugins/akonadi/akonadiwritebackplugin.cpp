/*
    Copyright (C) 2011  Smit Shah <Who828@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <QFile>
#include <QUrl>

#include <Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NCO>
#include <Nepomuk/Variant>

#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemFetchScope>

#include <KABC/Addressee>

#include "akonadiwritebackplugin.h"

using namespace Nepomuk::Vocabulary;

Nepomuk::AkonadiWritebackPlugin::AkonadiWritebackPlugin(QObject* parent,const QList<QVariant>&): WritebackPlugin(parent)
{

}

Nepomuk::AkonadiWritebackPlugin::~AkonadiWritebackPlugin()
{

}

void Nepomuk::AkonadiWritebackPlugin::doWriteback(const QUrl& url)
{
    Nepomuk::Resource resource(url);
    if(resource.isValid()) {
        Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(  Akonadi::Item::fromUrl(url)  );
        fetchJob->fetchScope().fetchFullPayload();

        connect( fetchJob, SIGNAL( result(KJob*) ), SLOT( fetchFinished(KJob*) ) );
    }
}

void Nepomuk::AkonadiWritebackPlugin::fetchFinished(KJob* job)
{
    if ( job->error() ) {
        qDebug() << "Error encountered" ;
        emitFinished(false);
    }
    qDebug() << "in fetchFinished";

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );

    Akonadi::Item item = fetchJob->items().first();

    KABC::Addressee addressee = item.payload<KABC::Addressee>();

    Nepomuk::Resource resource(item.url());

    if((resource.property(NCO::nickname())).isValid()) {
        QString nick= (resource.property(NCO::nickname())).toString();

        if(nick != addressee.nickName()) {
            addressee.setNickName(nick);
        }
    }

    if((resource.property(NCO::nameFamily())).isValid()) {
        QString fname= (resource.property(NCO::nameFamily())).toString();

        if(fname != addressee.familyName()) {
            addressee.setFamilyName(fname);
        }
    }

    if((resource.property(NCO::nameGiven())).isValid()) {
        QString gname= (resource.property(NCO::nameGiven())).toString();

        if(gname != addressee.givenName()) {
            addressee.setGivenName(gname);
        }
    }

    if((resource.property(NCO::EmailAddress())).isValid()) {
        QString email= (resource.property(NCO::EmailAddress())).toString();

        if(email != addressee.preferredEmail()) {
            addressee.insertEmail(email,true);
        }
    }

    if((resource.property(NCO::birthDate())).isValid()) {
        QDateTime birthDate= (resource.property(NCO::birthDate())).toDateTime();

        if(birthDate != addressee.birthday()) {
            addressee.setBirthday(birthDate);
        }
    }
    item.setPayload<KABC::Addressee>( addressee );
    Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item );
    connect( modifyJob, SIGNAL( result( KJob* ) ), SLOT( modifyFinished( KJob* ) ) );
}

void Nepomuk::AkonadiWritebackPlugin::modifyFinished( KJob *job )
{
    if ( job->error() ) {
        qDebug() << "Error occurred";
        emitFinished(false);
    }
    else {
        qDebug() << "Item modified successfully";
        emitFinished(true);
    }
}

NEPOMUK_EXPORT_WRITEBACK_PLUGIN(Nepomuk::AkonadiWritebackPlugin,"nepomuk_writeback_akonadi")

#include "akonadiwritebackplugin.moc"
