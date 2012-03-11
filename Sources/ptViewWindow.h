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
/*!
  \class ptViewWindow

  \brief Displays the preview image and manages all interactions that happen directly
    on the image itself, e.g. zoom, crop, spot repair.

  Usage notes:

  The global instances of ptMainWindow and ptTheme must be created BEFORE
  creating the global ptViewWindow instance.

  Consider ptViewWindow to be a singleton. DO NOT create additional instances.

  Current horizontal scale factor: this->transform().m11();
  Current vertical scale factor: this->transform().m22();
  Because we do not change aspect ratio both factors are always the same.
  Use FZoomFactor whenever possible and m11() otherwise.
**/

#ifndef PTVIEWWINDOW_H
#define PTVIEWWINDOW_H

//==============================================================================

#include <QLine>
#include <QMenu>
#include <QGraphicsView>

#include "ptImage.h"
#include "ptMainWindow.h"
#include "ptReportOverlay.h"
#include "ptLineInteraction.h"
#include "ptSimpleRectInteraction.h"
#include "ptRichRectInteraction.h"
#include "ptGridInteraction.h"
#include "imagespot/ptSpotInteraction.h"
#include "imagespot/ptRepairInteraction.h"
#include "imagespot/ptImageSpotListView.h"

//==============================================================================

enum ptInteraction {
  iaNone        = 0,
  iaCrop        = 1,
  iaSelectRect  = 2,  // simple rectangle selection: e.g. for spot WB
  iaDrawLine    = 3,  // draw a single straight line: e.g. for rotate angle
  iaSpotRepair  = 4,
  iaLocalAdjust = 5
};

enum ptPixelReading {
  prNone    = 0,
  prLinear  = 1,
  prPreview = 2
};

//==============================================================================

class ptViewWindow : public QGraphicsView {
Q_OBJECT

public:
  ptViewWindow(QWidget* Parent, ptMainWindow* mainWin);
  ~ptViewWindow();

  inline ptInteraction interaction() const { return FInteraction; }
  inline int zoomPercent() { return qRound(FZoomFactor * 100); }
  inline float zoomFactor() const { return FZoomFactor; }

  // Save (and later restore) current zoom settings. Takes care of
  // everything incl. ptSettings. RestoreZoom() also updates the
  // viewport accordingly.
  void SaveZoom();
  void RestoreZoom();

  // Show status overlay in the top left viewport corner.
  // For mode use ptStatus_ constants.
  void ShowStatus(short mode);
  void ShowStatus(const QString text);    // shown for 1.5sec

  // Start, stop, control interactions
  // - StartSimpleRect: Pass the function that is called after the
  //   selection finishes.
  void StartLine();
  void StartSimpleRect(void (*CB_SimpleRect)(const ptStatus, QRect));
  void StartCrop();
  void StartLocalAdjust(ptImageSpotListView* AListView);
  void StartSpotRepair(ptImageSpotListView* AListView);

  ptRichRectInteraction *crop() const { return FCrop; }
  ptSpotInteraction     *localAdjust() const { return FLocalAdjust; }
  ptRepairInteraction   *spotRepair() const { return FSpotRepair; }

  void setGrid(const short enabled, const uint linesX, const uint linesY);
  void UpdateImage(const ptImage* relatedImage);
  void ZoomTo(float factor);  // 1.0 means 100%
  int  ZoomToFit(const short withMsg = 1);  // fit complete image into viewport
  void ZoomStep(int direction);

  ptPixelReading isPixelReading() const { return FPixelReading; }
  void SetPixelReader(void (*PixelReader)(const QPointF Point, const ptPixelReading PixelReading))
    { FPixelReader = PixelReader; }

//------------------------------------------------------------------------------

protected:
  void contextMenuEvent(QContextMenuEvent* event);
  void dragEnterEvent(QDragEnterEvent* event);
  void dropEvent(QDropEvent* event);
  void keyPressEvent(QKeyEvent* event);
  void keyReleaseEvent(QKeyEvent* event);
  void paintEvent(QPaintEvent* event);
  void resizeEvent(QResizeEvent* event);
  void mouseDoubleClickEvent(QMouseEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void leaveEvent(QEvent* event);

//------------------------------------------------------------------------------

private:
  const float   MinZoom;
  const float   MaxZoom;
  QList<float>  ZoomFactors;   // steps for wheel zoom

  short                     FCtrlIsPressed;
  ptLineInteraction         *FDrawLine;
  ptSpotInteraction         *FLocalAdjust;
  ptSimpleRectInteraction   *FSelectRect;
  ptRichRectInteraction     *FCrop;
  ptGridInteraction         *FGrid;
  ptRepairInteraction       *FSpotRepair;
  ptInteraction             FInteraction;
  short                     FLeftMousePressed;
  void (*FCB_SimpleRect)(const ptStatus, QRect);
  short                     FZoomIsSaved;
  float                     FZoomFactor;
  float                     FZoomFactorSav;
  short                     FZoomModeSav;
  ptPixelReading            FPixelReading;
  QTimer                    *FPReadTimer;

  QGraphicsPixmapItem       *F8bitImageItem;
  QLine                     *FDragDelta;
  QGraphicsScene            *FImageScene;
  ptReportOverlay           *FStatusOverlay;
  ptReportOverlay           *FZoomSizeOverlay;

  // context menu stuff
  void          ConstructContextMenu();
  QAction       *ac_ZoomFit;
  QAction       *ac_Zoom100;
  QAction       *ac_ZoomIn;
  QAction       *ac_ZoomOut;
  QAction       *ac_Mode_RGB;
  QAction       *ac_Mode_Structure;
  QAction       *ac_Mode_L;
  QAction       *ac_Mode_A;
  QAction       *ac_Mode_B;
  QAction       *ac_Mode_Gradient;
  QAction       *ac_PRead_None;
  QAction       *ac_PRead_Linear;
  QAction       *ac_PRead_Preview;
  QAction       *ac_Clip_Indicate;
  QAction       *ac_Clip_Over;
  QAction       *ac_Clip_Under;
  QAction       *ac_Clip_R;
  QAction       *ac_Clip_G;
  QAction       *ac_Clip_B;
  QAction       *ac_SensorClip;
  QAction       *ac_SensorClipSep;
  QAction       *ac_ShowTools;
  QAction       *ac_ShowZoomBar;
  QAction       *ac_OpenFileMgr;
  QAction       *ac_Fullscreen;
  QActionGroup  *ac_ModeGroup;
  QActionGroup  *ac_PReadGroup;

  ptMainWindow  *FMainWindow;

  void (*FPixelReader)(const QPointF Point, const ptPixelReading PixelReading);

//------------------------------------------------------------------------------

private slots:
  void finishInteraction(ptStatus ExitStatus);

  // context menu stuff
  void Menu_Clip_Indicate();
  void Menu_Clip_Over();
  void Menu_Clip_Under();
  void Menu_Clip_R();
  void Menu_Clip_G();
  void Menu_Clip_B();
  void Menu_SensorClip();
  void Menu_ShowZoomBar();
  void Menu_ShowTools();
  void Menu_Fullscreen();
  void Menu_ZoomIn();
  void Menu_ZoomFit();
  void Menu_Zoom100();
  void Menu_ZoomOut();
  void Menu_Mode();
  void Menu_OpenFileMgr();
  void Menu_PixelReading();

//------------------------------------------------------------------------------

signals:
  void keyChanged(QKeyEvent* event);
  void mouseChanged(QMouseEvent* event);
  void openFileMgr();


};
#endif // PTVIEWWINDOW_H
