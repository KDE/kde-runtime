/*
This file is part of KDE.

Copyright (c) 2009 Frederik Gladhorn <gladhorn@kde.org>

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

#include "downloaddescription.h"

#include <QtCore/QSharedData>

namespace Attica {
class DownloadDescription::Private :public QSharedData
{
public:
    bool isDownloadtypLink;
    bool hasPrice;
    QString category;
    QString name;
    QString link;
    QString distributionType;
    QString priceReason;
    QString priceAmount;
    Private() {}
};
}

using namespace Attica;

DownloadDescription::DownloadDescription()
    :d(new Private)
{
}

DownloadDescription::DownloadDescription(const Attica::DownloadDescription& other)
    :d(other.d)
{
}

DownloadDescription& DownloadDescription::operator=(const Attica::DownloadDescription& other)
{
    d = other.d;
    return *this;
}

DownloadDescription::~DownloadDescription()
{
}

QString Attica::DownloadDescription::category()
{
    return d->category;
}

void DownloadDescription::setCategory(const QString& category)
{
    d->category = category;
}

QString Attica::DownloadDescription::distributionType()
{
    return d->distributionType;
}

void DownloadDescription::setDistributionType(const QString& distributionType)
{
    d->distributionType = distributionType;
}

bool Attica::DownloadDescription::hasPrice()
{
    return d->hasPrice;
}

void DownloadDescription::setHasPrice(bool hasPrice)
{
    d->hasPrice = hasPrice;
}

bool Attica::DownloadDescription::isDownloadtypLink()
{
    return d->isDownloadtypLink;
}

void DownloadDescription::setDownloadtypLink(bool isLink)
{
    d->isDownloadtypLink = isLink;
}

QString Attica::DownloadDescription::link()
{
    return d->link;
}

void DownloadDescription::setLink(const QString& link)
{
    d->link = link;
}

QString Attica::DownloadDescription::name()
{
    return d->name;
}

void DownloadDescription::setName(const QString& name)
{
    d->name = name;
}

QString Attica::DownloadDescription::priceAmount()
{
    return d->priceAmount;
}

void DownloadDescription::setPriceAmount(const QString& priceAmount)
{
    d->priceAmount = priceAmount;
}

QString Attica::DownloadDescription::priceReason()
{
    return d->priceReason;
}

void Attica::DownloadDescription::setPriceReason(const QString& priceReason)
{
    d->priceReason = priceReason;
}

