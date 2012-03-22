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
#include <algorithm>

#include "ptImageSpotModel.h"
#include "ptLocalSpot.h"
#include "ptRepairSpot.h"

void ReportProgress(const QString Message);

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

void ptImageSpotModel::appendSpot(ptImageSpot *ANewSpot) {
  FSpotList->append(ANewSpot);
  RebuildModel();
}

//==============================================================================

Qt::ItemFlags ptImageSpotModel::flags(const QModelIndex &index) const {
  if (index.isValid()) {
    return Qt::ItemIsEnabled |
           Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable |
           Qt::ItemIsDragEnabled;
  } else {
    return Qt::ItemIsDropEnabled;
  }
}

//==============================================================================

int ptImageSpotModel::MoveRow(const QModelIndex &AFromIdx, const int AOffset) {
  // Sanity and boundary checks
  if (!AFromIdx.isValid() || (AOffset == 0)) return -1;
  int hDestRow = qBound(0, AFromIdx.row()+AOffset, this->rowCount()-1);
  if (AFromIdx.row() == hDestRow) return hDestRow;

  // The actual move: spot data structure first, then the Qt model
  FSpotList->move(AFromIdx.row(), hDestRow);
  this->insertRow(hDestRow, this->takeRow(AFromIdx.row()));

  return hDestRow;
}

//==============================================================================

void ptImageSpotModel::ReadFromFile(QSettings *APtsFile) {
  ClearList();

  int hSize = APtsFile->beginReadArray(FPtsName);
  ReportProgress(
    tr(QString("Reading %1 spots from '%2'.").arg(hSize).arg(FPtsName).toAscii().data())
  );

  for (int i = 0; i < hSize; i++) {
    APtsFile->setArrayIndex(i);

    // TODO: Change into a proper factory some day ...
    if (FPtsName == CLocalAdjustName) {
      FSpotList->append(new ptLocalSpot(APtsFile));
    } else if (FPtsName == CRepairSpotName) {
      FSpotList->append(new ptRepairSpot(APtsFile));
    } else {
      assert(!"FPtsName string not recognised. Invalid type of spot.");
    }
  }
  APtsFile->endArray();

  RebuildModel();
}

//==============================================================================

void ptImageSpotModel::RunFiltering(ptImage *AImage) {
  if (FPtsName == CLocalAdjustName) {
    std::for_each (FSpotList->begin(), FSpotList->end(), [AImage](ptImageSpot *hSpot) {
      ptLocalSpot *hLSpot = static_cast<ptLocalSpot*>(hSpot);
      AImage->MaskedContrast((uint16_t)hLSpot->x(),
                             (uint16_t)hLSpot->y(),
                             hLSpot->threshold(),
                             0,
                             0);
    } );

  } else if (FPtsName == CRepairSpotName) {
    // TODO: BJ repair spot processing

  } else {
    fprintf(stderr, "(%s,%d) WARNING: Unrecognized spot type (%s). No processing done.\n",
            __FILE__, __LINE__, FPtsName.toAscii().data());
  }
}

//==============================================================================

bool ptImageSpotModel::setData(const QModelIndex &index,
                               const QVariant    &value,
                               int               role /*= Qt::EditRole*/)
{
  auto hSpot  = FSpotList->at(index.row());
  auto hValue = QVariant(value);
  // update underlying spot data structure
  if (role == Qt::DisplayRole) {            // spot name
    hSpot->setName(value.toString());
    hValue = AppendCoordsToName(hSpot);

  } else if (role == Qt::CheckStateRole) { // en/disabled switch
    hSpot->setEnabled(value.toBool());
  }

  // update model data
  return QStandardItemModel::setData(index, hValue, role);
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
    tr(QString("Saving %1 spots to '%2'.").arg(FSpotList->size()).arg(FPtsName).toAscii().data())
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
    QStandardItem* hSpotItem = new QStandardItem(AppendCoordsToName(hSpot));
    hSpotItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable |
                        Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    // ListView checkboxes are tristate, so we can’t pass a bool as is.
    hSpotItem->setCheckState(hSpot->isEnabled() ? Qt::Checked : Qt::Unchecked);
    hSpotItem->setSizeHint(FSizeHint);
    hSpotItem->setToolTip(QString(tr("Coordinates in current pipe size: x=%1, y=%2"))
                                    .arg(hSpot->x()).arg(hSpot->y()) );
    appendRow(hSpotItem);
  }
}

//==============================================================================

QString ptImageSpotModel::AppendCoordsToName(const ptImageSpot *ASpot) {
  return QString("%1\t@%2,%3px").arg(ASpot->name()).arg(ASpot->x()).arg(ASpot->y());
}

//==============================================================================
