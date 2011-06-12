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

#ifndef PTRICHRECTINTERACTION_H
#define PTRICHRECTINTERACTION_H

#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QRectF>
#include <QRect>

#include "ptImageInteraction.h"

///////////////////////////////////////////////////////////////////////////
//
// Custom types used by ptRichRectInteraction
//
///////////////////////////////////////////////////////////////////////////

// Position of the mouse when button pressed for dragging
enum ptMovingEdge {
  meNone = 0,
  meTop = 1,
  meRight = 2,
  meBottom = 3,
  meLeft = 4,
  meTopLeft = 5,
  meTopRight = 6,
  meBottomLeft = 7,
  meBottomRight = 8,
  meCenter = 9
};


///////////////////////////////////////////////////////////////////////////
//
// class ptSimpleRectInteraction
//
///////////////////////////////////////////////////////////////////////////
class ptRichRectInteraction : public ptImageInteraction {
Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  explicit ptRichRectInteraction(QGraphicsView* View,
                                 const int x,
                                 const int y,
                                 const int width,
                                 const int height,
                                 const short FixedAspectRatio,
                                 const uint AspectRatioW,
                                 const uint AspectRatioH,
                                 const short CropGuidelines);
  ~ptRichRectInteraction();

  inline ptStatus exitStatus() const { return m_ExitStatus; }
  inline short isDragging() const { return m_NowDragging; }
  void flipAspectRatio();
  void moveToCenter(const short horizontal, const short vertical);  
  inline QRect rect() { return QRect(m_RectItem->rect().toRect()); }
  void setAspectRatio(const short FixedAspectRatio,
                      uint AspectRatioW,
                      uint AspectRatioH,
                      const short ImmediateUpdate = 1);
  void setGuidelines(const short mode);
  void setLightsOut(short mode);
  void stop(ptStatus exitStatus);

///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////
private:
  const int EdgeThickness;
  const int TinyRectThreshold;

  QLine*      m_DragDelta;
  QGraphicsLineItem* m_GuideItems[4];
  QBrush*     m_LightsOutBrushes[3];
  QGraphicsRectItem* m_LightsOutRects[4];
  QGraphicsRectItem* m_RectItem;

  qreal       m_AspectRatio;        //  m_AspectRatioW / m_AspectRatioH
  uint        m_AspectRatioW;
  uint        m_AspectRatioH;
  int         m_CtrlIsPressed;
  short       m_FixedAspectRatio;   // 0: fixed AR, 1: no AR restriction
  short       m_Guides;
  ptMovingEdge m_MovingEdge;
  short       m_NowDragging;
  ptStatus    m_ExitStatus;

  void ClampToScene();
  void ClearGuidesList();
  void CreateGuidesList();
  void EnforceAspectRatio(qreal dx = 0, qreal dy = 0);
  void Finalize();
  ptMovingEdge MouseDragPos(const QMouseEvent* event);
  void RecalcRect();
  void RecalcGuides();
  void RecalcLightsOutRects();
  void UpdateScene();
  void UpdateCursor();

  void MousePressHandler(const QMouseEvent* event);
  void MouseReleaseHandler(const QMouseEvent* event);
  void MouseDblClickHandler(const QMouseEvent* event);
  void MouseMoveHandler(const QMouseEvent* event);

///////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
///////////////////////////////////////////////////////////////////////////
private slots:
  void keyAction(QKeyEvent* event);
  void mouseAction(QMouseEvent* event);

};

#endif // PTRICHRECTINTERACTION_H
