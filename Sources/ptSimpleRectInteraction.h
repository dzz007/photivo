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

#ifndef PTSELECTINTERACTION_H
#define PTSELECTINTERACTION_H

#include <QRectF>
#include <QLine>
#include <QGraphicsRectItem>
#include <QMouseEvent>

#include "ptAbstractInteraction.h"

///////////////////////////////////////////////////////////////////////////
//
// class ptSimpleRectInteraction
//
///////////////////////////////////////////////////////////////////////////
class ptSimpleRectInteraction : public ptAbstractInteraction {
Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  explicit ptSimpleRectInteraction(QGraphicsView* View);
  ~ptSimpleRectInteraction();

  inline QRect rect() const { return QRect(m_Rect->normalized().toRect()); }


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////
private:
  int m_CtrlPressed;
  QLine* m_DragDelta;
  QRectF* m_Rect;
  short m_NowDragging;
  QGraphicsRectItem* m_RectItem;

  void Finalize(const ptStatus status);


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
///////////////////////////////////////////////////////////////////////////
private slots:
  void keyAction(QKeyEvent* event);
  void mouseAction(QMouseEvent* event);

};

#endif // PTSELECTINTERACTION_H
