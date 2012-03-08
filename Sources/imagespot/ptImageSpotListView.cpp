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

#include "ptImageSpotListView.h"
#include "ptImageSpotModel.h"
//#include "../ptMainWindow.h"
//#include "../ptSettings.h"
#include "../ptTheme.h"

//extern ptSettings* Settings;
//extern ptMainWindow* MainWindow;
extern ptTheme* Theme;

//==============================================================================

ptImageSpotListView::ptImageSpotListView(QWidget *AParent /*= nullptr*/)
: QListView(AParent)
{
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());
  setSelectionMode(QAbstractItemView::SingleSelection);
  setDragEnabled(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  viewport()->setAcceptDrops(true);
  setDropIndicatorShown(true);
}

//==============================================================================

void ptImageSpotListView::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  QListView::currentChanged(current, previous);
  emit rowChanged(current);

//  if (current.row() > -1) {
//    ptRepairSpot *hSpot = static_cast<ptRepairSpotModel*>(this->model())->spot(current.row());
//    Settings->SetValue("SpotOpacity", hSpot->opactiy());
//    Settings->SetValue("SpotEdgeSoftness", hSpot->edgeBlur());
//  }

//  MainWindow->UpdateSpotRepairUI();
//  QListView::currentChanged(current, previous);
//  emit rowChanged(current);
}

//==============================================================================

void ptImageSpotListView::deleteSpot() {
  static_cast<ptImageSpotModel*>(model())
      ->removeRows(currentIndex().row(), 1, QModelIndex());
  emit rowChanged(currentIndex());
}

//==============================================================================

