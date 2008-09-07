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

#include <QStyleOptionProgressBarV2>

#include <kwidgetjobtracker.h>

ProgressListModel::ProgressListModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

ProgressListModel::~ProgressListModel()
{
    foreach (const JobInfo &it, jobInfoMap.values())
    {
        delete it.progressBar;
    }
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

    switch (role)
    {
        case Capabilities:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].capabilities;
            break;
        case ApplicationName:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].applicationName;
            break;
        case Icon:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].icon;
            break;
        case SizeTotals:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].sizeTotals;
            break;
        case SizeProcessed:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].sizeProcessed;
            break;
        case TimeTotals:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].timeTotals;
            break;
        case TimeElapsed:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].timeElapsed;
            break;
        case Speed:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].speed;
            break;
        case Percent:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].percent;
            break;
        case Message:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].message;
            break;
        case State:
            result = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].state;
            break;
        case JobViewRole:
            result = QVariant::fromValue<UIServer::JobView*>(jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].jobView);
            break;
        default:
            break;
    }

    return result;
}

Qt::ItemFlags ProgressListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return Qt::ItemIsSelectable
           | Qt::ItemIsEnabled;
}

QModelIndex ProgressListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if (row >= rowCount())
        return QModelIndex();

    return createIndex(row, column, 0);
}

QModelIndex ProgressListModel::indexForJob(UIServer::JobView *jobView) const
{
    int i = 0;
    foreach (const JobInfo &it, jobInfoMap.values())
    {
        if (it.jobView == jobView)
            return createIndex(i, 0, 0);

        i++;
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

    switch (role)
    {
        case Capabilities:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].capabilities = value.toInt();
            break;
        case ApplicationName:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].applicationName = value.toString();
            break;
        case Icon:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].icon = value.toString();
            break;
        case SizeTotals:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].sizeTotals = value.toString();
            break;
        case SizeProcessed:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].sizeProcessed = value.toString();
            break;
        case TimeTotals:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].timeTotals = value.toLongLong();
            break;
        case TimeElapsed:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].timeElapsed = value.toLongLong();
            break;
        case Speed:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].speed = value.toString();
            break;
        case Percent:
            if (!jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar)
            {
                jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar = new QStyleOptionProgressBarV2();
                jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar->maximum = 100;
                jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar->minimum = 0;
            }
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].percent = value.toInt();
            if (jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar)
            {
                jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar->progress = jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].percent;
            }
            break;
        case Message:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].message = value.toString();
            break;
        case State:
            jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].state = (JobInfo::State) value.toInt();
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
    newJob.applicationName.clear();
    newJob.icon.clear();
    newJob.sizeTotals.clear();
    newJob.sizeProcessed.clear();
    newJob.timeElapsed = -1;
    newJob.timeTotals = -1;
    newJob.speed.clear();
    newJob.percent = -1;
    newJob.message.clear();
    newJob.progressBar = 0;
    newJob.state = JobInfo::Running;
    jobInfoMap.insert(newJob.jobView, newJob);

    return newJob.jobView;
}


void ProgressListModel::finishJob(UIServer::JobView *jobView)
{
    beginRemoveRows(QModelIndex(), rowCount() - 1, rowCount() - 1);

    QModelIndex indexToRemove = indexForJob(jobView);

    if (indexToRemove.isValid())
        jobInfoMap.remove(jobView);

    endRemoveRows();
}

QPair<QString, QString> ProgressListModel::getDescriptionField(const QModelIndex &index, uint id)
{
    if (!index.isValid() || !jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields.contains(id))
        return QPair<QString, QString>(QString(), QString());

    return jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields[id];
}

bool ProgressListModel::setDescriptionField(const QModelIndex &index, uint id, const QString &name, const QString &value)
{
    if (!index.isValid())
        return false;

    if (jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields.contains(id))
    {
        jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields[id].first = name;
        jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields[id].second = value;
    }
    else
    {
        QPair<QString, QString> descField(name, value);
        jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields.insert(id, descField);
    }

    return true;
}

void ProgressListModel::clearDescriptionField(const QModelIndex &index, uint id)
{
    if (!index.isValid())
        return;

    if (jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields.contains(id))
    {
        jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].descFields.remove(id);
    }
}

JobInfo::State ProgressListModel::state(const QModelIndex &index) const
{
    if (index.isValid()) {
        return ((JobInfo::State) data(index, State).toInt());
    }

    return JobInfo::InvalidState;
}

QStyleOptionProgressBarV2 *ProgressListModel::progressBar(const QModelIndex &index) const
{
    return jobInfoMap[static_cast<UIServer::JobView*>(index.internalPointer())].progressBar;
}

bool ProgressListModel::setData(int row, const QVariant &value, int role)
{
    return setData(index(row), value, role);
}

#include "progresslistmodel.moc"
