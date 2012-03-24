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

#include <algorithm>

#include "ptUserDataModel.h"

//==============================================================================

ptUserDataModel::ptUserDataModel(ptLocations::TLocation ALocation, QObject *AParent /*= nullptr*/)
: QStandardItemModel(AParent),
  FCurrentRun(ptLocations::GlobalLocation),
  FLocation(ALocation)
{
  Populate();
}

//==============================================================================

ptUserDataModel::~ptUserDataModel()
{}

//==============================================================================

QStandardItem* ptUserDataModel::CreateItem(const QString &ACaption,
                                           ptUserDataModel::NodeType ANodeType,
                                           ptUserDataModel::Role ARole,
                                           QStandardItem *AParent /*= nullptr*/)
{
  auto hItem = new QStandardItem(ACaption);
  if (AParent) AParent->appendRow(hItem);
  hItem->setData(ANodeType, ARole);
  hItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  if (FCurrentRun == ptLocations::GlobalLocation) {
    hItem->setData(ReadOnlyAccess, AccessRole);

  } else if (FCurrentRun == ptLocations::UserLocation) {
    hItem->setData(ReadWriteAccess, AccessRole);
    hItem->setFlags(hItem->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
  }

  return hItem;
}

//==============================================================================

void ptUserDataModel::Populate() {
  this->clear();

  auto hType = ptLocations::get()->type(FLocation);
  QDir hDir;
  hDir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
  hDir.setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

  // create tree for the read-only items from the Photivo program folder
  if (hType.testFlag(ptLocations::GlobalLocation)) {
    FCurrentRun = ptLocations::GlobalLocation;
    hDir.setNameFilters(QStringList(ptLocations::get()->filter(FLocation)));
    hDir.setPath(ptLocations::get()->path(FLocation, ptLocations::GlobalLocation));
    this->appendRow(TraversePath(hDir, CreateItem(tr("Built in"),
                                                  ptUserDataModel::ParentNode,
                                                  ptUserDataModel::NodeRole)));
  }

  // create tree for the writeable items from the user appdata folder
  if (hType.testFlag(ptLocations::UserLocation)) {
    FCurrentRun = ptLocations::UserLocation;
    hDir.setNameFilters(QStringList(ptLocations::get()->filter(FLocation)));
    hDir.setPath(ptLocations::get()->path(FLocation, ptLocations::UserLocation));
    this->appendRow(TraversePath(hDir, CreateItem(tr("User defined"),
                                                  ptUserDataModel::ParentNode,
                                                  ptUserDataModel::NodeRole)));
  }
}

//==============================================================================

QStandardItem* ptUserDataModel::TraversePath(QDir &ADir, QStandardItem *AParent) {
  auto hEntryList = ADir.entryInfoList();

  std::for_each (hEntryList.begin(), hEntryList.end(), [&](const QFileInfo &hInfo) {
    if (hInfo.isDir()) {
      // file system object is a folder: recurse
      ADir.setPath(hInfo.filePath());
      TraversePath(ADir, CreateItem(hInfo.baseName(),
                                    ptUserDataModel::ParentNode,
                                    ptUserDataModel::NodeRole,
                                    AParent));

    } else {
      // file system object is a file
      CreateItem(hInfo.baseName(),
                 ptUserDataModel::LeafNode,
                 ptUserDataModel::NodeRole,
                 AParent);
    }
  } );

  return AParent;
}

//==============================================================================
