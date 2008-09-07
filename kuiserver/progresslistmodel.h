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

#ifndef PROGRESSLISTMODEL_H
#define PROGRESSLISTMODEL_H

#include "uiserver.h"

#include <QAbstractListModel>
#include <QTimer>

#include <kio/global.h>
#include <kio/jobclasses.h>

class QStyleOptionProgressBarV2;
class KWidgetJobTracker;

struct JobInfo
{
    enum State {
        InvalidState = 0,
        Running,
        Suspended,
        Cancelled
    };

    int capabilities;           ///< The capabilities of the job
    UIServer::JobView *jobView; ///< The D-Bus object associated to this job
    QString applicationName;    ///< The application name
    QString icon;               ///< The icon name
    QString sizeTotals;         ///< The total size of the operation
    QString sizeProcessed;      ///< The processed size at the moment
    qlonglong timeElapsed;      ///< The elapsed time
    qlonglong timeTotals;       ///< The total time of the operation
    QString speed;              ///< The current speed of the operation (human readable, example, "3Mb/s")
    int percent;                ///< The current percent of the progress
    QString message;            ///< The information message to be shown
    QHash<uint, QPair<QString, QString> > descFields; ///< Description fields
    QStyleOptionProgressBarV2 *progressBar;           ///< The progress bar to be shown
    State state;                ///< The state of the job
};

class ProgressListModel
    : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ExtraModelRole
    {
        Capabilities = 33,
        ApplicationName,
        Icon,
        SizeTotals,
        SizeProcessed,
        TimeTotals,
        TimeElapsed,
        Speed,
        Percent,
        Message,
        DescFields,
        State,
        JobViewRole
    };

    ProgressListModel(QObject *parent = 0);
    ~ProgressListModel();

    QModelIndex parent(const QModelIndex&) const;

    /**
      * Returns the data on @p index that @p role contains. The result is
      * a QVariant, so you may need to cast it to the type you want
      *
      * @param index    the index in which you are accessing
      * @param role     the role you want to retrieve
      * @return         the data in a QVariant class
      */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    /**
      * Returns what operations the model/delegate support on the given @p index
      *
      * @param index    the index in which you want to know the allowed operations
      * @return         the allowed operations on the model/delegate
      */
    Qt::ItemFlags flags(const QModelIndex &index) const;

    /**
      * Returns the index for the given @p row. Since it is a list, @p column should
      * be 0, but it will be ignored. @p parent will be ignored as well.
      *
      * @param row      the row you want to get the index
      * @param column   will be ignored
      * @param parent   will be ignored
      * @return         the index for the given @p row as a QModelIndex
      */
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    QModelIndex indexForJob(UIServer::JobView *jobView) const;

    /**
      * Returns the number of columns
      *
      * @param parent   will be ignored
      * @return         the number of columns. In this case is always 1
      */
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    /**
      * Returns the number of rows
      *
      * @param parent   will be ignored
      * @return         the number of rows in the model
      */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    /**
      * Sets the data contained on @p value to the given @p index and @p role
      *
      * @param index    the index where the data contained on @p value will be stored
      * @param value    the data that is going to be stored
      * @param role     in what role we want to store the data at the given @p index
      * @return         whether the data was successfully stored or not
      */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    UIServer::JobView* newJob(const QString &appName, const QString &appIcon, int capabilities);

    void finishJob(UIServer::JobView *jobView);

    QPair<QString, QString> getDescriptionField(const QModelIndex &index, uint id);

    bool setDescriptionField(const QModelIndex &index, uint id, const QString &name, const QString &value);

    void clearDescriptionField(const QModelIndex &index, uint id);

    JobInfo::State state(const QModelIndex &index) const;

    /**
      * Returns the progress bar for the given @p index
      *
      * @param index    the index we want to retrieve the progress bar
      * @return         the progress bar for the given @p index. Might return 0 if no progress was set
      */
    QStyleOptionProgressBarV2 *progressBar(const QModelIndex &index) const;

private:
    /**
      * @internal
      */
    bool setData(int row, const QVariant &value, int role = Qt::EditRole);

    QMap<UIServer::JobView*, JobInfo> jobInfoMap; /// @internal
};

Q_DECLARE_METATYPE(UIServer::JobView*);

#endif // PROGRESSLISTMODEL_H
