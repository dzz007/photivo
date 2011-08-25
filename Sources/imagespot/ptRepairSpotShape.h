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

#ifndef PTREPAIRSPOTSHAPE_H
#define PTREPAIRSPOTSHAPE_H

#include <QGraphicsItemGroup>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>

#include "ptRepairSpot.h"

class ptRepairSpotShape : public QGraphicsItemGroup {
public:
  explicit ptRepairSpotShape(QGraphicsItem *parent = 0);
  ~ptRepairSpotShape();
  void Draw(ptRepairSpot* SpotData);

private:
  QGraphicsItemGroup*   m_SpotGroup;        // container for the spot
  QGraphicsItemGroup*   m_RepairerGroup;    // container for repairer and connector line to spot
  QGraphicsEllipseItem* m_Spot;
  QGraphicsEllipseItem* m_SpotBorder;
  QGraphicsRectItem*    m_RadiusHandle;
  QGraphicsEllipseItem* m_Repairer;
  QGraphicsLineItem*    m_Connector;


};

#endif // PTREPAIRSPOTSHAPE_H
