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

#include "ptSpotInteraction.h"

//==============================================================================

ptSpotInteraction::ptSpotInteraction(QGraphicsView *AView)
: ptAbstractInteraction(AView),
  FAbortNextMouseAction(false)
{
  FView->setCursor(Qt::CrossCursor);
}

//==============================================================================

ptSpotInteraction::~ptSpotInteraction() {}

//==============================================================================

void ptSpotInteraction::abortMouseAction(const ptMouseAction /*AAction*/) {
  FAbortNextMouseAction = true;
}

//==============================================================================

void ptSpotInteraction::stop() {
  emit finished(stSuccess);
}

//==============================================================================

void ptSpotInteraction::mouseAction(QMouseEvent *AEvent) {
  if (FAbortNextMouseAction) {
    FAbortNextMouseAction = false;
    return;
  }

  if (AEvent->type() == QEvent::MouseButtonRelease &&
      AEvent->button() == Qt::LeftButton &&
      AEvent->modifiers() == Qt::NoModifier)
  {
    QPointF hClickPos = FView->mapToScene(AEvent->pos());
    // ensure click was inside image, then emit signal
    if (FView->scene()->sceneRect().contains(hClickPos)) {
      emit clicked(QPoint(qRound(hClickPos.x()), qRound(hClickPos.y())));
    }
  }
}

//==============================================================================
