/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#include <cassert>

#include <QGraphicsView>
#include <QList>

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "../ptMessageBox.h"
#include "../ptImage8.h"
#include "ptImageView.h"
#include "ptFileMgrWindow.h"

//==============================================================================

extern ptSettings*  Settings;
extern ptTheme*     Theme;

//==============================================================================

ptImageView::ptImageView(QWidget            *AParent,
                         ptFileMgrDM        *ADataModule,
                         ptThumbGroupEvents *AEventHandler) :
  QGraphicsView(AParent),
  // constants
  m_DataModule(ADataModule),
  FEventHandler(AEventHandler),
  MinZoom(0.05),
  MaxZoom(4.0),
  m_ResizeEventTimer(),
  FNextImage(),
  FCurrentImage(),
  FImage(),
  FHaveImage(false)
{
  assert(AParent     != NULL);
  assert(ADataModule != NULL);
  assert(Theme      != NULL);
  assert(Settings   != NULL);

  ZoomFactors << MinZoom << 0.08 << 0.10 << 0.15 << 0.20 << 0.25 << 0.33 << 0.50 << 0.66 << 1.00
              << 1.50 << 2.00 << 3.00 << MaxZoom;

  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setFocusPolicy(Qt::NoFocus);

  // Layout to always fill the complete image pane with ViewWindow
  m_parentLayout = new QGridLayout(AParent);
  m_parentLayout->setContentsMargins(9,9,9,9);
  m_parentLayout->setSpacing(0);
  m_parentLayout->addWidget(this);
  this->setStyleSheet("QGraphicsView { border: none; }");

  // We create a Graphicsscene and connect it.
  m_Scene      = new QGraphicsScene(0, 0, 0, 0, this);
  setScene(m_Scene);

  // Init
  m_PixmapItem        = m_Scene->addPixmap(QPixmap());
  m_PixmapItem->setPos(0,0);
  m_ZoomMode          = ptZoomMode_Fit;
  m_ZoomFactor        = 1.0;
  m_Zoom              = 100;
  m_DragDelta         = new QLine();
  m_LeftMousePressed  = false;
  m_ZoomSizeOverlay   = new ptReportOverlay(this, "", QColor(75,150,255), QColor(190,220,255),
                                            1000, Qt::AlignRight, 20);
  m_StatusOverlay     = new ptReportOverlay(this, "", QColor(), QColor(), 0, Qt::AlignLeft, 20);
  m_StatusOverlay->setColors(QColor(75,150,255), QColor(190,220,255)); // blue
  m_StatusOverlay->setDuration(0);

  // timer for decoupling the mouse wheel
  m_ResizeTimeOut = 50;
  m_ResizeTimer   = new QTimer(this);
  m_ResizeTimer->setSingleShot(1);
  connect(m_ResizeTimer,SIGNAL(timeout()), this,SLOT(ResizeTimerExpired()));

  m_ResizeEventTimer.setSingleShot(true);
  m_ResizeEventTimer.setInterval(100);
  connect(&m_ResizeEventTimer, SIGNAL(timeout()), this, SLOT(zoomFit()));

  //-------------------------------------

  // Create actions for context menu
  ac_ZoomIn = new QAction(tr("Zoom &in") + "\t" + tr("1"), this);
  ac_ZoomIn->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-in.png")));
  connect(ac_ZoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));

  ac_Zoom100 = new QAction(tr("Zoom &100%") + "\t" + tr("2"), this);
  ac_Zoom100->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-original.png")));
  connect(ac_Zoom100, SIGNAL(triggered()), this, SLOT(zoom100()));

  ac_ZoomOut = new QAction(tr("Zoom &out") + "\t" + tr("3"), this);
  ac_ZoomOut->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-out.png")));
  connect(ac_ZoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));

  ac_ZoomFit = new QAction(tr("Zoom &fit") + "\t" + tr("4"), this);
  ac_ZoomFit->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-fit.png")));
  connect(ac_ZoomFit, SIGNAL(triggered()), this, SLOT(zoomFit()));
}

//==============================================================================

ptImageView::~ptImageView() {
  DelAndNull(m_DragDelta);
  DelAndNull(m_ZoomSizeOverlay);
  DelAndNull(m_StatusOverlay);
  DelAndNull(m_Scene);
  DelAndNull(m_parentLayout);
}

//==============================================================================

void ptImageView::ShowImage(const QString AFileName) {
  if (!isVisible()) {
    return;
  }

  if (FNextImage.FileName == AFileName) {
    m_StatusOverlay->stop();
    return;
  }

  // We don't need that image any longer
  ptFileMgrDM::GetInstance()->getThumbDM()->cancelThumb(FNextImage);
  FNextImage.FileName = AFileName;

  m_StatusOverlay->exec(QObject::tr("Loading"));

  // We order the new image.
  ptFileMgrDM::GetInstance()->getThumbDM()->orderThumb(FNextImage, false);
}

//==============================================================================

void ptImageView::resizeEvent(QResizeEvent* event) {
  if (m_ZoomMode == ptZoomMode_Fit) {
    // Only zoom fit after timer expires to avoid constant image resizing while
    // changing widget geometry. Prevents jerky UI response at the cost of
    // the image not being resized while the geometry change is ongoing.
    m_ResizeEventTimer.start();
  } else {
    // takes care of positioning the scene inside the viewport on non-fit zooms
    QGraphicsView::resizeEvent(event);
  }
}

//==============================================================================

void ptImageView::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    event->accept();
    m_LeftMousePressed = true;
    m_DragDelta->setPoints(event->pos(), event->pos());
  }
}

//==============================================================================

void ptImageView::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && m_LeftMousePressed) {
    m_LeftMousePressed = false;
  }
  event->accept();
}

//==============================================================================

void ptImageView::mouseDoubleClickEvent(QMouseEvent* event) {
  event->accept();
  if (FEventHandler) {
    FEventHandler->thumbnailAction(tnaLoadImage, FCurrentImage.FileName);
  }
}

//==============================================================================

void ptImageView::mouseMoveEvent(QMouseEvent* event) {
  // drag image with left mouse button to scroll
  if (m_LeftMousePressed) {
    m_DragDelta->setP2(event->pos());
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                    m_DragDelta->x2() +
                                    m_DragDelta->x1());
    verticalScrollBar()->setValue(verticalScrollBar()->value() -
                                  m_DragDelta->y2() +
                                  m_DragDelta->y1());
    m_DragDelta->setP1(event->pos());
    event->accept();

  } else {
    event->ignore();
  }
}

//==============================================================================

void ptImageView::wheelEvent(QWheelEvent* event) {
  ZoomStep(event->delta());
}

//==============================================================================

void ptImageView::contextMenuEvent(QContextMenuEvent* event) {
  QMenu Menu(this);
  Menu.setPalette(Theme->menuPalette());
  Menu.setStyle(Theme->style());
  Menu.addAction(ac_ZoomIn);
  Menu.addAction(ac_Zoom100);
  Menu.addAction(ac_ZoomOut);
  Menu.addAction(ac_ZoomFit);
  Menu.exec(((QMouseEvent*)event)->globalPos());
}

//==============================================================================

void ptImageView::ZoomStep(int direction) {
  int ZoomIdx = -1;

  // zoom larger
  if (direction > 0) {
    for (int i = 0; i < ZoomFactors.size(); i++) {
      if (ZoomFactors[i] > m_ZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }

  // zoom smaller
  } else if (direction < 0) {
    for (int i = ZoomFactors.size() - 1; i >= 0; i--) {
      if (ZoomFactors[i] < m_ZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }
  }

  if (ZoomIdx != -1) {
    m_ZoomFactor = ZoomFactors[ZoomIdx];
    m_Zoom = qRound(m_ZoomFactor * 100);
    m_ResizeTimer->start(m_ResizeTimeOut);
    m_ZoomSizeOverlay->exec(QString::number(m_Zoom) + "%");
  }
}

//==============================================================================

void ptImageView::zoomIn() {
  ZoomStep(1);
}

//==============================================================================

void ptImageView::zoomOut() {
  ZoomStep(-1);
}

//==============================================================================

void ptImageView::thumbnail(const ptThumbId AThumbId,
                            ptThumbPtr      AImage)
{
  // Check if it is the image we requested
  if (!AThumbId.isEqual(FNextImage))
    return;

  if (!AImage)
    return;

  FImageData.Set(AImage.get());

  FImage     = QImage((const uchar*) FImageData.m_Image,
                      FImageData.m_Width,
                      FImageData.m_Height,
                      QImage::Format_ARGB32);

  FHaveImage    = !FImage.isNull();
  FCurrentImage = AThumbId;

  m_StatusOverlay->stop();

  if (m_ZoomMode == ptZoomMode_Fit)
    zoomFit(false);
  else
    ZoomTo(m_ZoomFactor, true);
}

//==============================================================================

void ptImageView::zoom100() {
  ZoomTo(1.0, true);
}

//==============================================================================

// ZoomTo() is also called by wheelEvent() for mouse wheel zoom.
void ptImageView::ZoomTo(float factor, const bool withMsg) {
  m_ZoomMode = ptZoomMode_NonFit;
  factor = qBound(MinZoom, factor, MaxZoom);

  ImageToScene(factor);

  m_ZoomFactor = factor;//transform().m11();
  m_Zoom = qRound(m_ZoomFactor * 100);
  if (withMsg)
    m_ZoomSizeOverlay->exec(QString::number(m_Zoom) + "%");
}

//==============================================================================

int ptImageView::zoomFit(const bool withMsg /*= true*/) {
  m_ZoomMode = ptZoomMode_Fit;

  if (FHaveImage) {
    m_Scene->setSceneRect(0, 0, FImage.width(), FImage.height());
  }

  fitInView(m_Scene->sceneRect(), Qt::KeepAspectRatio);
  m_ZoomFactor = transform().m11();

  // we will reset the transform in the next step!
  ImageToScene(m_ZoomFactor);

  if (withMsg) {
    m_ZoomSizeOverlay->exec(tr("Fit"));
  }

  m_Zoom = qRound(m_ZoomFactor * 100);
  return m_ZoomFactor;
}

//==============================================================================

void ptImageView::ImageToScene(const double Factor) {
  if (FHaveImage) {
    resetTransform();

    Qt::TransformationMode Mode;

    if(((uint)(Factor * 10000) % 10000) < 1) {
      // nearest neighbour resize for 200%, 300%, 400% zoom
      Mode = Qt::FastTransformation;
    } else {
      // bilinear resize for all others
      Mode = Qt::SmoothTransformation;
    }
    m_Scene->setSceneRect(0, 0, FImage.width()*Factor, FImage.height()*Factor);
    m_PixmapItem->setPixmap(QPixmap::fromImage(FImage.scaled(FImage.width()*Factor,
                                                             FImage.height()*Factor,
                                                             Qt::IgnoreAspectRatio,
                                                             Mode)));
    m_PixmapItem->setTransformationMode(Mode);
  }
}

//==============================================================================

void ptImageView::ResizeTimerExpired() {
  ZoomTo(m_ZoomFactor, false);
}

//==============================================================================

void ptImageView::showEvent(QShowEvent* event) {
  QGraphicsView::showEvent(event);

  ptFileMgrDM::GetInstance()->getThumbDM()->addThumbReciever(this);

  if (!FNextImage.FileName.isEmpty()) {
    ShowImage(FNextImage.FileName);
  }
}

//==============================================================================

void ptImageView::hideEvent(QHideEvent*)
{
  ptFileMgrDM::GetInstance()->getThumbDM()->removeThumbReciever(this);
}

//==============================================================================
