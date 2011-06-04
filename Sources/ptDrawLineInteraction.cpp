/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
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

#include <cassert>
#include <math.h>

#include "ptConstants.h"
#include "ptDrawLineInteraction.h"


///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////

ptDrawLineInteraction::ptDrawLineInteraction(QGraphicsView* View, QGraphicsScene* Scene)
: ptImageInteraction(View, Scene),
  m_NowDragging(0)
{
  m_Line = new QLineF();
  m_LineItem = NULL;
}

ptDrawLineInteraction::~ptDrawLineInteraction() {
  delete m_Line;
}


///////////////////////////////////////////////////////////////////////////
//
// angle()
// Determine rotation angle from the drawn line.
//
///////////////////////////////////////////////////////////////////////////

double ptDrawLineInteraction::angle() {
  if (m_Line->x1() == m_Line->x2()) {
    return 90.0;
  }
  double m = -(double)(m_Line->y1() - m_Line->y2()) / (m_Line->x1() - m_Line->x2());
  return atan(m) * 180.0 / ptPI;
}


///////////////////////////////////////////////////////////////////////////
//
// Mouse actions: left press/release, move
//
///////////////////////////////////////////////////////////////////////////

void ptDrawLineInteraction::mouseAction(QMouseEvent* event) {
  switch (event->type()) {
    // left button press
    case QEvent::MouseButtonPress: {
      if (event->button() == Qt::LeftButton) {
        assert(m_LineItem == NULL);

        // map viewport coords to scene coords
        QPointF pos(m_View->mapToScene(event->pos()));
        m_Line->setPoints(pos, pos);

        QPen pen(QColor(255, 0, 0));
        m_LineItem = m_Scene->addLine(*m_Line, pen);
        m_Scene->update();

        m_NowDragging = 1;
      }
      break;
    }


    // left button release
    case QEvent::MouseButtonRelease: {
      if (event->button() == Qt::LeftButton) {
        m_Scene->removeItem(m_LineItem);
        delete m_LineItem;
        m_NowDragging = 0;
        emit finished();
      }
      break;
    }


    // mouse move
    case QEvent::MouseMove: {
      if (m_NowDragging) {
        m_Line->setP2(m_View->mapToScene(event->pos()));
        m_LineItem->setLine(*m_Line);
        m_Scene->update();
      }
      break;
    }


    default: {
      assert(!"Wrong event type");
      break;
    }
  } //switch
}
