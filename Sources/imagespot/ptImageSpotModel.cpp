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
#include "../ptImage.h"

void ReportProgress(const QString Message);

//==============================================================================

ptImageSpotModel::ptImageSpotModel(const QSize ASizeHint,
                                   const QString &APtsSectionName,
                                   QObject *AParent /*= nullptr*/)
: QStandardItemModel(AParent),
  FLastChangedRole(Qt::DisplayRole),
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

bool ptImageSpotModel::hasEnabledSpots() const {
  // Find at least one enabled spot in the list
  for (auto hSpot: *FSpotList) {
    if (hSpot->isEnabled())
      return true;
  }
  return false;
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
    for (auto hSpot: *FSpotList) {
      if (hSpot->isEnabled()) {
        ptLocalSpot *hLSpot = static_cast<ptLocalSpot*>(hSpot);
        AImage->MaskedColorAdjust(hLSpot);
      }
    }

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
  // update underlying spot data structure
  auto hSpot = FSpotList->at(index.row());
  if (role == Qt::DisplayRole) {            // spot name
    hSpot->setName(value.toString());

  } else if (role == Qt::CheckStateRole) { // en/disabled switch
    hSpot->setEnabled(value.toBool());
  }

  // update model data
  FLastChangedRole = role;
  return QStandardItemModel::setData(index, value, role);
}

//==============================================================================

void ptImageSpotModel::setSpot(const int AIndex, ptImageSpot *ASpotData) {
  delete FSpotList->at(AIndex);
  FSpotList->replace(AIndex, ASpotData);
}

//==============================================================================

void ptImageSpotModel::setSpotPos(const int AIndex, const int Ax, const int Ay) {
  FSpotList->at(AIndex)->setPos((uint)Ax, (uint)Ay);
  this->setData(this->index(AIndex,0), CreateToolTip(FSpotList->at(AIndex)), Qt::ToolTipRole);
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

  // Create new model entries from spot data structure
  for (auto hSpot: *FSpotList) {
    this->appendRow(CreateSpotItem(hSpot));
  }
}

//==============================================================================

QStandardItem *ptImageSpotModel::CreateSpotItem(const ptImageSpot *ASpot) {
  QStandardItem* hSpotItem = new QStandardItem(ASpot->name());

  hSpotItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable |
                      Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
  // ListView checkboxes are tristate, so we can’t pass a bool as is.
  hSpotItem->setCheckState(ASpot->isEnabled() ? Qt::Checked : Qt::Unchecked);
  hSpotItem->setSizeHint(FSizeHint);
  hSpotItem->setToolTip(CreateToolTip(ASpot));

  return hSpotItem;
}

//==============================================================================

QString ptImageSpotModel::CreateToolTip(const ptImageSpot *ASpot) {
  return
    QString(tr("%1\nCoordinates in pipe size: x=%2, y=%3"))
      .arg(ASpot->name()).arg(ASpot->x()).arg(ASpot->y());
}

//==============================================================================
