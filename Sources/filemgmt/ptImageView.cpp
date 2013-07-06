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

#include "ptImageView.h"
#include "ptGraphicsSceneEmitter.h"
#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "../ptMessageBox.h"
#include <QAction>
#include <QGraphicsPixmapItem>
#include <QMenu>
#include <QMouseEvent>
#include <QList>
#include <QScrollBar>
#include <limits>
#include <cassert>

extern ptSettings*  Settings;
extern ptTheme*     Theme;

//------------------------------------------------------------------------------
/*! Creates a \c ptImageView instance.
  \param parent
    The image view’s parent widget.
*/
ptImageView::ptImageView(QWidget *AParent):
  QGraphicsView(AParent),
  MinZoom(0.05),
  MaxZoom(4.0),
  MaxImageSize(std::numeric_limits<int>::max()),
  // Cache is split 60/40 between thumbnail list and image view
  FThumbGen(Settings->GetInt("FileMgrThumbCacheSize")*1024*1024 * 0.4, 1)
{
  Q_ASSERT_X(Theme != nullptr, __PRETTY_FUNCTION__, "ptTheme pointer is null.");
  Q_ASSERT_X(Settings != nullptr, __PRETTY_FUNCTION__, "ptSettings pointer is null.");

  ZoomFactors << MinZoom << 0.08 << 0.10 << 0.15 << 0.20 << 0.25 << 0.33 << 0.50 << 0.66 << 1.00
              << 1.50 << 2.00 << 3.00 << MaxZoom;

  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setFocusPolicy(Qt::NoFocus);

  FThumbGen.connectBroadcast(this, SLOT(receiveImage(uint,TThumbPtr)));

  // Layout to always fill the complete image pane with ViewWindow
  FParentLayout = new QGridLayout(AParent);
  FParentLayout->setContentsMargins(9,9,9,9);
  FParentLayout->setSpacing(0);
  FParentLayout->addWidget(this);
  this->setStyleSheet("QGraphicsView { border: none; }");

  // We create a Graphicsscene and connect it.
  FScene = new QGraphicsScene(0, 0, 0, 0, this);
  this->setScene(FScene);

  // Init
  FPixmapItem        = FScene->addPixmap(QPixmap());
  FPixmapItem->setPos(0,0);
  FZoomMode          = ptZoomMode_Fit;
  FZoomFactor        = 1.0;
  FZoom              = 100;
  FDragDelta         = new QLine();
  FLeftMousePressed  = false;
  FZoomSizeOverlay   = new ptReportOverlay(this, "", QColor(75,150,255), QColor(190,220,255),
                                            1000, Qt::AlignRight, 20);
  FStatusOverlay     = new ptReportOverlay(this, "", QColor(), QColor(), 0, Qt::AlignLeft, 20);
  FStatusOverlay->setColors(QColor(75,150,255), QColor(190,220,255)); // blue
  FStatusOverlay->setDuration(0);

  // timer for decoupling the mouse wheel
  FResizeTimeOut = 50;
  FResizeTimer   = new QTimer(this);
  FResizeTimer->setSingleShot(1);
  connect(FResizeTimer,SIGNAL(timeout()), this,SLOT(resizeTimerExpired()));

  FResizeEventTimer.setSingleShot(true);
  FResizeEventTimer.setInterval(100);
  connect(&FResizeEventTimer, SIGNAL(timeout()), this, SLOT(zoomFit()));

  //-------------------------------------

  // Create actions for context menu
  FZoomInAction = new QAction(tr("Zoom &in") + "\t" + tr("1"), this);
  FZoomInAction->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-in.png")));
  connect(FZoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));

  FZoom100Action = new QAction(tr("Zoom &100%") + "\t" + tr("2"), this);
  FZoom100Action->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-original.png")));
  connect(FZoom100Action, SIGNAL(triggered()), this, SLOT(zoom100()));

  FZoomOutAction = new QAction(tr("Zoom &out") + "\t" + tr("3"), this);
  FZoomOutAction->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-out.png")));
  connect(FZoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));

  FZoomFitAction = new QAction(tr("Zoom &fit") + "\t" + tr("4"), this);
  FZoomFitAction->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-fit.png")));
  connect(FZoomFitAction, SIGNAL(triggered()), this, SLOT(zoomFit()));
}

//------------------------------------------------------------------------------
ptImageView::~ptImageView() {
  DelAndNull(FDragDelta);
  DelAndNull(FZoomSizeOverlay);
  DelAndNull(FStatusOverlay);
  DelAndNull(FParentLayout);
}

//------------------------------------------------------------------------------
/*! Stops image generation and clears image cache. */
void ptImageView::clear() {
  FThumbGen.clear();
  FImage.reset();
  FPixmapItem->setPixmap(QPixmap());
}

//------------------------------------------------------------------------------
/*!
  Load and show the image specified by AFilename. If AForceReload is true the
  image is loaded again even if AFilename and the currently loaded image are the same.
*/
void ptImageView::showImage(const QString& AFilename, bool AForceReload) {
  if ((AFilename != FFilenameCurrent) || AForceReload) {
    FThumbGen.abort();
    FStatusOverlay->exec(QObject::tr("Loading"));
    FFilenameNext = AFilename;
    FThumbGen.request({0, makeThumbId(AFilename, MaxImageSize, fsoFile)});
  }
}

//------------------------------------------------------------------------------
void ptImageView::receiveImage(uint, TThumbPtr AImage8) {
  FFilenameCurrent = FFilenameNext;
  FFilenameNext.clear();

  FImage = AImage8;

  if (FZoomMode == ptZoomMode_Fit)
    zoomFit(false);
  else
    zoomTo(FZoomFactor, true);

  FStatusOverlay->stop();
}


//------------------------------------------------------------------------------
void ptImageView::resizeEvent(QResizeEvent* event) {
  if (FImage) {
    if (FZoomMode == ptZoomMode_Fit) {
      // Only zoom fit after timer expires to avoid constant image resizing while
      // changing widget geometry. Prevents jerky UI response at the cost of
      // the image not being resized while the geometry change is ongoing.
      FResizeEventTimer.start();
    } else {
      // takes care of positioning the scene inside the viewport on non-fit zooms
      QGraphicsView::resizeEvent(event);
    }
  }
}

//------------------------------------------------------------------------------
void ptImageView::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    event->accept();
    FLeftMousePressed = true;
    FDragDelta->setPoints(event->pos(), event->pos());
  }
}

//------------------------------------------------------------------------------
void ptImageView::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && FLeftMousePressed) {
    FLeftMousePressed = false;
  }
  event->accept();
}

//------------------------------------------------------------------------------
void ptImageView::mouseDoubleClickEvent(QMouseEvent* event) {
  if (FImage) {
    event->accept();
    ptGraphicsSceneEmitter::EmitThumbnailAction(tnaLoadImage, FFilenameCurrent);
  } else {
    event->ignore();
  }
}

//------------------------------------------------------------------------------
void ptImageView::mouseMoveEvent(QMouseEvent* event) {
  // drag image with left mouse button to scroll
  if (FLeftMousePressed) {
    FDragDelta->setP2(event->pos());
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                    FDragDelta->x2() +
                                    FDragDelta->x1());
    verticalScrollBar()->setValue(verticalScrollBar()->value() -
                                  FDragDelta->y2() +
                                  FDragDelta->y1());
    FDragDelta->setP1(event->pos());
    event->accept();

  } else {
    event->ignore();
  }
}

//------------------------------------------------------------------------------
void ptImageView::wheelEvent(QWheelEvent* event) {
  zoomStep(event->delta());
}

//------------------------------------------------------------------------------
void ptImageView::contextMenuEvent(QContextMenuEvent* event) {
  QMenu Menu(this);
  Menu.setPalette(Theme->menuPalette());
  Menu.setStyle(Theme->style());
  Menu.addAction(FZoomInAction);
  Menu.addAction(FZoom100Action);
  Menu.addAction(FZoomOutAction);
  Menu.addAction(FZoomFitAction);
  Menu.exec(((QMouseEvent*)event)->globalPos());
}

//------------------------------------------------------------------------------
void ptImageView::zoomStep(int ADirection) {
  int ZoomIdx = -1;

  // zoom larger
  if (ADirection > 0) {
    for (int i = 0; i < ZoomFactors.size(); i++) {
      if (ZoomFactors[i] > FZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }

  // zoom smaller
  } else if (ADirection < 0) {
    for (int i = ZoomFactors.size() - 1; i >= 0; i--) {
      if (ZoomFactors[i] < FZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }
  }

  if (ZoomIdx != -1) {
    FZoomFactor = ZoomFactors[ZoomIdx];
    FZoom = qRound(FZoomFactor * 100);
    FResizeTimer->start(FResizeTimeOut);
    FZoomSizeOverlay->exec(QString::number(FZoom) + "%");
  }
}

//------------------------------------------------------------------------------
void ptImageView::zoomIn() {
  zoomStep(1);
}

//------------------------------------------------------------------------------
void ptImageView::zoomOut() {
  zoomStep(-1);
}

//------------------------------------------------------------------------------
void ptImageView::zoom100() {
  zoomTo(1.0, true);
}

//------------------------------------------------------------------------------
// ZoomTo() is also called by wheelEvent() for mouse wheel zoom.
void ptImageView::zoomTo(float AFactor, bool AWithMsg) {
  FZoomMode = ptZoomMode_NonFit;
  AFactor = qBound(MinZoom, AFactor, MaxZoom);

  imageToScene(AFactor);

  FZoomFactor = AFactor;
  FZoom = qRound(FZoomFactor * 100);
  if (AWithMsg)
    FZoomSizeOverlay->exec(QString::number(FZoom) + "%");
}

//------------------------------------------------------------------------------
int ptImageView::zoomFit(bool AWithMsg /*= true*/) {
  FZoomMode = ptZoomMode_Fit;
  FScene->setSceneRect(0, 0, FImage->width(), FImage->height());

  fitInView(FScene->sceneRect(), Qt::KeepAspectRatio);
  FZoomFactor = transform().m11();

  // we will reset the transform in the next step!
  imageToScene(FZoomFactor);

  if (AWithMsg) {
    FZoomSizeOverlay->exec(tr("Fit"));
  }

  FZoom = qRound(FZoomFactor * 100);
  return FZoomFactor;
}

//------------------------------------------------------------------------------
void ptImageView::imageToScene(double AFactor) {
  resetTransform();

  Qt::TransformationMode Mode;

  if(((uint)(AFactor * 10000) % 10000) < 1) {
    // nearest neighbour resize for 200%, 300%, 400% zoom
    Mode = Qt::FastTransformation;
  } else {
    // bilinear resize for all others
    Mode = Qt::SmoothTransformation;
  }
  FScene->setSceneRect(0, 0, FImage->width()*AFactor, FImage->height()*AFactor);
  FPixmapItem->setPixmap(QPixmap::fromImage(QImage((const uchar*) FImage->image().data(),
                                                                  FImage->width(),
                                                                  FImage->height(),
                                                                  QImage::Format_RGB32).scaled(FImage->width()*AFactor,
                                                                                               FImage->height()*AFactor,
                                                                                               Qt::IgnoreAspectRatio,
                                                                                               Mode)));
  FPixmapItem->setTransformationMode(Mode);
}

//------------------------------------------------------------------------------
void ptImageView::resizeTimerExpired() {
  zoomTo(FZoomFactor, false);
}

//------------------------------------------------------------------------------
void ptImageView::showEvent(QShowEvent* event) {
  QGraphicsView::showEvent(event);
  if (!FImage && !FFilenameCurrent.isEmpty()) {
    this->showImage(FFilenameCurrent, true);
  }
}

//------------------------------------------------------------------------------
QString ptImageView::currentFilename() const {
  return FFilenameCurrent;
}
