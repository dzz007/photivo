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
#include <cmath>

#include "ptRichRectInteraction.h"
#include "ptDefines.h"
#include "ptSettings.h"
#include "ptTheme.h"

extern ptSettings* Settings;
extern ptTheme* Theme;

///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////

ptRichRectInteraction::ptRichRectInteraction(QGraphicsView* View,
      const int x, const int y, const int width, const int height,
      const short FixedAspectRatio, const uint AspectRatioW,
      const uint AspectRatioH, const short Guidelines)
: ptImageInteraction(View),
  //constants
  EdgeThickness(20),
  TinyRectThreshold(40),
  //variables
  m_CtrlIsPressed(0),
  m_Guides(Guidelines),
  m_MovingEdge(meNone),
  m_NowDragging(0),
  m_ExitStatus(stUndetermined)
{
  assert(Settings != NULL);
  assert(Theme != NULL);

  // Brushes for the LightsOut modes
  m_LightsOutBrushes[0] = new QBrush();
  m_LightsOutBrushes[1] = new QBrush(QColor(20, 20, 20, 200));
  if (Settings->GetInt("BackgroundColor")) {
    m_LightsOutBrushes[2] = new QBrush(QColor(Settings->GetInt("BackgroundRed"),
                                              Settings->GetInt("BackgroundGreen"),
                                              Settings->GetInt("BackgroundBlue")));
  } else {
    m_LightsOutBrushes[2] = new QBrush(Theme->ptBackground);
  }

  // initial rectangles for LightsOut
  for (int i = 0; i <= 3; i++) {
    m_LightsOutRects[i] = m_View->scene()->addRect(
                              0,0,0,0,
                              QPen(),
                              *m_LightsOutBrushes[Settings->GetInt("LightOut")]);
   }

  m_DragDelta = new QLine();
  m_RectItem = m_View->scene()->addRect(x, y, width, height, QPen(QColor(150,150,150)));

  // Guidelines items init
  for (int i = 0; i <= 3; i++) {
    m_GuideItems[i] = new QGraphicsLineItem();
    m_GuideItems[i]->hide();
    m_GuideItems[i]->setPen(QPen(QColor(150,150,150)));
    m_View->scene()->addItem(m_GuideItems[i]);
  }

  setAspectRatio(FixedAspectRatio, AspectRatioW, AspectRatioH);
}


ptRichRectInteraction::~ptRichRectInteraction() {
  for (int i = 0; i <= 3; i++) {
    m_View->scene()->removeItem(m_GuideItems[i]);
    DelAndNull(m_GuideItems[i]);
  }

  DelAndNull(m_DragDelta);
  DelAndNull(m_RectItem);

  for (int i = 0; i <= 3; i++) {
    DelAndNull(m_LightsOutRects[i]);
  }

}


///////////////////////////////////////////////////////////////////////////
//
// setAspectRatio()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::setAspectRatio(const short FixedAspectRatio,
                                           uint AspectRatioW, uint AspectRatioH,
                                           const short ImmediateUpdate /*= 1*/)
{
  m_FixedAspectRatio = FixedAspectRatio;
  m_AspectRatioW = AspectRatioW;
  m_AspectRatioH = AspectRatioH;

  if (FixedAspectRatio) {
    assert((AspectRatioW != 0) && (AspectRatioH != 0));
    m_AspectRatio = (double) AspectRatioW / (double) AspectRatioH;
  }

  if (ImmediateUpdate) {
    m_MovingEdge = meNone;
    EnforceAspectRatio();
  }
}


///////////////////////////////////////////////////////////////////////////
//
// set guidelines
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::setGuidelines(const short mode) {
  if (m_Guides != mode) {
    m_Guides = mode;
    RecalcGuides();
  }
}


///////////////////////////////////////////////////////////////////////////
//
// seLightsOut()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::setLightsOut(short mode) {
  mode = qBound((short)0, mode, (short)2);
  Settings->SetValue("LightsOut", mode);
  for (int i = 0; i <= 3; i++) {
    m_LightsOutRects[i]->setBrush(*m_LightsOutBrushes[mode]);
  }
}


///////////////////////////////////////////////////////////////////////////
//
// mouseAction() [slot]
// Dispatcher for mouse events received from the parent.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::mouseAction(QMouseEvent* event) {
  switch (event->type()) {
    case QEvent::MouseButtonPress:
      MousePressHandler(event);
      break;
    case QEvent::MouseButtonRelease:
      MouseReleaseHandler(event);
      break;
    case QEvent::MouseButtonDblClick:
      MouseDblClickHandler(event);
      break;
    case QEvent::MouseMove:
      MouseMoveHandler(event);
      break;
    default:
      // ignore other mouse events
      break;
  }
}


///////////////////////////////////////////////////////////////////////////
//
// mouse button press
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::MousePressHandler(const QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    QPointF scPos = m_View->mapToScene(event->pos());

    if (m_RectItem->rect().contains(scPos)) {
      m_NowDragging = 1;
      m_DragDelta->setPoints(event->pos(), event->pos());

    // Start new rect when none is present or clicked outside current one.
    } else if (m_View->scene()->sceneRect().contains(scPos)) {
      m_NowDragging = 1;
      m_MovingEdge = meNone;
      m_RectItem->setRect(scPos.x(), scPos.y(), 0.0, 0.0);
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//
// mouse button release
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::MouseReleaseHandler(const QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_NowDragging = 0;
  }
}


///////////////////////////////////////////////////////////////////////////
//
// mouse double click
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::MouseDblClickHandler(const QMouseEvent* event) {
  if (m_RectItem->contains(m_View->mapToScene(event->pos())) ) {
    m_ExitStatus = stSuccess;
    Finalize();
  }
}


///////////////////////////////////////////////////////////////////////////
//
// mouse move
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::MouseMoveHandler(const QMouseEvent* event) {
  if (m_NowDragging) {
    m_DragDelta->setP2(event->pos());   // GraphicsView scale

    // Transform delta to scene scale. To avoid choppy behaviour we need to ensure the
    // final change/move of the viewport scale rect is at least 1px --> we add 0.5 towards
    // +inf or -inf depending on the sign of dx/dy.
    // m_View->transform().m11() is the scale factor.
//    qreal dx = (m_DragDelta->dx() / m_View->transform().m11() + 0.5 * SIGN(m_DragDelta->dx()));
//    qreal dy = (m_DragDelta->dy() / m_View->transform().m11() + 0.5 * SIGN(m_DragDelta->dy()));
    // TODO SR: forget above comment. Let’s see if it "just works" in floating point.
    qreal dx = m_DragDelta->dx() / m_View->transform().m11();
    qreal dy = m_DragDelta->dy() / m_View->transform().m11();

    // Move current rectangle. The qBounds make sure it stops at image boundaries.
    if (m_NowDragging && (m_CtrlIsPressed > 0)) {
      m_RectItem->rect().moveTo(
          qBound(0.0, m_RectItem->rect().left() + dx,
                 m_View->scene()->sceneRect().width() - m_RectItem->rect().width()),
          qBound(0.0, m_RectItem->rect().top() + dy,
                 m_View->scene()->sceneRect().height() - m_RectItem->rect().height())
      );
      RecalcGuides();
      RecalcLightsOutRects();

    // initialize movement direction when rectangle was just started
    } else {
      if (m_MovingEdge == meNone) {
        if ((dx >= 0) && (dy >= 0)) {
          m_MovingEdge = meBottomRight;
        } else if ((dx < 0) && (dy <= 0)) {
          m_MovingEdge = meBottomLeft;
        } else if ((dx > 0) && (dy < 0)) {
          m_MovingEdge = meTopLeft;
        } else if ((dx > 0) && (dy > 0)) {
          m_MovingEdge = meTopRight;
        }
      }

      UpdateScene();
    }
    m_DragDelta->setP1(m_DragDelta->p2());

  } else {
    m_MovingEdge = MouseDragPos(event);
    UpdateCursor();
  }
}


///////////////////////////////////////////////////////////////////////////
//
// keyAction()
// Slot receiving keyboard events from the parent.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::keyAction(QKeyEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    switch (event->key()) {
      case Qt::Key_Escape:
        event->accept();
        m_ExitStatus = stFailure;
        Finalize();
        break;

      case Qt::Key_Enter:
        event->accept();
        m_ExitStatus = stSuccess;
        Finalize();
        break;

      case Qt::Key_Alt:
        event->accept();
        setLightsOut((Settings->GetInt("LightOut") + 1) % 3);
        break;

      // Ctrl needs to be held down to move the rectangle.
      // m_CtrlIsPressed not a simple bool flag to account for keyboards with
      // multiple ctrl keys. There’s a lot of those. ;-) For each ctrl press the
      // variable is increased, for each release it’s decreased. This is necessary
      // because in cases like press left ctrl - press right ctrl - release left ctrl
      // Photivo should still recognise the ctrl key as being held down.
      case Qt::Key_Control:
        event->accept();
        m_CtrlIsPressed++;
        UpdateCursor();
        break;

      default:
        // nothing to do here
        break;
    }


  } else if (event->type() == QEvent::KeyRelease) {
    if (event->key() == Qt::Key_Control) {
      m_CtrlIsPressed--;
      UpdateCursor();
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//
// UpdateCursor()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::UpdateCursor() {
  if ((m_MovingEdge != meNone) && (m_CtrlIsPressed > 0)) {
    m_View->setCursor(Qt::SizeAllCursor);

  } else {
    switch (m_MovingEdge) {
      case meNone:
      case meCenter:
        m_View->setCursor(Qt::ArrowCursor);
        break;
      case meTop:
      case meBottom:
        m_View->setCursor(Qt::SizeVerCursor);
        break;
      case meLeft:
      case meRight:
        m_View->setCursor(Qt::SizeHorCursor);
        break;
      case meTopLeft:
      case meBottomRight:
        m_View->setCursor(Qt::SizeFDiagCursor);
        break;
      case meTopRight:
      case meBottomLeft:
        m_View->setCursor(Qt::SizeBDiagCursor);
        break;
      default:
        assert(!"Unhandled m_MovingEdge!");
        break;
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//
// stop()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::stop(ptStatus exitStatus) {
  m_ExitStatus = exitStatus;
  Finalize();
}


///////////////////////////////////////////////////////////////////////////
//
// Finalize()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::Finalize() {
  m_View->scene()->removeItem(m_RectItem);
  // Do not delete m_RectItem here! It’s still needed to report the size and
  // position of the rectangle to the caller. Deletion happens in the destructor.
  m_View->setCursor(Qt::ArrowCursor);
  emit finished(m_ExitStatus);
}


///////////////////////////////////////////////////////////////////////////
//
// MoveToCenter()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::moveToCenter(const short horizontal, const short vertical) {
  QPointF NewCenter = m_RectItem->rect().center();

  if (horizontal) {
    NewCenter.setX(m_View->scene()->width() / 2);
  }
  if (vertical) {
    NewCenter.setY(m_View->scene()->height() / 2);
  }

  m_RectItem->rect().moveCenter(NewCenter);
}


///////////////////////////////////////////////////////////////////////////
//
// flipAspectRatio()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::flipAspectRatio() {
  if (!m_FixedAspectRatio) {
    setAspectRatio(0, m_RectItem->rect().height(), m_RectItem->rect().width(), 0);
  } else {
    setAspectRatio(1, m_AspectRatioH, m_AspectRatioW, 0);
  }
}


///////////////////////////////////////////////////////////////////////////
//
// ClampToScene()
//
// Make sure rectangle stays inside sceneRect. Does not change AR, even
// when fixed AR is not set.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::ClampToScene() {
  QPointF center = m_RectItem->rect().center();
  qreal arw = m_RectItem->rect().width();
  qreal arh = m_RectItem->rect().height();

  qreal MaxWidth = qMin(center.x() - m_View->scene()->sceneRect().left(),
                        m_View->scene()->sceneRect().right() - center.x());
  qreal MaxHeight = qMin(center.y() - m_View->scene()->sceneRect().top(),
                         m_View->scene()->sceneRect().bottom() - center.y());

  if (m_RectItem->rect().width() > MaxWidth) {
    m_RectItem->rect().setWidth(MaxWidth);
    m_RectItem->rect().setHeight(MaxWidth * arh / arw);
    MaxHeight = qMin(MaxHeight, m_RectItem->rect().height());
  }

  if (m_RectItem->rect().height() > MaxHeight) {
    m_RectItem->rect().setHeight(MaxHeight);
    m_RectItem->rect().setWidth(MaxHeight * arw / arh);
  }

  m_RectItem->rect().moveCenter(center);
}


///////////////////////////////////////////////////////////////////////////
//
// EnforceAspectRatio()
//
// Make sure rectangle has the proper AR. Wenn adjusted the opposite
// corner/edge to the one that was moved remains fixed.
// When there was no movement (e.g. changed AR values in MainWindow)
// the center of the rectangle remains fixed.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::EnforceAspectRatio(qreal dx /*= 0*/, qreal dy /*= 0*/) {
  dx = qAbs(dx);
  dy = qAbs(dy);
  qreal ImageRight = m_View->scene()->sceneRect().right();
  qreal ImageBottom = m_View->scene()->sceneRect().bottom();
  qreal NewWidth = m_RectItem->rect().height() * m_AspectRatio;
  qreal NewHeight = m_RectItem->rect().width() / m_AspectRatio;
  qreal EdgeCenter = 0.0;

  switch (m_MovingEdge){
    case meTopLeft:
      if (dx > dy) {  // primarily horizontal mouse movement: new width takes precedence
        m_RectItem->rect().setTop(m_RectItem->rect().top() +
                                  m_RectItem->rect().height() - NewHeight);
        if (m_RectItem->rect().top() < 0) {
          m_RectItem->rect().setTop(0);
          m_RectItem->rect().setLeft(m_RectItem->rect().right() -
                                     m_RectItem->rect().height() * m_AspectRatio);
        }
      } else {  // primarily vertical mouse movement: new height takes precedence
        m_RectItem->rect().setLeft(m_RectItem->rect().left() +
                                   m_RectItem->rect().width() - NewWidth);
        if (m_RectItem->rect().left() < 0) {
          m_RectItem->rect().setLeft(0);
          m_RectItem->rect().setTop(m_RectItem->rect().bottom() -
                                    m_RectItem->rect().width() / m_AspectRatio);
        }
      }
      break;

    case meTop:
      EdgeCenter = m_RectItem->rect().left() + m_RectItem->rect().width() / 2;
      m_RectItem->rect().setWidth(NewWidth);
      m_RectItem->rect().moveLeft(EdgeCenter - NewWidth / 2);
      if (m_RectItem->rect().right() > ImageRight) {
        m_RectItem->rect().setRight(ImageRight);
        m_RectItem->rect().setTop(m_RectItem->rect().bottom() -
                                  m_RectItem->rect().width() / m_AspectRatio);
      }
      if (m_RectItem->rect().left() < 0) {
        m_RectItem->rect().setLeft(0);
        m_RectItem->rect().setTop(m_RectItem->rect().bottom() -
                                  m_RectItem->rect().width() / m_AspectRatio);
      }
      break;

    case meTopRight:
      if (dx > dy) {
        m_RectItem->rect().setTop(m_RectItem->rect().top() +
                                  m_RectItem->rect().height() - NewHeight);
        if (m_RectItem->rect().top() < 0) {
          m_RectItem->rect().setTop(0);
          m_RectItem->rect().setRight(m_RectItem->rect().left() +
                                      m_RectItem->rect().height() * m_AspectRatio);
        }
      } else {
        m_RectItem->rect().setWidth(NewWidth);
        if (m_RectItem->rect().right() > ImageRight) {
          m_RectItem->rect().setRight(ImageRight);
          m_RectItem->rect().setTop(m_RectItem->rect().bottom() -
                                    m_RectItem->rect().width() / m_AspectRatio);
        }
      }
      break;

    case meRight:
      EdgeCenter = m_RectItem->rect().top() + m_RectItem->rect().height() / 2;
      m_RectItem->rect().setHeight(NewHeight);
      m_RectItem->rect().moveTop(EdgeCenter - NewHeight / 2);
      if (m_RectItem->rect().bottom() > ImageBottom) {
        m_RectItem->rect().setBottom(ImageBottom);
        m_RectItem->rect().setRight(m_RectItem->rect().left() +
                                    m_RectItem->rect().height() * m_AspectRatio);
      }
      if (m_RectItem->rect().top() < 0) {
        m_RectItem->rect().setTop(0);
        m_RectItem->rect().setRight(m_RectItem->rect().left() +
                                    m_RectItem->rect().height() * m_AspectRatio);
      }
      break;

    case meBottomRight:
      if (dx > dy) {
        m_RectItem->rect().setBottom(m_RectItem->rect().bottom() +
                                     NewHeight - m_RectItem->rect().height());
        if (m_RectItem->rect().bottom() > ImageBottom) {
          m_RectItem->rect().setBottom(ImageBottom);
          m_RectItem->rect().setRight(m_RectItem->rect().left() +
                                      m_RectItem->rect().height() * m_AspectRatio);
        }
      } else {
        m_RectItem->rect().setWidth(NewWidth);
        if (m_RectItem->rect().right() > ImageRight) {
          m_RectItem->rect().setRight(ImageRight);
          m_RectItem->rect().setTop(m_RectItem->rect().bottom() -
                                    m_RectItem->rect().width() / m_AspectRatio);
        }
      }
      break;

    case meBottom:
      EdgeCenter = m_RectItem->rect().left() + m_RectItem->rect().width() / 2;
      m_RectItem->rect().setWidth(NewWidth);
      m_RectItem->rect().moveLeft(EdgeCenter - NewWidth / 2);
      if (m_RectItem->rect().right() > ImageRight) {
        m_RectItem->rect().setRight(ImageRight);
        m_RectItem->rect().setBottom(m_RectItem->rect().top() +
                                     m_RectItem->rect().width() / m_AspectRatio);
      }
      if (m_RectItem->rect().left() < 0) {
        m_RectItem->rect().setLeft(0);
        m_RectItem->rect().setBottom(m_RectItem->rect().top() +
                                     m_RectItem->rect().width() / m_AspectRatio);
      }
      break;

    case meBottomLeft:
      if (dx > dy) {
        m_RectItem->rect().setBottom(m_RectItem->rect().bottom() +
                                     NewHeight - m_RectItem->rect().height());
        if (m_RectItem->rect().bottom() > ImageBottom) {
          m_RectItem->rect().setBottom(ImageBottom);
          m_RectItem->rect().setLeft(m_RectItem->rect().right() -
                                     m_RectItem->rect().height() * m_AspectRatio);
        }
      } else {
        m_RectItem->rect().setLeft(m_RectItem->rect().left() +
                                   m_RectItem->rect().width() - NewWidth);
        if (m_RectItem->rect().left() < 0) {
          m_RectItem->rect().setLeft(0);
          m_RectItem->rect().setTop(m_RectItem->rect().bottom() -
                                    m_RectItem->rect().width() / m_AspectRatio);
        }
      }
      break;

    case meLeft:
      EdgeCenter = m_RectItem->rect().top() + m_RectItem->rect().height() / 2;
      m_RectItem->rect().setHeight(NewHeight);
      m_RectItem->rect().moveTop(EdgeCenter - NewHeight / 2);
      if (m_RectItem->rect().bottom() > ImageBottom) {
        m_RectItem->rect().setBottom(ImageBottom);
        m_RectItem->rect().setLeft(m_RectItem->rect().right() -
                                   m_RectItem->rect().height() * m_AspectRatio);
      }
      if (m_RectItem->rect().top() < 0) {
        m_RectItem->rect().setTop(0);
        m_RectItem->rect().setLeft(m_RectItem->rect().right() -
                                   m_RectItem->rect().height() * m_AspectRatio);
      }
      break;

    case meNone: {
      // Calculate rect with new AR and about the same area as the old one according to:
      // OldWidth * OldHeight = x * NewARW * x * NewARH
      // x = sqrt((OldWidth * OldHeight) / (NewARW * NewARH))
      // Center point stays the same.
      QPointF center = m_RectItem->rect().center();
      float x = sqrt((m_RectItem->rect().width() * m_RectItem->rect().height()) /
                     (m_AspectRatioW * m_AspectRatioH));
      m_RectItem->rect().setWidth(x * m_AspectRatioW);
      m_RectItem->rect().setHeight(x * m_AspectRatioH);
      m_RectItem->rect().moveCenter(center);
      ClampToScene();
      break;
    }

    default:
      assert(!"Unhandled m_MovingEdge!");
  }
}


////////////////////////////////////////////////////////////////////////
//
// MouseDragPos
//
// Returns the area of the crop/selection rectangle the mouse cursor hovers over.
// The mouse position inside the crop rectangle determines which action is performed
// on drag. There are nine areas: 55511111111666
//                                444        222
//                                77733333333888
// - Dragging the corners changes both adjacent edges.
// - Dragging the edges changes only that edge. Dragging in the middle area moves
//   the rectangle without changing its size.
// - Dragging beyond actual image borders is not possible.
// - Edge areas are usually EdgeThickness pixels thick. On hovering mouse cursor
//   changes shape to indicate the move/resize mode.
// - For rectangle edges of TinyRectThreshold pixels or shorter only the corner modes
//   apply, one for each half of the edge. This avoids too tiny interaction areas.
//
////////////////////////////////////////////////////////////////////////


ptMovingEdge ptRichRectInteraction::MouseDragPos(const QMouseEvent* event) {
  QPointF pos(m_View->mapToScene(event->pos()));

  // Catch mouse outside current crop rect
  if (!m_RectItem->contains(pos)) {
    return meNone;
  }

  ptMovingEdge HoverOver = meNone;
  int TBthick = 0;    // top/bottom
  int LRthick = 0;    // left/right

  // Determine edge area thickness
  if (m_RectItem->rect().height() <= TinyRectThreshold) {
    TBthick = (int)(m_RectItem->rect().height() / 2);
  } else {
    TBthick = EdgeThickness;
  }
  if (m_RectItem->rect().width() <= TinyRectThreshold) {
    LRthick = (int)(m_RectItem->rect().width() / 2);
  } else {
    LRthick = EdgeThickness;
  }

  // Determine in which area the mouse is
  if (m_RectItem->rect().bottom() - pos.y() <= TBthick) {
    HoverOver = meBottom;
  } else if (pos.y() - m_RectItem->rect().top() <= TBthick) {
    HoverOver = meTop;
  } else {
    HoverOver = meCenter;
  }

  if (m_RectItem->rect().right() - pos.x() <= LRthick) {
    if (HoverOver == meBottom) {
      HoverOver = meBottomRight;
    } else if (HoverOver == meTop) {
      HoverOver = meTopRight;
    } else {
      HoverOver = meRight;
    }

  } else if (pos.x() - m_RectItem->rect().left() <= LRthick) {
    if (HoverOver == meBottom) {
      HoverOver = meBottomLeft;
    } else if (HoverOver == meTop) {
      HoverOver = meTopLeft;
    } else {
      HoverOver = meLeft;
    }
  }

  return HoverOver;
}


///////////////////////////////////////////////////////////////////////////
//
// RecalcRect()
// Calc new corner point, change moving edge/corner when rect is
// dragged into another "quadrant", and update rectangle.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::RecalcRect() {
  QRectF NewPos;
  qreal ImageRight = m_View->scene()->sceneRect().right();
  qreal ImageBottom = m_View->scene()->sceneRect().bottom();
  qreal dx = m_DragDelta->dx() / m_View->transform().m11();
  qreal dy = m_DragDelta->dy() / m_View->transform().m11();
  qreal dxCeil = dx;
  qreal dyCeil = dy;
  qreal dxFloor = dx;
  qreal dyFloor = dy;

  switch (m_MovingEdge) {
    case meTopLeft:
      // Calc preliminary new position of moved corner point. Don't allow it to move
      // beyond the actual image.
      NewPos.setX(qBound(0.0, m_RectItem->rect().left() + dxFloor, ImageRight));
      NewPos.setY(qBound(0.0, m_RectItem->rect().top() + dyFloor, ImageBottom));
      // Determine if moving corner changed
      if ((NewPos.x() > m_RectItem->rect().right()) && (NewPos.y() <= m_RectItem->rect().bottom())) {
        m_MovingEdge = meTopRight;
      } else if ((NewPos.x() > m_RectItem->rect().right()) && (NewPos.y() >= m_RectItem->rect().bottom())) {
        m_MovingEdge = meBottomRight;
      } else if ((NewPos.x() <= m_RectItem->rect().right()) && (NewPos.y() > m_RectItem->rect().bottom())) {
        m_MovingEdge = meBottomLeft;
      }
      // Update crop rect
      m_RectItem->rect().setCoords(qMin(NewPos.x(), m_RectItem->rect().right()),
                                qMin(NewPos.y(), m_RectItem->rect().bottom()),
                                qMax(NewPos.x(), m_RectItem->rect().right()),
                                qMax(NewPos.y(), m_RectItem->rect().bottom()) );
      break;


    case meTop:
      dx = 0;
      NewPos.setY(qBound(0.0, m_RectItem->rect().top() + dyFloor, ImageBottom));
      if (NewPos.y() > m_RectItem->rect().bottom()) {
        m_MovingEdge = meBottom;
      }
      m_RectItem->rect().setCoords(m_RectItem->rect().left(),
                                qMin(NewPos.y(), m_RectItem->rect().bottom()),
                                m_RectItem->rect().right(),
                                qMax(NewPos.y(), m_RectItem->rect().bottom()) );
      break;

    case meTopRight:
      NewPos.setX(qBound(0.0, m_RectItem->rect().right() + dxCeil, ImageRight));
      NewPos.setY(qBound(0.0, m_RectItem->rect().top() + dyFloor, ImageBottom));
      if ((NewPos.x() < m_RectItem->rect().left()) && (NewPos.y() <= m_RectItem->rect().bottom())) {
        m_MovingEdge = meTopLeft;
      } else if ((NewPos.x() < m_RectItem->rect().left()) && (NewPos.y() > m_RectItem->rect().bottom())) {
        m_MovingEdge = meBottomLeft;
      } else if ((NewPos.x() >= m_RectItem->rect().left()) && (NewPos.y() > m_RectItem->rect().bottom())) {
        m_MovingEdge = meBottomRight;
      }
      m_RectItem->rect().setCoords(MIN(NewPos.x(), m_RectItem->rect().left()),
                                MIN(NewPos.y(), m_RectItem->rect().bottom()),
                                MAX(NewPos.x(), m_RectItem->rect().left()),
                                MAX(NewPos.y(), m_RectItem->rect().bottom()) );
      break;

    case meRight:
      dy = 0;
      NewPos.setX(qBound(0.0, m_RectItem->rect().right() + dxCeil, ImageRight));
      if ((NewPos.x() < m_RectItem->rect().left())) {
        m_MovingEdge = meLeft;
      }
      m_RectItem->rect().setCoords(MIN(NewPos.x(), m_RectItem->rect().left()),
                                m_RectItem->rect().top(),
                                MAX(NewPos.x(), m_RectItem->rect().left()),
                                m_RectItem->rect().bottom() );
      break;

    case meBottomRight:
      NewPos.setX(qBound(0.0, m_RectItem->rect().right() + dxCeil, ImageRight));
      NewPos.setY(qBound(0.0, m_RectItem->rect().bottom() + dyCeil, ImageBottom));
      if ((NewPos.x() < m_RectItem->rect().left()) && (NewPos.y() >= m_RectItem->rect().top())) {
        m_MovingEdge = meBottomLeft;
      } else if ((NewPos.x() < m_RectItem->rect().left()) && (NewPos.y() < m_RectItem->rect().top())) {
        m_MovingEdge = meTopLeft;
      } else if ((NewPos.x() >= m_RectItem->rect().left()) && (NewPos.y() < m_RectItem->rect().top())) {
        m_MovingEdge = meTopRight;
      }
      m_RectItem->rect().setCoords(MIN(NewPos.x(), m_RectItem->rect().left()),
                                MIN(NewPos.y(), m_RectItem->rect().top()),
                                MAX(NewPos.x(), m_RectItem->rect().left()),
                                MAX(NewPos.y(), m_RectItem->rect().top()) );
      break;

    case meBottom:
      dx = 0;
      NewPos.setY(qBound(0.0, m_RectItem->rect().bottom() + dy, ImageBottom));
      if (NewPos.y() < m_RectItem->rect().top()) {
        m_MovingEdge = meTop;
      }
      m_RectItem->rect().setCoords(m_RectItem->rect().left(),
                                MIN(NewPos.y(), m_RectItem->rect().top()),
                                m_RectItem->rect().right(),
                                MAX(NewPos.y(), m_RectItem->rect().top()) );
      break;

    case meBottomLeft:
      NewPos.setX(qBound(0.0, m_RectItem->rect().left() + dx, ImageRight));
      NewPos.setY(qBound(0.0, m_RectItem->rect().bottom() + dy, ImageBottom));
      if ((NewPos.x() > m_RectItem->rect().right()) && (NewPos.y() >= m_RectItem->rect().top())) {
        m_MovingEdge = meBottomRight;
      } else if ((NewPos.x() > m_RectItem->rect().right()) && (NewPos.y() < m_RectItem->rect().top())) {
        m_MovingEdge = meTopRight;
      } else if ((NewPos.x() <= m_RectItem->rect().right()) && (NewPos.y() < m_RectItem->rect().top())) {
        m_MovingEdge = meTopLeft;
      }
      m_RectItem->rect().setCoords(MIN(NewPos.x(), m_RectItem->rect().right()),
                                MIN(NewPos.y(), m_RectItem->rect().top()),
                                MAX(NewPos.x(), m_RectItem->rect().right()),
                                MAX(NewPos.y(), m_RectItem->rect().top()) );
      break;

    case meLeft:
      dy = 0;
      NewPos.setX(qBound(0.0, m_RectItem->rect().left() + dx, ImageRight));
      if (NewPos.x() > m_RectItem->rect().right()) {
        m_MovingEdge = meRight;
      }
      m_RectItem->rect().setCoords(MIN(NewPos.x(), m_RectItem->rect().right()),
                                m_RectItem->rect().top(),
                                MAX(NewPos.x(), m_RectItem->rect().right()),
                                m_RectItem->rect().bottom() );
      break;


    case meNone:
      // nothing to do here
      break;

    default:
      assert(!"Unhandled m_MovingEdge");
      break;
  }

  if (m_FixedAspectRatio) {
    EnforceAspectRatio(dx, dy);
  }
}


///////////////////////////////////////////////////////////////////////////
//
// RecalcLightsOutRects()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::RecalcGuides() {
  if (Settings->GetInt("LightsOut") != ptLightsOutMode_Black) {

    switch (m_Guides) {
      case ptGuidelines_RuleThirds: {
        qreal HeightThird = m_RectItem->rect().height() / 3.0;
        qreal WidthThird = m_RectItem->rect().width() / 3.0;

        m_GuideItems[0]->line().setLine(m_RectItem->rect().left() + WidthThird,
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().left() + WidthThird,
                                        m_RectItem->rect().bottom());
        m_GuideItems[1]->line().setLine(m_RectItem->rect().right() - WidthThird,
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().right() - WidthThird,
                                        m_RectItem->rect().bottom());
        m_GuideItems[2]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().top() + HeightThird,
                                        m_RectItem->rect().right(),
                                        m_RectItem->rect().top() + HeightThird);
        m_GuideItems[3]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().bottom() - HeightThird,
                                        m_RectItem->rect().right(),
                                        m_RectItem->rect().bottom() - HeightThird);

        for (int i = 0; i <= 3; i++) {
          m_GuideItems[i]->show();
        }
        break;
      }


      case ptGuidelines_GoldenRatio: {
        qreal ShortWidth = m_RectItem->rect().width() * 5.0/13.0;
        qreal ShortHeight = m_RectItem->rect().height() * 5.0/13.0;

        m_GuideItems[0]->line().setLine(m_RectItem->rect().left() + ShortWidth,
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().left() + ShortWidth,
                                        m_RectItem->rect().bottom());
        m_GuideItems[1]->line().setLine(m_RectItem->rect().right() - ShortWidth,
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().right() - ShortWidth,
                                        m_RectItem->rect().bottom());
        m_GuideItems[2]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().top() + ShortHeight,
                                        m_RectItem->rect().right(),
                                        m_RectItem->rect().top() + ShortHeight);
        m_GuideItems[3]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().bottom() - ShortHeight,
                                        m_RectItem->rect().right(),
                                        m_RectItem->rect().bottom() - ShortHeight);

        for (int i = 0; i <= 3; i++) {
          m_GuideItems[i]->show();
        }
        break;
      }


      case ptGuidelines_Diagonals: {
        qreal length = m_RectItem->rect().width() > m_RectItem->rect().height()
                       ? m_RectItem->rect().height()
                       : m_RectItem->rect().width();

        m_GuideItems[0]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().left() + length,
                                        m_RectItem->rect().top() + length);
        m_GuideItems[1]->line().setLine(m_RectItem->rect().right(),
                                        m_RectItem->rect().bottom(),
                                        m_RectItem->rect().right() - length + 1,
                                        m_RectItem->rect().bottom() - length + 1);
        m_GuideItems[2]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().bottom(),
                                        m_RectItem->rect().left() + length,
                                        m_RectItem->rect().bottom() - length + 1);
        m_GuideItems[3]->line().setLine(m_RectItem->rect().right(),
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().right() - length + 1,
                                        m_RectItem->rect().top() + length);

        for (int i = 0; i <= 3; i++) {
          m_GuideItems[i]->show();
        }
        break;
      }


      case ptGuidelines_Centerlines: {
        m_GuideItems[0]->line().setLine(m_RectItem->rect().center().x(),
                                        m_RectItem->rect().top(),
                                        m_RectItem->rect().center().x(),
                                        m_RectItem->rect().bottom());
        m_GuideItems[1]->line().setLine(m_RectItem->rect().left(),
                                        m_RectItem->rect().center().y(),
                                        m_RectItem->rect().right(),
                                        m_RectItem->rect().center().y());

        m_GuideItems[0]->show();
        m_GuideItems[1]->show();
        m_GuideItems[2]->hide();
        m_GuideItems[3]->hide();
      }


      default:
        // nothing to do here
        break;
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//
// RecalcLightsOutRects()
//
// Array index order: top, right, bottom, left
// Up to four rectangles are drawn according to the following figure.
// tttttttttttttttttt
// lll            rrr
// lll            rrr
// bbbbbbbbbbbbbbbbbb
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::RecalcLightsOutRects() {
  m_LightsOutRects[0]->rect().setCoords(    // top
      m_View->scene()->sceneRect().left(),
      m_View->scene()->sceneRect().top(),
      m_View->scene()->sceneRect().width(),
      m_RectItem->rect().top() - m_View->scene()->sceneRect().top()
  );

  m_LightsOutRects[1]->rect().setCoords(    // right
      m_RectItem->rect().right() + 1,
      m_RectItem->rect().top(),
      m_View->scene()->sceneRect().right() - m_RectItem->rect().right(),
      m_RectItem->rect().height()
  );

  m_LightsOutRects[2]->rect().setCoords(    // bottom
      m_View->scene()->sceneRect().left(),
      m_RectItem->rect().bottom() + 1,
      m_View->scene()->sceneRect().width(),
      m_View->scene()->sceneRect().bottom() - m_RectItem->rect().bottom()
  );

  m_LightsOutRects[3]->rect().setCoords(    // left
      m_View->scene()->sceneRect().left(),
      m_RectItem->rect().top(),
      m_RectItem->rect().left() - m_View->scene()->sceneRect().left(),
      m_RectItem->rect().height()
  );
}


///////////////////////////////////////////////////////////////////////////
//
// UpdateScene()
// Update all elements in the scene
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::UpdateScene() {
  RecalcRect();
  RecalcGuides();
  RecalcLightsOutRects();
}
