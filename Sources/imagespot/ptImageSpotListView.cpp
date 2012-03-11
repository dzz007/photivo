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
#include "ptImageSpot.h"
#include "../ptTheme.h"

extern ptTheme* Theme;

//==============================================================================

ptImageSpotListView::ptImageSpotListView(QWidget *AParent,
                                         ptImageSpot::PCreateSpotFunc ASpotCreator)
: QListView(AParent),
  FSpotCreator(ASpotCreator)
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
}

//==============================================================================

void ptImageSpotListView::deleteSpot() {
  static_cast<ptImageSpotModel*>(model())
      ->removeRows(currentIndex().row(), 1, QModelIndex());
  emit rowChanged(currentIndex());
}

//==============================================================================

void ptImageSpotListView::processCoordinates(const QPoint &APos, const bool AMoveCurrent) {
  if (AMoveCurrent && this->currentIndex().isValid()) {
    // move currently selected spot to new position
    qobject_cast<ptImageSpotModel*>(this->model())
        ->spot(this->currentIndex().row())
            ->setPos(APos.x(), APos.y());

  } else {
    // append new spot to list
    ptImageSpot *hSpot = FSpotCreator();
    hSpot->setPos(APos.x(), APos.y());
    qobject_cast<ptImageSpotModel*>(this->model())
        ->appendSpot(hSpot);
  }
}

//==============================================================================

