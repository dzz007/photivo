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
#include "ptEnums.h"


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
                 const uint16_t AspectRatioW,
                 const uint16_t AspectRatioH,
                 const short CropGuidelines);

  QRect   StopCrop();   // QRect scale is 100% zoom and current pipe size
  void    StartSelection();
  void    StartLine();
  ptViewportAction  OngoingAction();
  double  GetRotationAngle();
  QRect   GetRectangle();

  void Grid(const short Enabled, const short GridX, const short GridY);

  // Zoom functions. Fit returns the factor in %.
  short ZoomFit();
  short ZoomFitFactor(const uint16_t Width, const uint16_t Height);
  void  Zoom(const short Factor, const short Update = 1); // Expressed in %
  void  LightsOut();

  // Status report
  void StatusReport(const short State);
  void StatusReport(const QString Text);

  const ptImage*       m_RelatedImage;

  int16_t              m_StartDragX;  //TODOBJ: should all probably be private or deleted
  int16_t              m_StartDragY;
  int16_t              m_EndDragX;
  int16_t              m_EndDragY;
  uint16_t             m_StartX; // Offset of the shown part into the image.
  uint16_t             m_StartY;
  uint16_t             m_XOffsetInVP; // For images smaller than viewport
  uint16_t             m_YOffsetInVP;
  double               m_ZoomFactor;

  // Order reflects also order into the pipe :
  // The original->Zoom->Cut (to visible) ->pixmap for acceleration.
  QImage*              m_QImage;
  QImage*              m_QImageZoomed;
  QImage*              m_QImageCut;

  QAction*    m_AtnFullScreen;


///////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
///////////////////////////////////////////////////////////////////////////

protected:
  // overloaded virtual ones.
  void paintEvent(QPaintEvent* Event);
  void resizeEvent(QResizeEvent* Event);
  void mousePressEvent(QMouseEvent* Event);
  void mouseMoveEvent(QMouseEvent* Event);
  void mouseReleaseEvent(QMouseEvent* Event);
  void contextMenuEvent(QContextMenuEvent* Event);
  void wheelEvent(QWheelEvent* Event);
  void dragEnterEvent(QDragEnterEvent* Event);
  void dropEvent(QDropEvent* Event);
  void scrollContentsBy(int dx, int dy);


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////

private:
  void          RecalcCut();
  void          RecalcRect();
  ptMovingEdge  MouseDragPos(QMouseEvent* Event);
  void          ContextMenu(QEvent* Event);
  void          FinalizeAction();

  uint16_t    m_ZoomWidth;
  uint16_t    m_ZoomHeight;
  double      m_PreviousZoomFactor;
  short       m_HasGrid;
  short       m_GridX;
  short       m_GridY;
  short       m_CropGuidelines;
  short       m_CropLightsOut;
  double      m_HOverW;

  ptViewportAction  m_Action;
  QRect*      m_Rect;           // crop/selection rectangle in viewport scale
  QRect*      m_RealSizeRect;
  QRect*      m_Frame;          // (visible part of the) image in the viewport
  QLine*      m_DragDelta;
  short       m_DeltaToEdgeX;   // delta between mouse pos and rect edge
  short       m_DeltaToEdgeY;   // "
  short       m_NowDragging;  
  ptMovingEdge  m_MovingEdge;
  QCursor     m_Cursor[];
  short       m_FixedAspectRatio;
  uint16_t    m_AspectRatioW;
  uint16_t    m_AspectRatioH;
  double      m_AspectRatio;

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
