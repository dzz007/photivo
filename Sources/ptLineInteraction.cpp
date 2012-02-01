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
#include "ptLineInteraction.h"
#include "ptDefines.h"


///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////

ptLineInteraction::ptLineInteraction(QGraphicsView* View)
: ptImageInteraction(View),
  m_NowDragging(0)
{
  m_Line = new QLineF();
  m_LineItem = NULL;
}

ptLineInteraction::~ptLineInteraction() {
  delete m_Line;
}


///////////////////////////////////////////////////////////////////////////
//
// angle()
// Determine rotation angle from the drawn line.
//
///////////////////////////////////////////////////////////////////////////

double ptLineInteraction::angle() {
  if (m_Line->x1() == m_Line->x2()) {
    return 90.0;
  }
  double m = -(double)(m_Line->y1() - m_Line->y2()) / (m_Line->x1() - m_Line->x2());
  return atan(m) * 180.0 / ptPI;
}


///////////////////////////////////////////////////////////////////////////
//
// Finalize()
//
///////////////////////////////////////////////////////////////////////////

void ptLineInteraction::Finalize(const ptStatus status) {
  if (m_LineItem != NULL) {
    FView->scene()->removeItem(m_LineItem);
    DelAndNull(m_LineItem);
  }

  m_NowDragging = 0;
  emit finished(status);
}


///////////////////////////////////////////////////////////////////////////
//
// Key actions
//
///////////////////////////////////////////////////////////////////////////

void ptLineInteraction::keyAction(QKeyEvent* event) {
  switch (event->type()) {
    case QEvent::KeyPress:
      if (event->key() == Qt::Key_Escape) {
        event->accept();
        Finalize(stFailure);
      }
      break;

    default:
      // nothing to do
      break;
  }
}


///////////////////////////////////////////////////////////////////////////
//
// Mouse actions: left press/release, move
//
///////////////////////////////////////////////////////////////////////////

void ptLineInteraction::mouseAction(QMouseEvent* event) {
  switch (event->type()) {
    // left button press
    case QEvent::MouseButtonPress: {
      if (event->button() == Qt::LeftButton) {
        event->accept();
        assert(m_LineItem == NULL);

        // map viewport coords to scene coords
        QPointF pos(FView->mapToScene(event->pos()));
        m_Line->setPoints(pos, pos);

        m_LineItem = FView->scene()->addLine(*m_Line, QPen(QColor(255, 0, 0)));
        FView->repaint();

        m_NowDragging = 1;
      }
      break;
    }


    // left button release
    case QEvent::MouseButtonRelease: {
      if (event->button() == Qt::LeftButton) {
        event->accept();
        Finalize(stSuccess);
      }
      break;
    }


    // mouse move
    case QEvent::MouseMove: {
      if (m_NowDragging) {
        event->accept();
        m_Line->setP2(FView->mapToScene(event->pos()));
        m_LineItem->setLine(*m_Line);
        FView->repaint();
      }
      break;
    }


    default: {
      assert(!"Wrong event type");
      break;
    }
  } //switch
}
