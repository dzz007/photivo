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

#include <QPen>

#include "../ptDefines.h"
#include "ptRepairSpotShape.h"

//==============================================================================

ptRepairSpotShape::ptRepairSpotShape(QGraphicsItem *AParent)
: QGraphicsItemGroup(AParent),
  FSpotGroup(new QGraphicsItemGroup()),
  FRepairerGroup(new QGraphicsItemGroup()),
  FSpot(new QGraphicsEllipseItem()),
  FSpotBorder(new QGraphicsEllipseItem()),
  FRadiusHandle(new QGraphicsRectItem(0.0, 0.0, 15.0, 15.0)),
  FPositionHandle(new QGraphicsRectItem(0.0, 0.0, 15.0, 15.0)),
  FRotationHandle(new QGraphicsEllipseItem(0.0, 0.0, 15.0, 15.0)),
  FShadow(new QGraphicsDropShadowEffect),
  FRepairer(new QGraphicsEllipseItem()),
  FConnector(new QGraphicsLineItem())
{
  this->hide();
  QPen hSolidPen = QPen(QColor(255,255,255), 1);
  hSolidPen.setCosmetic(true);

  FShadow->setBlurRadius(0);
  FShadow->setOffset(1);
  FShadow->setColor(QColor(0,0,0));

  FSpot->setPen(hSolidPen);
  FSpotBorder->setPen(QPen(QColor(255,255,255), 0, Qt::DotLine));
  FRadiusHandle->setPen(hSolidPen);
  FRadiusHandle->setBrush(QBrush(QColor(255,255,255), Qt::SolidPattern));
  FPositionHandle->setPen(hSolidPen);
  FPositionHandle->setBrush(QBrush(QColor(255,255,255), Qt::SolidPattern));
  FRotationHandle->setPen(hSolidPen);
  FRotationHandle->setBrush(QBrush(QColor(255,255,255), Qt::SolidPattern));

  FSpot->setGraphicsEffect(FShadow);
  FSpotBorder->setGraphicsEffect(FShadow);
  FRadiusHandle->setGraphicsEffect(FShadow);
  FPositionHandle->setGraphicsEffect(FShadow);


  FSpotGroup->addToGroup(FSpot);
  FSpotGroup->addToGroup(FSpotBorder);
  FSpotGroup->addToGroup(FRadiusHandle);
  FSpotGroup->addToGroup(FPositionHandle);
  FSpotGroup->addToGroup(FRotationHandle);

  FRepairer->setPen(hSolidPen);
  FConnector->setPen(hSolidPen);

  FRepairerGroup->addToGroup(FRepairer);
  FRepairerGroup->addToGroup(FConnector);

  this->addToGroup(FSpotGroup);
  this->addToGroup(FRepairerGroup);
}

//==============================================================================

ptRepairSpotShape::~ptRepairSpotShape() {
  DelAndNull(FSpot);
  DelAndNull(FSpotBorder);
  DelAndNull(FRadiusHandle);
  DelAndNull(FRepairer);
  DelAndNull(FConnector);
  DelAndNull(FSpotGroup);
  DelAndNull(FRepairerGroup);
  DelAndNull(FShadow);
}

//==============================================================================

void ptRepairSpotShape::Draw(ptRepairSpot* ASpotData) {
  // no spot focused in list
  if (ASpotData == NULL) {
    this->hide();
    return;
  }

  // Center point of spot and repairer relative to their groups
  QPointF hSpotCenter(ASpotData->radiusX()/2, ASpotData->radiusY()/2);

  // Set spot’s container’s topleft position so that we can work with (0,0)
  // as the spot’s topleft point.
  FSpotGroup->setPos(ASpotData->pos());

  FSpot->setRect(0, 0,
                 ASpotData->radiusX() * 2,
                 ASpotData->radiusY() * 2);

  // spot inner edge
  // TODO SR: implement inner border display
  FSpotBorder->hide();

  // Unrotated handle positions: Radius handle rightcenter, rotation handle bottomcenter
  FRadiusHandle->setPos(FSpot->rect().width() - FRadiusHandle->rect().width()/2,
                        hSpotCenter.y() - FRadiusHandle->rect().height()/2);
  FRotationHandle->setPos(hSpotCenter.x() - FRotationHandle->rect().width()/2,
                          FSpot->rect().height() - FRotationHandle->rect().height()/2);
  FSpotGroup->setRotation(ASpotData->angle());


  if (ASpotData->hasRepairer()) {
    // repairer ellipse
    FRepairerGroup->setPos(ASpotData->repairerPos());
    FRepairer->setRect(0, 0,
                       ASpotData->radiusX() * 2, ASpotData->radiusY() * 2);
    FRepairer->setRotation(ASpotData->angle());

    // connector line between spot and repairer (from/to center)
    // TODO SR: hide undesired sections inside spots
    FConnector->setLine(QLineF(FSpot->mapToItem(FRepairerGroup, hSpotCenter), hSpotCenter));

    FRepairerGroup->show();
  } else {
    FRepairerGroup->hide();
  }

  this->show();
}
