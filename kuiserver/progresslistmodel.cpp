/**
  * This file is part of the KDE project
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "progresslistmodel.h"

#include <kwidgetjobtracker.h>

ProgressListModel::ProgressListModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

ProgressListModel::~ProgressListModel()
{
}

QModelIndex ProgressListModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

QVariant ProgressListModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    if (!index.isValid())
        return result;

    JobInfo jobInfo = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())];

    switch (role)
    {
        case Capabilities:
            result = jobInfo.capabilities;
            break;
        case ApplicationName:
            result = jobInfo.applicationName;
            break;
        case Icon:
            result = jobInfo.icon;
            break;
        case SizeTotals:
            result = jobInfo.sizeTotals;
            break;
        case SizeProcessed:
            result = jobInfo.sizeProcessed;
            break;
        case TimeTotals:
            result = jobInfo.timeTotals;
            break;
        case TimeElapsed:
            result = jobInfo.timeElapsed;
            break;
        case Speed:
            result = jobInfo.speed;
            break;
        case Percent:
            result = jobInfo.percent;
            break;
        case Message:
            result = jobInfo.message;
            break;
        case State:
            result = jobInfo.state;
            break;
        case JobViewRole:
            result = QVariant::fromValue<UIServer::JobView*>(jobInfo.jobView);
            break;
        default:
            break;
    }

    return result;
}

Qt::ItemFlags ProgressListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QModelIndex ProgressListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if (row >= rowCount())
        return QModelIndex();

    int i = 0;
    UIServer::JobView *jobView = 0;
    foreach (UIServer::JobView *jv, jobInfoMap.keys()) {
        jobView = jv;
        if (i == row)
            break;
        ++i;
    }

    return createIndex(row, column, jobView);
}

QModelIndex ProgressListModel::indexForJob(UIServer::JobView *jobView) const
{
    int i = 0;
    foreach (UIServer::JobView *jv, jobInfoMap.keys()) {
        if (jv == jobView)
            return createIndex(i, 0, jv);
        ++i;
    }

    return QModelIndex();
}

int ProgressListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int ProgressListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : jobInfoMap.count();
}

bool ProgressListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    JobInfo &jobInfo = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())];

    switch (role)
    {
        case SizeTotals:
            jobInfo.sizeTotals = value.toString();
            break;
        case SizeProcessed:
            jobInfo.sizeProcessed = value.toString();
            break;
        case TimeTotals:
            jobInfo.timeTotals = value.toLongLong();
            break;
        case TimeElapsed:
            jobInfo.timeElapsed = value.toLongLong();
            break;
        case Speed:
            jobInfo.speed = value.toString();
            break;
        case Percent:
            jobInfo.percent = value.toInt();
            break;
        case Message:
            jobInfo.message = value.toString();
            break;
        case State:
            jobInfo.state = (JobInfo::State) value.toInt();
            break;
        default:
            return false;
    }

    emit dataChanged(index, index);
    emit layoutChanged();

    return true;
}

UIServer::JobView* ProgressListModel::newJob(const QString &appName, const QString &appIcon, int capabilities)
{
    insertRow(rowCount());

    JobInfo newJob;
    newJob.jobView = new UIServer::JobView();
    newJob.sizeTotals.clear();
    newJob.sizeProcessed.clear();
    newJob.timeElapsed = -1;
    newJob.timeTotals = -1;
    newJob.speed.clear();
    newJob.percent = -1;
    newJob.message.clear();
    newJob.state = JobInfo::Running;
    // given information
    newJob.applicationName = appName;
    newJob.icon = appIcon;
    newJob.capabilities = capabilities;
    jobInfoMap.insert(newJob.jobView, newJob);

    return newJob.jobView;
}


void ProgressListModel::finishJob(UIServer::JobView *jobView)
{
    beginRemoveRows(QModelIndex(), rowCount() - 1, rowCount() - 1);

    QModelIndex indexToRemove = indexForJob(jobView);

    if (indexToRemove.isValid()) {
        jobInfoMap.remove(jobView);
    }

    endRemoveRows();
}

QPair<QString, QString> ProgressListModel::getDescriptionField(const QModelIndex &index, uint id)
{
    if (!index.isValid())
        return QPair<QString, QString>(QString(), QString());

    JobInfo &jobInfo = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())];

    if (!jobInfo.descFields.contains(id))
        return QPair<QString, QString>(QString(), QString());

    return jobInfo.descFields[id];
}

bool ProgressListModel::setDescriptionField(const QModelIndex &index, uint id, const QString &name, const QString &value)
{
    if (!index.isValid())
        return false;

    JobInfo &jobInfo = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())];

    if (jobInfo.descFields.contains(id))
    {
        jobInfo.descFields[id].first = name;
        jobInfo.descFields[id].second = value;
    }
    else
    {
        QPair<QString, QString> descField(name, value);
        jobInfo.descFields.insert(id, descField);
    }

    return true;
}

void ProgressListModel::clearDescriptionField(const QModelIndex &index, uint id)
{
    if (!index.isValid())
        return;

    JobInfo &jobInfo = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())];

    if (jobInfo.descFields.contains(id))
    {
        jobInfo.descFields.remove(id);
    }
}

JobInfo::State ProgressListModel::state(const QModelIndex &index) const
{
    if (index.isValid()) {
        return ((JobInfo::State) data(index, State).toInt());
    }

    return JobInfo::InvalidState;
}

bool ProgressListModel::setData(int row, const QVariant &value, int role)
{
    return setData(index(row), value, role);
}

#include "progresslistmodel.moc"
