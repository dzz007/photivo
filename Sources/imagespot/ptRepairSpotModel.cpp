/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

ptRepairSpotModel::ptRepairSpotModel(ptImageSpotList* SpotList, const QSize SizeHint)
: QStandardItemModel(NULL),
  m_SizeHint(SizeHint),
  m_SpotList(SpotList)
{
  // Create model from the actual spot data. Data included:
  // name of current algorithm as the caption; enabled state
  for (int i = 0; i < m_SpotList->count(); i++) {
    ptRepairSpot* spot = static_cast<ptRepairSpot*>(m_SpotList->at(i));
    QStandardItem* SpotItem = new QStandardItem(GuiOptions->SpotRepair[spot->algorithm()].Text);
    SpotItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable |
                       Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    SpotItem->setCheckState(Qt::CheckState(spot->isEnabled()));
    SpotItem->setSizeHint(m_SizeHint);
    appendRow(SpotItem);
  }
}


bool ptRepairSpotModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  return QStandardItemModel::setData(index, value, role);

  // Update actual repair spot data
  ptRepairSpot* spot = static_cast<ptRepairSpot*>(m_SpotList->at(index.row()));
  if (role == Qt::DisplayRole) {    // algorithm
    int i = 0;
    QString AlgoName = value.toString();
    while (GuiOptions->SpotRepair[i].Value.toInt() > -1) {
      if (AlgoName == GuiOptions->SpotRepair[i].Text) {
        spot->setAlgorithm((ptSpotRepairAlgo)i);
        break;
      }
      i++;
    }

  } else if (role == Qt::CheckStateRole) {    // en/disabled switch
    spot->setEnabled(value.toInt());
  }
}


bool ptRepairSpotModel::removeRows(int row, int count, const QModelIndex &parent) {
  beginRemoveRows(parent, row, row+count-1);
  m_SpotList->removeAt(row);
  bool success = QStandardItemModel::removeRows(row, count, parent);
  endRemoveRows();
  return success;
}


///////////////////////////////////////////////////////////////////////////
//
// Drag & Drop
//
///////////////////////////////////////////////////////////////////////////

Qt::ItemFlags ptRepairSpotModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

  if (index.isValid())
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  else
    return Qt::ItemIsDropEnabled | defaultFlags;
}


Qt::DropActions ptRepairSpotModel::supportedDropActions() const {
  return Qt::MoveAction;
}
