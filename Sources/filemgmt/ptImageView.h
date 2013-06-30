/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
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

#ifndef PTIMAGEVIEW_H
#define PTIMAGEVIEW_H

#include "ptThumbDefines.h"
#include "../ptReportOverlay.h"
#include "../ptImage8.h"
#include "ptThumbGenMgr.h"
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGridLayout>
#include <memory>

//------------------------------------------------------------------------------
class ptImageView: public QGraphicsView {
Q_OBJECT

public:
  explicit ptImageView(QWidget* AParent = nullptr);
  ~ptImageView();

  void clear();
  void showImage(const QString& AFilename);
  QString currentFilename() const;

public slots:
  void zoom100();
  int  zoomFit(bool AWithMsg = true);  // fit complete image into viewport
  void zoomIn();
  void zoomOut();

protected:
  void contextMenuEvent(QContextMenuEvent* event);
  void mouseDoubleClickEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void resizeEvent(QResizeEvent* event);
  void showEvent(QShowEvent* event);
  void wheelEvent(QWheelEvent* event);

private:
  void imageToScene(double AFactor);    // Put the QImage in the scene
  void zoomStep(int ADirection);
  void zoomTo(float AFactor, bool AWithMsg);  // 1.0 means 100%

  const float           MinZoom;
  const float           MaxZoom;
  const int             MaxImageSize;
  QList<float>          ZoomFactors;   // steps for wheel zoom
  QGridLayout*          FParentLayout;
  QGraphicsScene*       FScene;
  TThumbPtr             FImage;
  QString               FFilenameCurrent;
  QString               FFilenameNext;
  ptThumbGenMgr         FThumbGen;
  int                   FZoomMode;
  float                 FZoomFactor;
  int                   FZoom;
  QLine*                FDragDelta;
  bool                  FLeftMousePressed;
  ptReportOverlay*      FZoomSizeOverlay;
  ptReportOverlay*      FStatusOverlay;
  QGraphicsPixmapItem*  FPixmapItem;
  int                   FResizeTimeOut;
  QTimer*               FResizeTimer;
  QTimer                FResizeEventTimer;  // to avoid jerky UI during widget resize
                                             // in zoom fit mode

  QAction* FZoom100Action;
  QAction* FZoomInAction;
  QAction* FZoomFitAction;
  QAction* FZoomOutAction;

private slots:
  void receiveImage(uint, TThumbPtr AImage8);
  void resizeTimerExpired();
};

#endif // PTIMAGEVIEW_H
