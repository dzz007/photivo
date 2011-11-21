/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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
#include "ptImageView.h"
#include "ptFileMgrWindow.h"
#include "ptGraphicsSceneEmitter.h"

//==============================================================================

extern ptSettings*  Settings;
extern ptTheme*     Theme;

//==============================================================================

void MyWorker::run() {
  (m_ImageView->*m_Fct)();
}

//==============================================================================

ptImageView::ptImageView(QWidget *parent, ptFileMgrDM* DataModule) :
  QGraphicsView(parent),
  // constants
  MinZoom(0.05),
  MaxZoom(4.0)
{
  assert(parent     != NULL);
  assert(DataModule != NULL);
  assert(Theme      != NULL);
  assert(Settings   != NULL);

  m_DataModule = DataModule;

  ZoomFactors << MinZoom << 0.08 << 0.10 << 0.15 << 0.20 << 0.25 << 0.33 << 0.50 << 0.66 << 1.00
              << 1.50 << 2.00 << 3.00 << MaxZoom;

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // Layout to always fill the complete image pane with ViewWindow
  m_parentLayout = new QGridLayout(parent);
  m_parentLayout->setContentsMargins(9,9,9,9);
  m_parentLayout->setSpacing(0);
  m_parentLayout->addWidget(this);
  this->setStyleSheet("QGraphicsView { border: none; }");

  // We create a Graphicsscene and connect it.
  m_Scene      = new QGraphicsScene(0, 0, 0, 0, this);
  setScene(m_Scene);

  // Init
  m_PixmapItem        = NULL;
  m_Image             = NULL;
  m_FileName          = "";
  m_ZoomMode          = ptZoomMode_Fit;
  m_ZoomFactor        = 1.0;
  m_Zoom              = 100;
  m_DragDelta         = new QLine();
  m_LeftMousePressed  = false;
  m_NeedRun           = false;
  m_ZoomSizeOverlay   = new ptReportOverlay(this, "", QColor(75,150,255), QColor(190,220,255),
                                            1000, Qt::AlignRight, 20);
  m_StatusOverlay     = new ptReportOverlay(this, "", QColor(), QColor(), 0, Qt::AlignLeft, 20);

  // parallel worker
  m_Worker = new MyWorker();
  m_Worker->m_ImageView = this;
  m_Worker->m_Fct       = &ptImageView::updateView;
  m_Worker->m_FileName  = "";
  connect(m_Worker, SIGNAL(finished()), this, SLOT(afterWorker()));

  // timer for decoupling the mouse wheel
  m_ResizeTimeOut = 300;
  m_ResizeTimer   = new QTimer(this);
  m_ResizeTimer->setSingleShot(1);
  connect(m_ResizeTimer,SIGNAL(timeout()), this,SLOT(ResizeTimerExpired()));
}

//==============================================================================

ptImageView::~ptImageView() {
  m_Worker->terminate();
  DelAndNull(m_Worker);
  DelAndNull(m_DragDelta);
  DelAndNull(m_ZoomSizeOverlay);
  DelAndNull(m_StatusOverlay);
  DelAndNull(m_Scene);
  DelAndNull(m_parentLayout);
}

//==============================================================================

void ptImageView::ShowImage(const QString FileName) {
  if (m_FileName == FileName) return;
  m_FileName = FileName;

  if (this->isVisible()) {
    // only process image when the ImageView is visible
    m_NeedRun  = true;
    startWorker();
  }
}

//==============================================================================

void ptImageView::resizeEvent(QResizeEvent* event) {
  if (m_ZoomMode == ptZoomMode_Fit) {
    event->accept();
    ZoomToFit(0);
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
  } else if (event->button() == Qt::RightButton) {
    event->accept();
    ZoomToFit(1);
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
  ptGraphicsSceneEmitter::EmitThumbnailAction(tnaLoadImage, m_FileName);
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
  int ZoomIdx = -1;

  // zoom larger
  if (event->delta() > 0) {
    for (int i = 0; i < ZoomFactors.size(); i++) {
      if (ZoomFactors[i] > m_ZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }

  // zoom smaller
  } else if (event->delta() < 0) {
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

void ptImageView::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();
  // todo
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

int ptImageView::ZoomToFit(const short withMsg /*= 1*/) {
  m_ZoomMode = ptZoomMode_Fit;

  if (m_Image != NULL) {
    m_Scene->setSceneRect(0, 0, m_Image->width(), m_Image->height());
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
  if (m_Image != NULL) {
    resetTransform();

    if (m_PixmapItem != NULL) {
      m_Scene->removeItem(m_PixmapItem);
      DelAndNull(m_PixmapItem);
    }

    Qt::TransformationMode Mode;

    if(((uint)(Factor * 10000) % 10000) < 1) {
      // nearest neighbour resize for 200%, 300%, 400% zoom
      Mode = Qt::FastTransformation;
    } else {
      // bilinear resize for all others
      Mode = Qt::SmoothTransformation;
    }
    m_Scene->setSceneRect(0, 0, m_Image->width()*Factor, m_Image->height()*Factor);
    m_PixmapItem = m_Scene->addPixmap(
                     QPixmap::fromImage(*m_Image).scaled(m_Image->width()*Factor,
                                                         m_Image->height()*Factor,
                                                         Qt::IgnoreAspectRatio,
                                                         Mode));
    m_PixmapItem->setTransformationMode(Mode);
  }
}

//==============================================================================

void ptImageView::updateView() {
  QImage* Image = m_DataModule->getThumbnail(m_FileName, 0);
  if (Image != NULL) {
    DelAndNull(m_Image);
    if (m_FileName == m_Worker->m_FileName) {
      m_Image = Image;
    } else {
      DelAndNull(Image);
    }
  }
}

//==============================================================================

void ptImageView::startWorker() {
  if (m_Worker->isRunning()) {
    if (m_Worker->m_FileName == m_FileName) m_NeedRun = false;
  } else {
    m_StatusOverlay->setColors(QColor(75,150,255), QColor(190,220,255)); // blue
    m_StatusOverlay->setDuration(0);
    m_StatusOverlay->exec(QObject::tr("Loading"));
    m_NeedRun = false;
    m_Worker->m_FileName = m_FileName;
    m_Worker->start();
    if (!m_Worker->isRunning())
      ptMessageBox::critical(NULL, "Thread error", "Could not start thread for parallel image loading.");
  }
}

//==============================================================================

void ptImageView::afterWorker() {
  if (m_NeedRun) {
    startWorker();
  } else {
    m_StatusOverlay->stop();
    if (m_Image != NULL) {
      if (m_ZoomMode == ptZoomMode_Fit)
        ZoomToFit(0);
      else
        ZoomTo(m_ZoomFactor, true);
    }
  }
}

//==============================================================================

void ptImageView::ResizeTimerExpired() {
  ZoomTo(m_ZoomFactor, false);
}

//==============================================================================

void ptImageView::showEvent(QShowEvent* event) {
  QGraphicsView::showEvent(event);

  if (!m_FileName.isEmpty()) {
    // Re-process on becoming visible. Filename might have changed while we were hidden.
    m_NeedRun  = true;
    startWorker();
  }
}

//==============================================================================
