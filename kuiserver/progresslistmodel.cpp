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
#include "progresslistdelegate.h"

#include <QStyleOptionProgressBarV2>

#include <kwidgetjobtracker.h>

ProgressListModel::ProgressListModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

ProgressListModel::~ProgressListModel()
{
    foreach (const JobInfo &it, jobInfoList)
    {
        delete it.progressBar;
    }
}

UIServer::JobView *ProgressListModel::jobView(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return jobInfoList[index.row()].jobView;
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
        case ProgressListDelegate::Capabilities:
            result = jobInfoList[index.row()].capabilities;
            break;
        case ProgressListDelegate::ApplicationName:
            result = jobInfoList[index.row()].applicationName;
            break;
        case ProgressListDelegate::Icon:
            result = jobInfoList[index.row()].icon;
            break;
        case ProgressListDelegate::SizeTotals:
            result = jobInfoList[index.row()].sizeTotals;
            break;
        case ProgressListDelegate::SizeProcessed:
            result = jobInfoList[index.row()].sizeProcessed;
            break;
        case ProgressListDelegate::TimeTotals:
            result = jobInfoList[index.row()].timeTotals;
            break;
        case ProgressListDelegate::TimeElapsed:
            result = jobInfoList[index.row()].timeElapsed;
            break;
        case ProgressListDelegate::Speed:
            result = jobInfoList[index.row()].speed;
            break;
        case ProgressListDelegate::Percent:
            result = jobInfoList[index.row()].percent;
            break;
        case ProgressListDelegate::Message:
            result = jobInfoList[index.row()].message;
            break;
        case ProgressListDelegate::State:
            result = jobInfoList[index.row()].state;
            break;
        default:
            return result;
    }

    return result;
}

Qt::ItemFlags ProgressListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return Qt::ItemIsSelectable
           | Qt::ItemIsEnabled
           | Qt::ItemIsEditable;
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
    foreach (const JobInfo &it, jobInfoList)
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
    return parent.isValid() ? 0 : jobInfoList.count();
}

bool ProgressListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(count);
    Q_UNUSED(parent);

    beginInsertRows(QModelIndex(), row, row);

    JobInfo newJob;

    newJob.jobView = 0;
    newJob.applicationName = QString();
    newJob.icon = QString();
    newJob.sizeTotals = QString();
    newJob.sizeProcessed = QString();
    newJob.timeElapsed = -1;
    newJob.timeTotals = -1;
    newJob.speed = QString();
    newJob.percent = -1;
    newJob.message = QString();
    newJob.progressBar = 0;
    newJob.state = JobInfo::Running;

    jobInfoList.append(newJob);

    endInsertRows();

    return true;
}

bool ProgressListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(count);
    Q_UNUSED(parent);

    if (row >= rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row);

    jobInfoList.removeAt(row);

    endRemoveRows();

    return true;
}

bool ProgressListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    switch (role)
    {
        case ProgressListDelegate::Capabilities:
            jobInfoList[index.row()].capabilities = value.toInt();
            break;
        case ProgressListDelegate::ApplicationName:
            jobInfoList[index.row()].applicationName = value.toString();
            break;
        case ProgressListDelegate::Icon:
            jobInfoList[index.row()].icon = value.toString();
            break;
        case ProgressListDelegate::SizeTotals:
            jobInfoList[index.row()].sizeTotals = value.toString();
            break;
        case ProgressListDelegate::SizeProcessed:
            jobInfoList[index.row()].sizeProcessed = value.toString();
            break;
        case ProgressListDelegate::TimeTotals:
            jobInfoList[index.row()].timeTotals = value.toLongLong();
            break;
        case ProgressListDelegate::TimeElapsed:
            jobInfoList[index.row()].timeElapsed = value.toLongLong();
            break;
        case ProgressListDelegate::Speed:
            jobInfoList[index.row()].speed = value.toString();
            break;
        case ProgressListDelegate::Percent:
            if (!jobInfoList[index.row()].progressBar)
            {
                jobInfoList[index.row()].progressBar = new QStyleOptionProgressBarV2();
                jobInfoList[index.row()].progressBar->maximum = 100;
                jobInfoList[index.row()].progressBar->minimum = 0;
            }
            jobInfoList[index.row()].percent = value.toInt();
            if (jobInfoList[index.row()].progressBar)
            {
                jobInfoList[index.row()].progressBar->progress = jobInfoList[index.row()].percent;
            }
            break;
        case ProgressListDelegate::Message:
            jobInfoList[index.row()].message = value.toString();
            break;
        case ProgressListDelegate::State:
            jobInfoList[index.row()].state = (JobInfo::State) value.toInt();
            break;
        default:
            return false;
    }

    emit dataChanged(index, index);
    emit layoutChanged();

    return true;
}

void ProgressListModel::newJob(const QString &appName, const QString &appIcon, int capabilities, UIServer::JobView *jobView)
{
    int newRow = rowCount();

    insertRow(rowCount());
    setData(newRow, appName, ProgressListDelegate::ApplicationName);
    setData(newRow, appIcon, ProgressListDelegate::Icon);
    setData(newRow, capabilities, ProgressListDelegate::Capabilities);
    jobInfoList[newRow].jobView = jobView;
}


void ProgressListModel::finishJob(UIServer::JobView *jobView)
{
    QModelIndex indexToRemove = indexForJob(jobView);

    if (indexToRemove.isValid())
        removeRow(indexToRemove.row());
}

QPair<QString, QString> ProgressListModel::getDescriptionField(const QModelIndex &index, uint id)
{
    if (!index.isValid() || !jobInfoList[index.row()].descFields.contains(id))
        return QPair<QString, QString>(QString(), QString());

    return jobInfoList[index.row()].descFields[id];
}

bool ProgressListModel::setDescriptionField(const QModelIndex &index, uint id, const QString &name, const QString &value)
{
    if (!index.isValid())
        return false;

    if (jobInfoList[index.row()].descFields.contains(id))
    {
        jobInfoList[index.row()].descFields[id].first = name;
        jobInfoList[index.row()].descFields[id].second = value;
    }
    else
    {
        QPair<QString, QString> descField(name, value);
        jobInfoList[index.row()].descFields.insert(id, descField);
    }

    return true;
}

void ProgressListModel::clearDescriptionField(const QModelIndex &index, uint id)
{
    if (!index.isValid())
        return;

    if (jobInfoList[index.row()].descFields.contains(id))
    {
        jobInfoList[index.row()].descFields.remove(id);
    }
}

JobInfo::State ProgressListModel::state(const QModelIndex &index) const
{
    if (index.isValid()) {
        return ((JobInfo::State) data(index, ProgressListDelegate::State).toInt());
    }

    return JobInfo::InvalidState;
}

QStyleOptionProgressBarV2 *ProgressListModel::progressBar(const QModelIndex &index) const
{
    return jobInfoList[index.row()].progressBar;
}

bool ProgressListModel::setData(int row, const QVariant &value, int role)
{
    return setData(index(row), value, role);
}

#include "progresslistmodel.moc"
