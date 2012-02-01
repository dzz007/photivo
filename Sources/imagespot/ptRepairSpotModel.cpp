/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptRepairSpotModel.h"
#include "ptRepairSpot.h"
#include "../ptGuiOptions.h"
#include "../ptConstants.h"

extern ptGuiOptions* GuiOptions;

//==============================================================================

ptRepairSpotModel::ptRepairSpotModel(const QSize ASizeHint)
: QStandardItemModel(NULL),
  CPtsName("RepairSpots"),
  FSizeHint(ASizeHint),
  FSpotList(new QList<ptRepairSpot*>)
{}

//==============================================================================

ptRepairSpotModel::~ptRepairSpotModel() {
  DelAndNull(FSpotList);
}

//==============================================================================

Qt::ItemFlags ptRepairSpotModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

  if (index.isValid())
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  else
    return Qt::ItemIsDropEnabled | defaultFlags;
}

//==============================================================================

void ptRepairSpotModel::LoadFromFile(QSettings *APtsFile) {
  ClearList();

  int hSize = APtsFile->beginReadArray(CPtsName);
  ReportProgress(QObject::tr(QString("Reading %1 repair spots.\n").arg(hSize).toAscii().data()));
  for (int i = 0; i < hSize; i++) {
    APtsFile->setArrayIndex(i);
    RepairSpotList->append(new ptRepairSpot(APtsFile));
  }
  APtsFile->endArray();

  RebuildModel();
}

//==============================================================================

bool ptRepairSpotModel::setData(const QModelIndex &AIndex, const QVariant &AValue, int ARole) {
  bool hResult = QStandardItemModel::setData(AIndex, AValue, ARole);
  if (!hResult) return hResult;

  // Update actual repair spot data
  ptRepairSpot* hspot = static_cast<ptRepairSpot*>(FSpotList->at(AIndex.row()));
  if (ARole == Qt::DisplayRole) {
    // algorithm
    int i = 0;
    QString hAlgoName = AValue.toString();
    while (GuiOptions->SpotRepair[i].Value.toInt() > -1) {
      if (hAlgoName == GuiOptions->SpotRepair[i].Text) {
        hspot->setAlgorithm((ptSpotRepairAlgo)i);
        break;
      }
      i++;
    }

  } else if (ARole == Qt::CheckStateRole) {
    // en/disabled switch
    hspot->setEnabled(AValue.toInt());
  }

  return hResult;
}

//==============================================================================

void ptRepairSpotModel::setSpot(const int AIndex, ptRepairSpot *ASpotData) {
  delete FSpotList[AIndex];
  FSpotList[AIndex] = ASpotData;
}

//==============================================================================

Qt::DropActions ptRepairSpotModel::supportedDropActions() const {
  return Qt::MoveAction;
}

//==============================================================================

bool ptRepairSpotModel::removeRows(int row, int count, const QModelIndex &parent) {
  beginRemoveRows(parent, row, row+count-1);
  FSpotList->removeAt(row);
  bool success = QStandardItemModel::removeRows(row, count, parent);
  endRemoveRows();
  return success;
}

//==============================================================================

void ptRepairSpotModel::WriteToFile(QSettings *APtsFile) {
  // Clear old stored spots, if any
  APtsFile->beginGroup(CPtsName);
  APtsFile->remove("");
  APtsFile->endGroup();

  // Save the new ones
  APtsFile->beginWriteArray(CPtsName);
  for (int i = 0; i < FSpotList->size(); i++) {
    APtsFile->setArrayIndex(i);
    FSpotList->at(i)->WriteToFile(APtsFile);
  }
  APtsFile->endArray();
}

//==============================================================================

void ptRepairSpotModel::ClearList() {
  while (!FSpotList->isEmpty()) {
    delete FSpotList->takeFirst();
  }
}

//==============================================================================

void ptRepairSpotModel::RebuildModel() {
  this->clear();

  for (int i = 0; i < FSpotList->size(); i++) {
    ptRepairSpot* hSpot = FSpotList[i];
    QStandardItem* hSpotItem = new QStandardItem(GuiOptions->SpotRepair[hSpot->algorithm()].Text);
    hSpotItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable |
                        Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    hSpotItem->setCheckState(Qt::CheckState(hSpot->isEnabled()));
    hSpotItem->setSizeHint(FSizeHint);
    appendRow(hSpotItem);
  }
}

//==============================================================================

