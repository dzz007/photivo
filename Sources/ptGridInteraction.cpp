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

ptGridInteraction::ptGridInteraction(QGraphicsView* View)
: ptImageInteraction(View)
{
}

ptGridInteraction::~ptGridInteraction() {
  ClearList();
}

void ptGridInteraction::show(const uint linesX, const uint linesY) {
  ClearList();
  QPen pen(QColor(150,150,150));

  uint width = (uint)FView->scene()->width();
  uint height = (uint)FView->scene()->height();
  uint stepX = (uint)((double)width / (double)(linesX + 1));
  uint stepY = (uint)((double)height / (double)(linesY + 1));

  // Add vertical lines
  for (uint i = 1; i <= linesX; i++) {
    QGraphicsLineItem* temp = FView->scene()->addLine(i*stepX, 0.0, i*stepX, height, pen);
    m_GridLines.append(temp);
  }

  // Add horizontal lines
  for (uint i = 1; i <= linesY; i++) {
    QGraphicsLineItem* temp = FView->scene()->addLine(0.0, i*stepY, width, i*stepY, pen);
    m_GridLines.append(temp);
  }
}



// Properly destroy all lines and clear the list
void ptGridInteraction::ClearList() {
  if (m_GridLines.count() > 0) {
    for (int i = 0; i < m_GridLines.count(); i++) {
      FView->scene()->removeItem(m_GridLines[i]);
      DelAndNull(m_GridLines[i]);
    }
    m_GridLines.clear();
  }
}
