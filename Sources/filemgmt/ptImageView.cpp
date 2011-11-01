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

#include "../ptConstants.h"
#include "../ptDefines.h"
#include "ptImageView.h"

//==============================================================================

ptImageView::ptImageView(QObject* parent)
: QGraphicsView(parent),
  MinZoom(0.05),
  MaxZoom(4.0),
  m_ZoomFactor(1),
  m_ZoomMode(ptZoomMode_Fit)
{
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_ZoomSizeOverlay = new ptReportOverlay(this, "", QColor(75,150,255), QColor(190,220,255),
                                          1000, Qt::AlignRight, 20);
  m_Scene = new QGraphicsScene(this);
  this->setScene(m_Scene);
  m_PixItem = new QGraphicsPixmapItem;
  m_PixItem->setPos(0,0);
  m_Scene->addItem(m_PixItem);
}

//==============================================================================

ptImageView::~ptImageView() {
  DelAndNull(m_Scene);
  // Parent mechanism deletes m_Scene and m_PixItem
}

//==============================================================================

void ptImageView::ZoomTo(float factor) {
  m_ZoomMode = ptZoomMode_NonFit;
  factor = qBound(MinZoom, factor, MaxZoom);

  if(((uint)(factor * 10000) % 10000) < 1) {
    // nearest neighbour resize for 200%, 300%, 400% zoom
    m_PixItem->setTransformationMode(Qt::FastTransformation);
  } else {
    // bilinear resize for all others
    m_PixItem->setTransformationMode(Qt::SmoothTransformation);
  }
  setTransform(QTransform(factor, 0, 0, factor, 0, 0));

  m_ZoomFactor = transform().m11();
  int z = qRound(m_ZoomFactor * 100);
  m_ZoomSizeOverlay->exec(QString::number(z) + "%");
}

//==============================================================================

int ptImageView::ZoomToFit(const bool withMsg = true) {
  m_ZoomMode = ptZoomMode_Fit;

  if (!m_PixItem->pixmap().isNull()) {
    // Always smooth scaling because we don't know the zoom factor in advance.
    m_PixItem->setTransformationMode(Qt::SmoothTransformation);
    fitInView(m_PixItem, Qt::KeepAspectRatio);
    m_ZoomFactor = transform().m11();

    if (withMsg) {
      m_ZoomSizeOverlay->exec(tr("Fit"));
    }
  }

  return m_ZoomFactor;
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
    ZoomTo(ZoomFactors[ZoomIdx]);
  }
}

//==============================================================================

void ptImageView::resizeEvent(QResizeEvent* event) {
  if (m_ZoomMode == ptZoomMode_Fit) {
    // in zoom-to-fit mode adjust zoom, but without overlay message
    event->accept();
    ZoomToFit(false);
  } else {
    // takes care of positioning the scene inside the viewport on non-fit zooms
    QGraphicsView::resizeEvent(event);
  }
}

//==============================================================================

void ptImageView::setImage(QImage* image) {
  if (image != NULL) {
    blockSignals(true);
    m_PixItem->setPixmap(QPixmap::fromImage(*image));
    m_Scene->setSceneRect(0, 0,
                          m_PixItem->pixmap().width(),
                          m_PixItem->pixmap().height());

    if (m_ZoomMode == ptZoomMode_Fit) {
      ZoomToFit(false);
    }
    blockSignals(false);
  }
}

//==============================================================================
