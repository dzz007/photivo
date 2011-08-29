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

ptRepairSpotShape::ptRepairSpotShape(QGraphicsItem *parent)
: QGraphicsItemGroup(parent),
  m_SpotGroup(new QGraphicsItemGroup()),
  m_RepairerGroup(new QGraphicsItemGroup()),
  m_Spot(new QGraphicsEllipseItem()),
  m_SpotBorder(new QGraphicsEllipseItem()),
  m_RadiusHandle(new QGraphicsRectItem(0.0, 0.0, 15.0, 15.0)),
  m_PositionHandle(new QGraphicsRectItem(0.0, 0.0, 15.0, 15.0)),
  m_RotationHandle(new QGraphicsEllipseItem(0.0, 0.0, 15.0, 15.0)),
  m_Repairer(new QGraphicsEllipseItem()),
  m_Connector(new QGraphicsLineItem())
{
  this->hide();
  QPen solidPen = QPen(QColor(150,150,150), 2);
  solidPen.setCosmetic(true);

  m_Spot->setPen(solidPen);
  m_SpotBorder->setPen(QPen(QColor(150,150,150), 0, Qt::DotLine));
  m_RadiusHandle->setPen(solidPen);
  m_RadiusHandle->setBrush(QBrush(QColor(150,150,150), Qt::SolidPattern));
  m_PositionHandle->setPen(solidPen);
  m_PositionHandle->setBrush(QBrush(QColor(150,150,150), Qt::SolidPattern));
  m_RotationHandle->setPen(solidPen);
  m_RotationHandle->setBrush(QBrush(QColor(150,150,150), Qt::SolidPattern));

  m_SpotGroup->addToGroup(m_Spot);
  m_SpotGroup->addToGroup(m_SpotBorder);
  m_SpotGroup->addToGroup(m_RadiusHandle);
  m_SpotGroup->addToGroup(m_PositionHandle);
  m_SpotGroup->addToGroup(m_RotationHandle);

  m_Repairer->setPen(solidPen);
  m_Connector->setPen(solidPen);

  m_RepairerGroup->addToGroup(m_Repairer);
  m_RepairerGroup->addToGroup(m_Connector);

  this->addToGroup(m_SpotGroup);
  this->addToGroup(m_RepairerGroup);
}


ptRepairSpotShape::~ptRepairSpotShape() {
  DelAndNull(m_Spot);
  DelAndNull(m_SpotBorder);
  DelAndNull(m_RadiusHandle);
  DelAndNull(m_Repairer);
  DelAndNull(m_Connector);
  DelAndNull(m_SpotGroup);
  DelAndNull(m_RepairerGroup);
}


///////////////////////////////////////////////////////////////////////////
//
// UpdateSpotShape()
//
///////////////////////////////////////////////////////////////////////////

void ptRepairSpotShape::Draw(ptRepairSpot* SpotData) {
  // no spot focused in list
  if (SpotData == NULL) {
    this->hide();
    return;
  }

//  // spot outer edge
//  printf("=========spotdata at shape===========\n"
//         "x %d  y %d\n"
//         "w %d  h %d\n"
//         "angle %f\n==========================\n",
//         SpotData->pos().x(), SpotData->pos().y(),
//         SpotData->radiusW(), SpotData->radiusH(), SpotData->angle());

  // Set spot’s container’s topleft position so that we can work with (0,0)
  // as the spot’s center point.
  m_SpotGroup->setPos(SpotData->pos().x() + SpotData->radiusX(),
                      SpotData->pos().y() + SpotData->radiusY() );

  m_Spot->setRect(0,0,//-SpotData->radiusX(),
                  //-SpotData->radiusY(),
                  SpotData->radiusX() * 2,
                  SpotData->radiusY() * 2);

  // spot inner edge
  // TODO: move inner egde to tool pane slider
  m_SpotBorder->hide();
//  if (SpotData->edgeRadius() == 0) {
//    m_SpotBorder->hide();
//  } else {
//    m_SpotBorder->setRect(SpotData->edgeRadius(),
//                          SpotData->edgeRadius(),
//                          2 * SpotData->radiusX() - 2 * SpotData->edgeRadius(),
//                          2 * SpotData->radiusY() - 2 * SpotData->edgeRadius() );
//    m_SpotBorder->show();
//  }

  // TODO SR: real radius/rotate handle position
  m_RadiusHandle->setPos(SpotData->radiusX(), SpotData->radiusY());
  m_RotationHandle->setPos(SpotData->radiusX() + 20, SpotData->radiusY() + 20);
  m_SpotGroup->setRotation(SpotData->angle());

  // repairer and connector line between spot and repairer
  m_RepairerGroup->hide();
//  if (SpotData->hasRepairer()) {
//    m_RepairerGroup->setPos(SpotData->repairerPos());
//    m_Repairer->setRect(0, 0,
//                        SpotData->radiusX() * 2,
//                        SpotData->radiusY() * 2);
//    m_Repairer->setRotation(SpotData->angle());
//    m_Connector->setLine(m_Spot->x(), m_Spot->y(), m_Repairer->x(), m_Repairer->y());
//    m_RepairerGroup->show();
//  } else {
//    m_RepairerGroup->hide();
//  }

  this->show();
}
