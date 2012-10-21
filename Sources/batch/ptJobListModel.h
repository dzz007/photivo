/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Sergey Salnikov <salsergey@gmail.com>
**
** This file is part of Photivo.
**
** Photivo is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 3
** as published by the Free Software Foundation.
**
** Photivo is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#ifndef PTJOBLISTMODEL_H
#define PTJOBLISTMODEL_H

//==============================================================================

#include <QAbstractTableModel>
#include "ptJobListItem.h"

//==============================================================================

/*! This class describes the model for the job list in the batch window */
class ptJobListModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit ptJobListModel(QObject *parent = 0);
  ~ptJobListModel();

  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  ptJobListItem* JobItem(int i) const;
  void AddJobToList(const QString &file);
  void RemoveJobFromList(int row);
  
signals:
  
public slots:
  void OnJobItemChanged();

private:
  QList<ptJobListItem*> *m_JobList;
};

//==============================================================================

#endif // PTJOBLISTMODEL_H
