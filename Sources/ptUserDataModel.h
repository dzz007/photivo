/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTUSERDATAMODEL_H
#define PTUSERDATAMODEL_H

//==============================================================================

#include <memory>
using std::unique_ptr;

#include <QDir>
#include <QStandardItemModel>
#include <QStandardItem>

#include "ptLocations.h"

//==============================================================================

class ptUserDataModel: public QStandardItemModel {
Q_OBJECT

public:
  enum Role {
    NodeRole    = Qt::UserRole + 1,
    AccessRole  = Qt::UserRole + 2
  };

  enum NodeType {
    ParentNode  = 0,
    LeafNode    = 1
  };

  enum AccessType {
    ReadOnlyAccess  = 0,
    ReadWriteAccess = 1
  };

//------------------------------------------------------------------------------

  ptUserDataModel(ptLocations::TLocation ALocation, QObject *AParent = nullptr);
  ~ptUserDataModel();
  
//------------------------------------------------------------------------------

private:
  QStandardItem*  CreateItem(const QString &ACaption,
                             NodeType ANodeType,
                             Role ARole,
                             QStandardItem *AParent = nullptr);
  void            Populate();
  QStandardItem*  TraversePath(QDir &ADir, QStandardItem *AParent);

  ptLocations::TLocationType  FCurrentRun;
  ptLocations::TLocation      FLocation;


};
#endif // PTUSERDATAMODEL_H
