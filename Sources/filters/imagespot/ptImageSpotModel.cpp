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

#include "ptImageSpotModel.h"
#include "ptImageSpotList.h"
#include "../../ptSettings.h"

//==============================================================================

ptImageSpotModel::ptImageSpotModel(const QSize ASizeHint,
                                   ptImageSpotList *ASpotList,
                                   QObject *AParent /*= nullptr*/)
: QStandardItemModel(AParent),
  FLastChangedRole(Qt::DisplayRole),
  FSizeHint(ASizeHint),
  FSpotList(ASpotList)
{
  this->rebuildModel();
}

//==============================================================================

ptImageSpotModel::~ptImageSpotModel() {
/*
  Resources managed by Qt parent or other objects. Do not delete manually.
    FSpotList
*/
}

//==============================================================================

void ptImageSpotModel::appendSpot(ptImageSpot *ANewSpot) {
  FSpotList->append(ANewSpot);
  rebuildModel();
}

//==============================================================================

void ptImageSpotModel::removeAll() {
  this->clear();
  FSpotList->clear();
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

int ptImageSpotModel::moveRow(const QModelIndex &AFromIdx, const int AOffset) {
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
  FSpotList->at(AIndex)->setPos(QPoint(Ax, Ay));
  this->setData(this->index(AIndex,0), createToolTip(*FSpotList->at(AIndex)), Qt::ToolTipRole);
}

//==============================================================================

Qt::DropActions ptImageSpotModel::supportedDropActions() const {
  return Qt::MoveAction;
}

//==============================================================================

bool ptImageSpotModel::removeRows(int row, int count, const QModelIndex &parent) {
  bool success = QStandardItemModel::removeRows(row, count, parent);
  FSpotList->removeAt(row);
  return success;
}

//==============================================================================

void ptImageSpotModel::rebuildModel() {
  this->clear();  // clears the model, NOT the list of spot data

  for (int i = 0; i < FSpotList->size(); ++i) {
    this->appendRow(createSpotItem(*FSpotList->at(i)));
  }
}

//==============================================================================

QStandardItem *ptImageSpotModel::createSpotItem(const ptImageSpot &ASpot) {
  QStandardItem* hSpotItem = new QStandardItem(ASpot.name());

  hSpotItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable |
                      Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
  // ListView checkboxes are tristate, so we can’t pass a bool as is.
  hSpotItem->setCheckState(ASpot.isEnabled() ? Qt::Checked : Qt::Unchecked);
  hSpotItem->setSizeHint(FSizeHint);
  hSpotItem->setToolTip(createToolTip(ASpot));

  return hSpotItem;
}

//==============================================================================

QString ptImageSpotModel::createToolTip(const ptImageSpot &ASpot) {
  return
    QString(tr("%1\nx=%2, y=%3 (1:1 pipe size)"))
      .arg(ASpot.name())
      .arg(ASpot.x() << Settings->GetInt("Scaled"))
      .arg(ASpot.y() << Settings->GetInt("Scaled"));
}

//==============================================================================
