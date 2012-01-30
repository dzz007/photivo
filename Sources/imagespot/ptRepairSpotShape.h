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

//==============================================================================

#include <QGraphicsItemGroup>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsDropShadowEffect>

#include "ptRepairSpot.h"

//==============================================================================

class ptRepairSpotShape : public QGraphicsItemGroup {
  friend class ptRepairInteraction;

public:
  explicit ptRepairSpotShape(QGraphicsItem* AParent = NULL);
  ~ptRepairSpotShape();
  void Draw(ptRepairSpot* ASpotData);

private:
  QGraphicsItemGroup*   FSpotGroup;        // container for the spot
  QGraphicsItemGroup*   FRepairerGroup;    // container for repairer and connector line to spot

  QGraphicsEllipseItem* FSpot;
  QGraphicsEllipseItem* FSpotBorder;
  QGraphicsRectItem*    FRadiusHandle;
  QGraphicsRectItem*    FPositionHandle;
  QGraphicsEllipseItem* FRotationHandle;

  QGraphicsDropShadowEffect*  FShadow;   // for the b/w line effect

  QGraphicsEllipseItem* FRepairer;
  QGraphicsLineItem*    FConnector;


};

#endif // PTREPAIRSPOTSHAPE_H
