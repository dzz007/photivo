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
  m_RadiusHandle(new QGraphicsRectItem(0.0, 0.0, 10.0, 10.0)),
  m_Repairer(new QGraphicsEllipseItem()),
  m_Connector(new QGraphicsLineItem())
{
  QPen solidPen = QPen(QColor(150,150,150));

  m_Spot->setPen(solidPen);
  m_SpotBorder->setPen(QPen(QColor(150,150,150), 0, Qt::DashLine));
  m_RadiusHandle->setPen(solidPen);
  m_Repairer->setPen(solidPen);
  m_Connector->setPen(solidPen);

  m_SpotBorder->setParentItem(m_Spot);
  m_RadiusHandle->setParentItem(m_Spot);

  m_SpotGroup->addToGroup(m_Spot);
  m_RepairerGroup->addToGroup(m_Repairer);
  m_RepairerGroup->addToGroup(m_Connector);
  this->addToGroup(m_SpotGroup);
  this->addToGroup(m_RepairerGroup);

  this->hide();
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


  // Paint current spot
  // spot outer edge
  m_Spot->setRect(SpotData->pos().x() - SpotData->radiusH(),
                  SpotData->pos().y() - SpotData->radiusW(),
                  SpotData->radiusH() * 2,
                  SpotData->radiusW() * 2);
  m_Spot->setRotation(SpotData->angle());

  // spot inner edge
  if (SpotData->edgeRadius() == 0) {
    m_SpotBorder->hide();
  } else {
    m_SpotBorder->setRect(SpotData->edgeRadius(), SpotData->edgeRadius(),
                          SpotData->radiusH() - 2 * SpotData->edgeRadius(),
                          SpotData->radiusW() - 2 * SpotData->edgeRadius() );
    m_SpotBorder->show();
  }

  // repairer and connector line between spot and repairer
  if (SpotData->hasRepairer()) {
    m_Repairer->setRect(SpotData->repairerPos().x() - SpotData->radiusH(),
                        SpotData->repairerPos().y() - SpotData->radiusW(),
                        SpotData->radiusH() * 2,
                        SpotData->radiusW() * 2);
    m_Repairer->setRotation(SpotData->angle());
    m_Connector->setLine(m_Spot->x(), m_Spot->y(), m_Repairer->x(), m_Repairer->y());
    m_RepairerGroup->show();
  } else {
    m_RepairerGroup->hide();
  }

  this->show();
}
