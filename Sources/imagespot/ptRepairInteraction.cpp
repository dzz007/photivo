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

#include <cassert>

#include "ptRepairInteraction.h"
#include "ptRepairSpotModel.h"
#include "../ptDefines.h"
#include "../ptConstants.h"

//==============================================================================

ptRepairInteraction::ptRepairInteraction(QGraphicsView* AView,
                                         ptRepairSpotListView* AListView,
                                         ptRepairSpot* ASpotData /*= NULL*/)
: ptImageInteraction(AView),
  FSpotData(ASpotData),
  FListView(AListView),
  FSpotShape(new ptRepairSpotShape)
{
  assert(FListView != NULL);
  FView->scene()->addItem(FSpotShape);

  connect(FListView, SIGNAL(rowChanged(QModelIndex)), this, SLOT(changeSpot(QModelIndex)));
  changeSpot(FListView->currentIndex());
}

//==============================================================================

ptRepairInteraction::~ptRepairInteraction() {
  FView->scene()->removeItem(FSpotShape);
  DelAndNull(FSpotShape);
  // Warning: Never delete FView, FListView, FSpotData!
}

//==============================================================================

void ptRepairInteraction::stop() {
  emit finished(stSuccess);
}

//==============================================================================

void ptRepairInteraction::mouseAction(QMouseEvent* AEvent) {
  switch (AEvent->type()) {
    case QEvent::MouseButtonPress:
      MousePressHandler(AEvent);
      break;
    case QEvent::MouseButtonRelease:
      //MouseReleaseHandler(event);
      break;
    case QEvent::MouseButtonDblClick:
      MouseDblClickHandler(AEvent);
      break;
    case QEvent::MouseMove:
      //MouseMoveHandler(event);
      break;
    default:
      // ignore other mouse events
      break;
  }
}

//==============================================================================

void ptRepairInteraction::MousePressHandler(QMouseEvent* AEvent) {
}

//==============================================================================

void ptRepairInteraction::MouseDblClickHandler(QMouseEvent *AEvent) {
  if (FSpotData == NULL) {
    // create new spot at mouse position
    FListView
  }
}

//==============================================================================

void ptRepairInteraction::keyAction(QKeyEvent* AEvent) {
}

//==============================================================================

void ptRepairInteraction::changeSpot(const QModelIndex& AIndex) {
  // Set new current spot
  if (index.isValid()) {
    FSpotData = static_cast<ptRepairSpot*>(
        static_cast<ptRepairSpotModel*>(FListView->model())->spotList()->at(index.row()) );

  // No spot focused in list -> remove the reference to active spot.
  } else {
    FSpotData = NULL;
  }

  FSpotShape->Draw(FSpotData);
}
