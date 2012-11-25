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

#include "ptAppDataModel.h"
#include <algorithm>

ptAppDataModel::ptAppDataModel(TPhotivoDir ADirType, QObject *AParent /*= nullptr*/)
: QStandardItemModel(AParent),
  FCurrentRun(GlobalLocation),
  FPhotivoDir(ADirType)
{
  populate();
}

//------------------------------------------------------------------------------
ptAppDataModel::~ptAppDataModel()
{}

//------------------------------------------------------------------------------
QStandardItem* ptAppDataModel::createItem(const QString &ACaption,
                                           TNodeType ANodeType,
                                           TAccessType AAccessType,
                                           QStandardItem *AParent /*= nullptr*/)
{
  auto hItem = new QStandardItem(ACaption);
  if (AParent)
    AParent->appendRow(hItem);
  hItem->setData(ANodeType, NodeRole);
  hItem->setData(AAccessType, AccessRole);
  hItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  if (AAccessType == ReadWriteAccess)
    hItem->setFlags(hItem->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

  return hItem;
}

//------------------------------------------------------------------------------
QStandardItem* ptAppDataModel::createTree(QDir &ADir, QStandardItem *AParent) {
  auto hEntryList = ADir.entryInfoList();

  std::for_each(hEntryList.begin(), hEntryList.end(), [&](const QFileInfo &hFileInfo) {
    if (hFileInfo.isDir()) {
      // file system object is a folder: recurse
      ADir.setPath(hFileInfo.filePath());
      this->createTree(ADir, this->createItem(hFileInfo.baseName(),
                                              ParentNode,
                                              (TAccessType)AParent->data(AccessRole).toInt(),
                                              AParent));
    } else {
      // file system object is a file
      this->createItem(hFileInfo.baseName(),
                       LeafNode,
                       (TAccessType)AParent->data(AccessRole).toInt(),
                       AParent);
    }
  } );

  return AParent;
}

//------------------------------------------------------------------------------
void ptAppDataModel::populate() {
  this->clear();

  TLocationTypes hType = Settings->PathTypes(FPhotivoDir);
  QDir hDir;
  hDir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
  hDir.setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

  // create tree for the read-only items from the Photivo program folder
  if (hType & GlobalLocation) {
    hDir.setNameFilters(Settings->PathFilters(FPhotivoDir));
    hDir.setPath(Settings->GlobalPath(FPhotivoDir));
    this->appendRow(
          this->createTree(hDir,
                           this->createItem(tr("Built in"), ParentNode, ReadOnlyAccess)) );
  }

  // create tree for the writeable items from the user appdata folder
  if (hType & UserLocation) {
    hDir.setNameFilters(Settings->PathFilters(FPhotivoDir));
    hDir.setPath(Settings->GlobalPath(FPhotivoDir));
    this->appendRow(
          this->createTree(hDir,
                           this->createItem(tr("Custom"), ParentNode, ReadWriteAccess)) );
  }
}
