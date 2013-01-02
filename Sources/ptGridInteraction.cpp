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

#include "ptGridInteraction.h"
#include "ptDefines.h"
#include <cstdio>

//==============================================================================

ptGridInteraction::ptGridInteraction(QGraphicsView* AView)
: ptAbstractInteraction(AView)
{
}

//==============================================================================

ptGridInteraction::~ptGridInteraction() {
  ClearList();
}

//==============================================================================

void ptGridInteraction::show(const uint ALinesX, const uint ALinesY) {
  ClearList();
  QPen pen(QColor(150,150,150));

  uint width = (uint)FView->scene()->width();
  uint height = (uint)FView->scene()->height();
  uint stepX = (uint)((double)width / (double)(ALinesX + 1));
  uint stepY = (uint)((double)height / (double)(ALinesY + 1));

  // Add vertical lines
  for (uint i = 1; i <= ALinesX; i++) {
    QGraphicsLineItem* temp = FView->scene()->addLine(i*stepX, 0.0, i*stepX, height, pen);
    FGridLines.append(temp);
  }

  // Add horizontal lines
  for (uint i = 1; i <= ALinesY; i++) {
    QGraphicsLineItem* temp = FView->scene()->addLine(0.0, i*stepY, width, i*stepY, pen);
    FGridLines.append(temp);
  }
}

//==============================================================================

// Properly destroy all lines and clear the list
void ptGridInteraction::ClearList() {
  if (FGridLines.count() > 0) {
    for (int i = 0; i < FGridLines.count(); i++) {
      FView->scene()->removeItem(FGridLines[i]);
      DelAndNull(FGridLines[i]);
    }
    FGridLines.clear();
  }
}

//==============================================================================
