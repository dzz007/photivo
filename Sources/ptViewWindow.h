////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
// Copyright (C) 2011 Bernd Schoeler <brother.john@photivo.org>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DLVIEWWINDOW_H
#define DLVIEWWINDOW_H

#include <QtGui>
#include <QRect>

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

/* CROPPING
   Photivo has two cropping modes used for different purposes.
   - Old-style cropping (constantly pressing mouse button): Managed by AllowSelection and
     SelectionOngoing. Used for spot WB and histogram "crop".
     TODO: Remove all the fixed AR stuff that is not needed here
   - New-style cropping (similar to Gimp): Managed by AllowCrop and CropOngoing.
     Used for image cropping.
*/
// Allow to select in the image. (push/drag/release events).
// Argument is 0 or 1.
// If FixedAspectRatio, then the selection box has the HOverW ratio.

  ptViewportAction GetAction();
  void StopAction();
  void StartCrop(const int AspectRatioW,
                 const int AspectRatioH,
                 const short CropGuidelines = ptCropGuidelines_None,
                 QRect InitialRect = NULL);

  void AllowSelection(const short  Allow,
                      const short  FixedAspectRatio = 0,
                      const double HOverW = 2.0/3,
                      const short  CropGuidelines = ptCropGuidelines_None);

  void StartLine();
  void StartSelection();

  // Returns 1 if selection process is ongoing.
  short SelectionOngoing();

  // New-style cropping. Only for actual image crop, NOT for histogram or spot WB "crop".
  // Allow is 0 (end crop) or 1 (start crop or change crop parameters)
  // Any of the AR parameters being 0 means: no AR restriction
  void AllowCrop(const short Allow,
                 const int AspectRatioW = 0,
                 const int AspectRatioH = 0,
                 const short CropGuidelines = ptCropGuidelines_None);

  // Returns 1 if crop process is ongoing.
  short CropOngoing();

      // Results of selection.
  // Expressed in terms of the RelatedImage.
  uint16_t GetSelectionX();
  uint16_t GetSelectionY();
  uint16_t GetSelectionWidth();
  uint16_t GetSelectionHeight();
  double   GetSelectionAngle();

  // Grid
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

  int16_t              m_StartDragX;
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
  void        RecalculateCut();
  void        ContextMenu(QEvent* Event);

  uint16_t    m_ZoomWidth;
  uint16_t    m_ZoomHeight;
  double      m_PreviousZoomFactor;
  short       m_HasGrid;
  short       m_GridX;
  short       m_GridY;
  short       m_DrawRectangle;    // draw crop rectangle
  short       m_DrawRotateLine;
  short       m_CropGuidelines;
  short       m_CropLightsOut;
  short       m_SelectionAllowed;   // TODO: BJ: Aren’t those two basically the same?
  short       m_SelectionOngoing;   // Do we really need both?
  double      m_HOverW;
  short       m_CropAllowed;        // On/off status of interactive crop mode
  short       m_FixedAspectRatio;
  uint16_t    m_CropARW;
  uint16_t    m_CropARH;
  short       m_CropRectChange;   // off (0), user is resizing (1) or moving (2) crop rectangle
  short       m_CropRectIsFullImage;

  ptViewportAction m_Action;
  QRect*      m_Rect;     // crop/selection rectangle
  QRect*      m_Frame;    // (visible part of the) image in the viewport

  QAction*    m_AtnExpIndicate;
  QAction*    m_AtnExpIndR;
  QAction*    m_AtnExpIndG;
  QAction*    m_AtnExpIndB;
  QAction*    m_AtnExpIndOver;
  QAction*    m_AtnExpIndUnder;
  QAction*    m_AtnExpIndSensor;
  QAction*    m_AtnShowBottom;
  QAction*    m_AtnShowTools;
  QAction*    m_AtnZoomFit;
  QAction*    m_AtnZoom100;
  QAction*    m_AtnModeRGB;
  QAction*    m_AtnModeL;
  QAction*    m_AtnModeA;
  QAction*    m_AtnModeB;
  QAction*    m_AtnModeGradient;
  QAction*    m_AtnModeStructure;
  QActionGroup*   m_ModeGroup;

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
