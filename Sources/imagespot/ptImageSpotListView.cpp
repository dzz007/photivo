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

#include <memory>

#include "ptImageSpotListView.h"
#include "ptImageSpotModel.h"
#include "ptImageSpot.h"
#include "../ptTheme.h"
#include "../ptViewWindow.h"
#include "../ptImage.h"
#include "../ptSettings.h"

extern ptImage *PreviewImage; // global prev image. Basis for quick interactive preview
extern ptTheme *Theme;
extern ptSettings *Settings;
extern ptViewWindow* ViewWindow;

//==============================================================================

ptImageSpotListView::ptImageSpotListView(QWidget *AParent,
                                         ptImageSpot::PCreateSpotFunc ASpotCreator)
: QListView(AParent),
  FSpotCreator(ASpotCreator)
{
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());

  setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
  setSelectionMode(QAbstractItemView::SingleSelection);

  setDragDropMode(QAbstractItemView::InternalMove);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setAcceptDrops(true);
}

//==============================================================================

ptImageSpotListView::~ptImageSpotListView() {}

//==============================================================================

void ptImageSpotListView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Delete && event->modifiers() == Qt::NoModifier) {
    // DEL key to delete selected spot
    event->accept();
    if (currentIndex().isValid()) {
      this->deleteSpot();
    }

  } else {
    // Base class takes care of standard stuff like cursor keys
    QListView::keyPressEvent(event);
  }
}

//==============================================================================

void ptImageSpotListView::deleteSpot() {
  static_cast<ptImageSpotModel*>(model())
      ->removeRows(currentIndex().row(), 1, QModelIndex());
  emit rowChanged(currentIndex());
}

//==============================================================================

void ptImageSpotListView::processCoordinates(const QPoint &APos, const bool AMoveCurrent) {
  ptImageSpotModel* hModel = qobject_cast<ptImageSpotModel*>(this->model());

  if (AMoveCurrent && this->currentIndex().isValid()) {
    // move currently selected spot to new position
    hModel->spot(this->currentIndex().row())->setPos(APos.x(), APos.y());

  } else {
    // append new spot to list
    ptImageSpot *hSpot = FSpotCreator();
    hSpot->setPos(APos.x(), APos.y());
    hSpot->setName(tr("Spot"));
    hModel->appendSpot(hSpot);
  }

  // Update preview in ViewWindow
  std::unique_ptr<ptImage> hImage(new ptImage);
  hImage->Set(PreviewImage);
  //TODO: BJ PreviewImage is in ptSpace_Profiled -> colour space conversion fails
//  hImage->RGBToLch();
  hModel->RunFiltering(hImage.get());
//  hImage->LchToRGB(Settings->GetInt("WorkColor"));
  ViewWindow->UpdateImage(hImage.get());
}

//==============================================================================

void ptImageSpotListView::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  QListView::currentChanged(current, previous);
  emit rowChanged(current);
}

//==============================================================================

