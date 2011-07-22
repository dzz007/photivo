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
/**
** Displays a rectangular grid on top of the preview image.
**
** - Does not emit the finished() signal because no lengthy interaction is going
**   on, only simple enabling, show(), and disabling, hide(), of the grid.
** - You do not need to call hide() explicitely before destroying a
**   ptGridInteraction object.
**/

#ifndef PTGRIDINTERACTION_H
#define PTGRIDINTERACTION_H

#include <QList>

#include "ptAbstractInteraction.h"
#include <QGraphicsLineItem>

///////////////////////////////////////////////////////////////////////////
//
// class ptGridInteraction
//
///////////////////////////////////////////////////////////////////////////
class ptGridInteraction : public ptAbstractInteraction {
Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  explicit ptGridInteraction(QGraphicsView* View);
  ~ptGridInteraction();

  void show(const uint linesX, const uint linesY);
  inline void hide() { ClearList(); }

///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////
private:
  QList<QGraphicsLineItem*> m_GridLines;
  void ClearList();


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
///////////////////////////////////////////////////////////////////////////
private slots:
  // abstract fncs from base class not needed here
  void keyAction(QKeyEvent* event) {}
  void mouseAction(QMouseEvent* event) {}

};

#endif // PTGRIDINTERACTION_H
