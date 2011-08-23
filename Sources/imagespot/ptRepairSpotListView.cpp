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

#include "ptRepairSpotListView.h"
#include "ptImageSpotList.h"
#include "ptRepairSpotModel.h"
#include "../ptMainWindow.h"
#include "../ptSettings.h"
#include "../ptTheme.h"

extern ptSettings* Settings;
extern ptMainWindow* MainWindow;
extern ptImageSpotList* RepairSpotList;
extern ptTheme* Theme;

ptRepairSpotListView::ptRepairSpotListView(QWidget *parent)
: QListView(parent) {
  setStyle(Theme->ptStyle);
  setStyleSheet(Theme->ptStyleSheet);
}


void ptRepairSpotListView::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  if (current.row() > -1) {
    Settings->SetValue("SpotOpacity", RepairSpotList->at(current.row())->opactiy());
    Settings->SetValue("SpotEdgeSoftness", RepairSpotList->at(current.row())->edgeBlur());
  }

  MainWindow->UpdateSpotRepairUI();
  QListView::currentChanged(current, previous);
}


void ptRepairSpotListView::deleteSpot() {
  static_cast<ptRepairSpotModel*>(model())
      ->removeRows(currentIndex().row(), 1, QModelIndex());
}
