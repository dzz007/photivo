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

#include <cassert>

#include "ptSimpleRectInteraction.h"
#include "ptDefines.h"


///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////

ptSimpleRectInteraction::ptSimpleRectInteraction(QGraphicsView* View)
: ptImageInteraction(View),
  m_NowDragging(0)
{
  m_Rect = new QRectF();
  m_RectItem = NULL;
}

ptSimpleRectInteraction::~ptSimpleRectInteraction() {
  delete m_Rect;
}


///////////////////////////////////////////////////////////////////////////
//
// rect()
//
///////////////////////////////////////////////////////////////////////////

QRectF ptSimpleRectInteraction::rect() {
  return QRectF(m_Rect->normalized());
}


///////////////////////////////////////////////////////////////////////////
//
// Mouse actions: left press/release, move
//
///////////////////////////////////////////////////////////////////////////

void ptSimpleRectInteraction::mouseAction(QMouseEvent* event) {
  switch (event->type()) {
    // left button press
    case QEvent::MouseButtonPress: {
      if (event->button() == Qt::LeftButton) {
        assert(m_RectItem == NULL);

        // map viewport coords to scene coords
        QPointF pos(m_View->mapToScene(event->pos()));
        m_Rect->setTopLeft(pos);
        m_Rect->setBottomRight(pos);

        QPen pen(QColor(150, 150, 150));
        m_RectItem = m_View->scene()->addRect(*m_Rect, pen);
        m_View->scene()->update();

        m_NowDragging = 1;
      }
      break;
    }


    // left button release
    case QEvent::MouseButtonRelease: {
      if (event->button() == Qt::LeftButton) {
        m_View->scene()->removeItem(m_RectItem);
        DelAndNull(m_RectItem);
        m_NowDragging = 0;
        emit finished();
      }
      break;
    }


    // mouse move
    case QEvent::MouseMove: {
      if (m_NowDragging) {
        m_Rect->setBottomRight(m_View->mapToScene(event->pos()));
        m_RectItem->setRect(m_Rect->normalized());
        m_View->scene()->update();
      }
      break;
    }


    default: {
      assert(!"Wrong event type");
      break;
    }
  } //switch
}
