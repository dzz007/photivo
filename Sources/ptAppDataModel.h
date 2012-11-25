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

#ifndef ptAppDataModel_H
#define ptAppDataModel_H

#include "ptSettings.h"
#include <QDir>
#include <QStandardItemModel>
#include <QStandardItem>

/*!
   \brief The `ptAppDataModel` class provides a Qt item model for access to data files
          in different locations.

   For a given standard Photivo data directory The model provides a double tree structure for
   the files in the global (shared) location and the files in the user’s home location.

   The global tree is read-only because there Photivo’s built-in data files are stored, that
   should not be modified. Also usually the user does not have write access to the file system
   in that location. The user tree on the other hand is editable. The model takes care of the
   necessary file system actions (e.g. renaming files) behind the scenes.
 */
class ptAppDataModel: public QStandardItemModel {
  Q_OBJECT

public:
  /*! The `Role` enum defines the custom types of data the model contains. */
  enum TRole {
    NodeRole    = Qt::UserRole + 1,   //!< type of a node (represents a directory or a file)
    AccessRole  = Qt::UserRole + 2    //!< allowed access modes
  };

  /*! The `NodeType` enum defines the possible types of nodes in the model tree. */
  enum TNodeType {
    ParentNode  = 0,    //!< a tree node that may contain children, a.k.a. directory
    LeafNode    = 1     //!< a tree node at the end of a branch, a.k.a. file
  };

  /*! The `AccessType` enum defines how a node can be accessed. */
  enum TAccessType {
    ReadOnlyAccess  = 0,
    ReadWriteAccess = 1
  };

public:
  ptAppDataModel(TPhotivoDir ADirType, QObject *AParent = nullptr);
  ~ptAppDataModel();

private:
  // Creates a single model item, either standalone or as a child of `AParent`.
  QStandardItem*  createItem(const QString &ACaption,
                             TNodeType ANodeType,
                             TAccessType AAccessType,
                             QStandardItem *AParent = nullptr);

  // Recursively populates `AParent` with a tree of its child items and returns `AParent`.
  QStandardItem*  createTree(QDir &ADir, QStandardItem *AParent);

  // Clears and then populates the complete model
  void            populate();

  TLocationType   FCurrentRun;
  TPhotivoDir     FPhotivoDir;     // associated directory of this model

};
#endif // ptAppDataModel_H
