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

#include "ptRichRectInteraction.h"
#include "ptDefines.h"

///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////

ptRichRectInteraction::ptRichRectInteraction(QGraphicsView* View,
      const int x, const int y, const int width, const int height,
      const short FixedAspectRatio, const uint AspectRatioW,
      const uint AspectRatioH, const short CropGuidelines)
: ptImageInteraction(View),
  //constants
  EdgeThickness(20),
  TinyRectThreshold(40),
  //variables
  m_AspectRatioW(AspectRatioW),
  m_AspectRatioH(AspectRatioH),
  m_CropGuidelines(CropGuidelines),
  m_FixedAspectRatio(FixedAspectRatio)
{
  m_Rect = new QRectF();
  m_RectItem = NULL;
}


ptRichRectInteraction::~ptRichRectInteraction() {
  delete m_Rect;
  delete m_RectItem;
}




///////////////////////////////////////////////////////////////////////////
//
// setAspectRatio()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::setAspectRatio(const short FixedAspectRatio, uint AspectRatioW,
                                           uint AspectRatioH, const short ImmediateUpdate)
{
  m_FixedAspectRatio = FixedAspectRatio;
  if (m_FixedAspectRatio) {
    assert((AspectRatioW != 0) && (AspectRatioH != 0));
    m_AspectRatioW = AspectRatioW;
    m_AspectRatioH = AspectRatioH;
    m_AspectRatio = (double) AspectRatioW / (double) AspectRatioH;

    if (ImmediateUpdate) {
//      m_MovingEdge = meNone;
//      EnforceRectAspectRatio();
//      viewport()->repaint();
    }
  }
}



///////////////////////////////////////////////////////////////////////////
//
// mouseAction()
// Slot receiving mouse events from the parent.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::mouseAction(QMouseEvent* event) {
  switch (event->type()) {
    // left button press
    case QEvent::MouseButtonPress: {
      if (event->button() == Qt::LeftButton) {
      }
      break;
    }


    // left button release
    case QEvent::MouseButtonRelease: {
      if (event->button() == Qt::LeftButton) {
      }
      break;
    }


    // dblclick:
    case QEvent::MouseButtonDblClick: {
    if (m_RectItem->contains(m_View->mapToScene(event->pos())) ) {
      Finalize();
    }
      break;
    }


    // mouse move
    case QEvent::MouseMove: {
      break;
    }


    default: {
      assert(!"Wrong event type");
      break;
    }
  } //switch
}



///////////////////////////////////////////////////////////////////////////
//
// keyAction()
// Slot receiving keyboard events from the parent.
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::keyAction(QKeyEvent* event) {

}


///////////////////////////////////////////////////////////////////////////
//
// Finalize()
//
///////////////////////////////////////////////////////////////////////////

void ptRichRectInteraction::Finalize() {
  m_View->scene()->removeItem(m_RectItem);
  DelAndNull(m_RectItem);
  emit finished();
}
