/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#ifndef DLVIEWWINDOW_H
#define DLVIEWWINDOW_H

#include <QtGui>
#include <QRect>
#include <QLine>

#include "ptImage.h"


///////////////////////////////////////////////////////////////////////////
//
// Custom types used in ViewWindow
//
///////////////////////////////////////////////////////////////////////////

// Crop: Position of the mouse when button pressed for dragging
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
  meCenter = 9      // move crop rect instead of resize
};

// User interaction in the view window
enum ptViewportAction {
  vaNone = 0,
  vaCrop = 1,
  vaSelectRect = 2,
  vaDrawLine = 3
};



///////////////////////////////////////////////////////////////////////////
//
// class ptViewWindow
//
///////////////////////////////////////////////////////////////////////////

class ptViewWindow : public QAbstractScrollArea {

Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////

public:
  ptViewWindow(const ptImage* RelatedImage,
                     QWidget* Parent);
  ~ptViewWindow();

  // NewRelatedImage to associate anonter ptImage with this window.
  void UpdateView(const ptImage* NewRelatedImage = NULL);

  /*user interaction:
    - Selection: mouse push/drag/release to draw a selection rectangle.
      Used for spot WB and histogram "crop".
    - Cropping: usage similar to Gimp’s crop tool. Used for image cropping.
    - Line drawing: mouse push/drag/release to draw a line. Used for image rotation angle.
  */
  void StartCrop(const int x,
                 const int y,
                 const int width,
                 const int height,
                 const short FixedAspectRatio,
                 const uint AspectRatioW,
                 const uint AspectRatioH,
                 const short CropGuidelines);

  QRect   StopCrop();   // QRect scale is 100% zoom and current pipe size
  void    StartSelection();
  void    StartLine();
  ptViewportAction  OngoingAction();
  double  GetRotationAngle();
  QRect   GetRectangle();
  void    setCropGuidelines(const short CropGuidelines);
  void    setAspectRatio(const short FixedAspectRatio,
                         uint AspectRatioW,
                         uint AspectRatioH,
                         const short ImmediateUpdate = 1);
  void    setGrid(const short Enabled, const short GridX, const short GridY);

  // Zoom functions. Fit returns the factor in %.
  short ZoomFit();
  short ZoomFitFactor(const uint16_t Width, const uint16_t Height);
  void  Zoom(const short Factor, const short Update = 1); // Expressed in %
  void  ToggleLightsOut();

  // Status report
  void StatusReport(const short State);
  void StatusReport(const QString Text);

  short       m_CropLightsOut;

  QAction*    m_AtnFullScreen;


///////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
///////////////////////////////////////////////////////////////////////////

protected:
  // overloaded virtual ones.
  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);
  void mousePressEvent(QMouseEvent* Event);
  void mouseDoubleClickEvent(QMouseEvent* Event);
  void mouseMoveEvent(QMouseEvent* Event);
  void mouseReleaseEvent(QMouseEvent* Event);
  void contextMenuEvent(QContextMenuEvent* Event);
  void wheelEvent(QWheelEvent* Event);
  void scrollContentsBy(int dx, int dy);
  void leaveEvent(QEvent*);
  void keyPressEvent(QKeyEvent* Event);
  void keyReleaseEvent(QKeyEvent* Event);


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////

private:
  // constants
  const int EdgeThickness;
  const int TinyRectThreshold;

  // methods
  void          RecalcCut();
  void          RecalcRect();
  void          EnforceRectAspectRatio(int dx = 0, int dy = 0);   // force fixed AR for a crop rectangle
  ptMovingEdge  MouseDragPos(QMouseEvent* Event);
  void          ContextMenu(QEvent* Event);
  void          FinalizeAction();
  void          UpdateViewportRects();

  //variables
  const ptImage*   m_RelatedImage;    // 16bit pipe sized image //TODO: Really needed as a member?
  QImage*          m_QImage;          // 8bit pipe sized image (directly converted from m_RelatedImage)
  QImage*          m_QImageZoomed;    // m_QImage scaled to current zoom factor
  QImage*          m_QImageCut;       // visible part of the zoomed image

  /* Crop/selection rectangles:
     m_PipeSizeRect: Master rect defining current values in image pipe size. No viewport
          offsets involed, i.e. coordinate system always starts topleft at (0,0).
     m_ViewSizeRect: Current rectrangle in viewport scale including any offsets if the
          visible part of the image is smaller than the viewport.
     m_ImageFrame: Defines position and size of the visible part of the image in the
          viewport. Width/height are always the same as m_QImageCut.
  */
  QRect*      m_ViewSizeRect;
  QRect*      m_PipeSizeRect;
  QRect*      m_ImageFrame;

  QLine*      m_DragDelta;      // direction and distance of mouse drag (viewport scale)
  short       m_NowDragging;    // flag, if mouse drag is ongoing
  ptMovingEdge      m_MovingEdge;
  ptMovingEdge      m_PrevMovingEdge;
  ptViewportAction  m_InteractionMode;
  short       m_FixedAspectRatio;   // 0: fixed AR, 1: no AR restriction
  uint        m_AspectRatioW;
  uint        m_AspectRatioH;
  double      m_AspectRatio;        //  m_AspectRatioW / m_AspectRatioH
  short       m_CropGuidelines;
  double      m_ZoomFactor;
  double      m_PreviousZoomFactor;
  uint16_t    m_ZoomWidth;
  uint16_t    m_ZoomHeight;
  short       m_HasGrid;
  short       m_GridX;
  short       m_GridY;

  QAction*      m_AtnExpIndicate;
  QAction*      m_AtnExpIndR;
  QAction*      m_AtnExpIndG;
  QAction*      m_AtnExpIndB;
  QAction*      m_AtnExpIndOver;
  QAction*      m_AtnExpIndUnder;
  QAction*      m_AtnExpIndSensor;
  QAction*      m_AtnShowBottom;
  QAction*      m_AtnShowTools;
  QAction*      m_AtnZoomFit;
  QAction*      m_AtnZoom100;
  QAction*      m_AtnModeRGB;
  QAction*      m_AtnModeL;
  QAction*      m_AtnModeA;
  QAction*      m_AtnModeB;
  QAction*      m_AtnModeGradient;
  QAction*      m_AtnModeStructure;
  QActionGroup* m_ModeGroup;

  QLabel*     m_SizeReport;
  QString     m_SizeReportText;
  int         m_SizeReportTimeOut;
  QTimer*     m_SizeReportTimer;
  QLabel*     m_StatusReport;
  int         m_StatusReportTimeOut;
  QTimer*     m_StatusReportTimer;
  int         m_ResizeTimeOut;
  QTimer*     m_ResizeTimer;
  int         m_NewSize;


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE SLOTS
//
///////////////////////////////////////////////////////////////////////////

private slots:
  void MenuExpIndicate();
  void MenuExpIndOver();
  void MenuExpIndUnder();
  void MenuExpIndR();
  void MenuExpIndG();
  void MenuExpIndB();
  void MenuExpIndSensor();
  void MenuShowBottom();
  void MenuShowTools();
  void MenuFullScreen();
  void MenuZoomFit();
  void MenuZoom100();
  void MenuMode();
  void SizeReportTimerExpired();
  void StatusReportTimerExpired();
  void ResizeTimerExpired();
};

#endif
