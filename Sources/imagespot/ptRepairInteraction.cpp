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

#include <cassert>
#include <algorithm>

#include "ptRepairInteraction.h"
#include "ptRepairSpotModel.h"
#include "../ptDefines.h"
#include "../ptConstants.h"

//==============================================================================

ptRepairInteraction::ptRepairInteraction(QGraphicsView *AView,
                                         ptRepairSpotListView *AListView)
: ptImageInteraction(AView),
  FListView(AListView),
  FShadow(new QGraphicsDropShadowEffect),
  FShapes(new QList<TSpotShape*>)
{
  assert(FListView != NULL);
//  FView->scene()->addItem(FSpotShape);

  FShadow->setBlurRadius(0);
  FShadow->setOffset(1);
  FShadow->setColor(QColor(0,0,0));

  connect(FListView, SIGNAL(rowChanged(QModelIndex)), this, SLOT(changeSpot(QModelIndex)));
  changeSpot(FListView->currentIndex());
}

//==============================================================================

ptRepairInteraction::~ptRepairInteraction() {
  std::for_each (FShapes->begin(), FShapes->end(), [this](TSpotShape *hShape) {
    FView->scene()->removeItem(hShape->Group);
    DestroyShape(hShape);
  } );

  DelAndNull(FShapes);
  DelAndNull(FShadow);

  // Warning: Never delete FView, FListView, FSpotData!
}

//==============================================================================

void ptRepairInteraction::stop() {
  emit finished(stSuccess);
}

//==============================================================================

ptRepairInteraction::TSpotShape *ptRepairInteraction::CreateShape() {
  TSpotShape *hShape      = new TSpotShape;
  hShape->Group           = new QGraphicsItemGroup;
  hShape->SpotGroup       = new QGraphicsItemGroup;
  hShape->RepairerGroup   = new QGraphicsItemGroup;
  hShape->Spot            = new QGraphicsEllipseItem;
  hShape->SpotBorder      = new QGraphicsEllipseItem;
  hShape->RadiusHandle    = new QGraphicsRectItem(0.0, 0.0, 15.0, 15.0);
  hShape->RotationHandle  = new QGraphicsEllipseItem(0.0, 0.0, 15.0, 15.0);
  hShape->Repairer        = new QGraphicsEllipseItem;
  hShape->Connector       = new QGraphicsLineItem;

  QPen hSolidPen = QPen(QColor(255,255,255), 1);
  hSolidPen.setCosmetic(true);
  hShape->Group->hide();

  // spot components
  hShape->Spot->setPen(hSolidPen);
  hShape->SpotBorder->setPen(QPen(QColor(255,255,255), Qt::DotLine));
  hShape->RadiusHandle->setPen(hSolidPen);
  hShape->RadiusHandle->setBrush(QBrush(QColor(255,255,255), Qt::SolidPattern));
  hShape->RotationHandle->setPen(hSolidPen);
  hShape->RotationHandle->setBrush(QBrush(QColor(255,255,255), Qt::SolidPattern));

  hShape->Spot->setGraphicsEffect(FShadow);
  hShape->SpotBorder->setGraphicsEffect(FShadow);
  hShape->RadiusHandle->setGraphicsEffect(FShadow);
  hShape->RadiusHandle->setGraphicsEffect(FShadow);

  hShape->SpotGroup->addToGroup(hShape->Spot);
  hShape->SpotGroup->addToGroup(hShape->SpotBorder);
  hShape->SpotGroup->addToGroup(hShape->RadiusHandle);
  hShape->SpotGroup->addToGroup(hShape->RotationHandle);

  // repairer components
  hShape->Repairer->setPen(hSolidPen);
  hShape->Connector->setPen(hSolidPen);

  hShape->Repairer->setGraphicsEffect(FShadow);
  hShape->Connector->setGraphicsEffect(FShadow);

  hShape->RepairerGroup->addToGroup(hShape->Repairer);
  hShape->RepairerGroup->addToGroup(hShape->Connector);

  // add everything to root group
  hShape->Group->addToGroup(hShape->SpotGroup);
  hShape->Group->addToGroup(hShape->RepairerGroup);

  return hShape;
}

//==============================================================================

void ptRepairInteraction::DestroyShape(ptRepairInteraction::TSpotShape* AShape) {
  // Killing the root group is enough because it’s the parent of all other visual components.
  DelAndNull(AShape->Group);
  DelAndNull(AShape);
}

//==============================================================================

void ptRepairInteraction::Draw(ptRepairInteraction::TSpotShape *AShape, ptRepairSpot *ASpotData) {
  // Center point of spot and repairer relative to their groups
  QPointF hSpotCenter(ASpotData->radiusX()/2, ASpotData->radiusY()/2);

  // Set spot’s container’s topleft position so that we can work with (0,0)
  // as the spot’s topleft point.
  AShape->SpotGroup->setPos(ASpotData->pos());

  AShape->Spot->setRect(0, 0, ASpotData->radiusX() * 2, ASpotData->radiusY() * 2);
  AShape->SpotBorder->hide();   // TODO SR: implement inner border display

  // Unrotated handle positions: Radius handle rightcenter, rotation handle bottomcenter
  AShape->RadiusHandle->setPos(
                          AShape->Spot->rect().width() - AShape->RadiusHandle->rect().width()/2,
                          hSpotCenter.y() - AShape->RadiusHandle->rect().height()/2
                        );
  AShape->RotationHandle->setPos(
                          hSpotCenter.x() - AShape->RotationHandle->rect().width()/2,
                          AShape->Spot->rect().height() - AShape->RotationHandle->rect().height()/2
                        );
  AShape->SpotGroup->setRotation(ASpotData->angle());

  if (ASpotData->hasRepairer()) {
    // repairer ellipse
    AShape->RepairerGroup->setPos(ASpotData->repairerPos());
    AShape->Repairer->setRect(0, 0, ASpotData->radiusX() * 2, ASpotData->radiusY() * 2);
    AShape->Repairer->setRotation(ASpotData->angle());

    // connector line between spot and repairer (from/to center)
    // TODO SR: hide undesired sections inside spots
    AShape->Connector->setLine(QLineF(AShape->Spot->mapToItem(AShape->RepairerGroup, hSpotCenter),
                                      hSpotCenter) );
    AShape->RepairerGroup->show();

  } else {
    AShape->RepairerGroup->hide();
  }

  AShape->Group->show();
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
}

//==============================================================================

void ptRepairInteraction::keyAction(QKeyEvent* AEvent) {
}

//==============================================================================

void ptRepairInteraction::changeSpot(const QModelIndex &AIndex) {
  if (AIndex.isValid()) {
    int hIdx = AIndex.row();
    assert(hIdx < FShapes->size());   // catch invalid index rows
    Draw(FShapes->at(hIdx), static_cast<const ptRepairSpotModel*>(AIndex.model())->spot(hIdx));
  }
}

//==============================================================================

