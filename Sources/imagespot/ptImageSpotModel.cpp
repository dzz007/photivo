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

#include <cassert>

#include "ptImageSpotModel.h"
#include "ptLocalSpot.h"
#include "ptRepairSpot.h"

//==============================================================================

ptImageSpotModel::ptImageSpotModel(const QSize ASizeHint,
                                   const QString &APtsSectionName,
                                   QObject *AParent /*= nullptr*/)
: QStandardItemModel(AParent),
  FPtsName(APtsSectionName),
  FSizeHint(ASizeHint),
  FSpotList(new QList<ptImageSpot*>)
{}

//==============================================================================

ptImageSpotModel::~ptImageSpotModel() {
  ClearList();
  DelAndNull(FSpotList);
}

//==============================================================================

Qt::ItemFlags ptImageSpotModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

  if (index.isValid())
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  else
    return Qt::ItemIsDropEnabled | defaultFlags;
}

//==============================================================================

void ptImageSpotModel::LoadFromFile(QSettings *APtsFile) {
  ClearList();

  int hSize = APtsFile->beginReadArray(FPtsName);
  ReportProgress(
    tr(QString("Reading %1 spots from '%2'.\n").arg(hSize).arg(FPtsName).toAscii().data())
  );

  for (int i = 0; i < hSize; i++) {
    APtsFile->setArrayIndex(i);

    // TODO: Change into a proper factory some day ...
    if (FPtsName == "LocalSpots") {
      FSpotList->append(new ptLocalSpot(APtsFile));
    } else if (FPtsName == "RepairSpots") {
      FSpotList->append(new ptRepairSpot(APtsFile));
    } else {
      assert(!"FPtsName string not recognised. Invalid type of spot.");
    }
  }
  APtsFile->endArray();

  RebuildModel();
}

//==============================================================================

bool ptImageSpotModel::setData(const QModelIndex &index,
                               const QVariant    &value,
                               int               role = Qt::EditRole)
{
  bool hResult = QStandardItemModel::setData(index, value, role);
  if (!hResult) return hResult;

  // Update actual repair spot data
  if (role == Qt::DisplayRole) {    // spot name
    FSpotList->at(index.row())->setName(value.toString());
  } else if (role == Qt::CheckStateRole) {    // en/disabled switch
    FSpotList->at(index.row())->setEnabled(value.toInt());
  }

  return hResult;
}

//==============================================================================

void ptImageSpotModel::setSpot(const int AIndex, ptImageSpot *ASpotData) {
  delete FSpotList->at(AIndex);
  FSpotList->replace(AIndex, ASpotData);
}

//==============================================================================

Qt::DropActions ptImageSpotModel::supportedDropActions() const {
  return Qt::MoveAction;
}

//==============================================================================

bool ptImageSpotModel::removeRows(int row, int count, const QModelIndex &parent) {
  beginRemoveRows(parent, row, row+count-1);
  FSpotList->removeAt(row);
  bool success = QStandardItemModel::removeRows(row, count, parent);
  endRemoveRows();
  return success;
}

//==============================================================================

void ptImageSpotModel::WriteToFile(QSettings *APtsFile) {
  // Clear old stored spots, if any
  APtsFile->beginGroup(FPtsName);
  APtsFile->remove("");
  APtsFile->endGroup();

  // Save the new ones
  ReportProgress(
    tr(QString("Saving %1 spots to '%2'.\n").arg(FSpotList->size()).arg(FPtsName).toAscii().data())
  );

  APtsFile->beginWriteArray(FPtsName);
  for (int i = 0; i < FSpotList->size(); i++) {
    APtsFile->setArrayIndex(i);
    FSpotList->at(i)->WriteToFile(APtsFile);
  }
  APtsFile->endArray();
}

//==============================================================================

void ptImageSpotModel::ClearList() {
  while (!FSpotList->isEmpty()) {
    delete FSpotList->takeFirst();
  }
}

//==============================================================================

void ptImageSpotModel::RebuildModel() {
  this->clear();  // clears the model, NOT the list of spot data

  for (int i = 0; i < FSpotList->size(); i++) {
    ptImageSpot* hSpot = FSpotList->at(i);
    QStandardItem* hSpotItem = new QStandardItem(hSpot->name());
    hSpotItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable |
                        Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    hSpotItem->setCheckState(Qt::CheckState(hSpot->isEnabled()));
    hSpotItem->setSizeHint(FSizeHint);
    appendRow(hSpotItem);
  }
}

//==============================================================================
