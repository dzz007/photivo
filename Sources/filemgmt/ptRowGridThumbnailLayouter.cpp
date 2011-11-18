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

#include <qmath.h>
#include <QScrollBar>

#include "ptRowGridThumbnailLayouter.h"

//==============================================================================

ptRowGridThumbnailLayouter::ptRowGridThumbnailLayouter(QGraphicsView* view)
: ptGridThumbnailLayouter(view)
{}

//==============================================================================

void ptRowGridThumbnailLayouter::Init(const int thumbCount, const QFont& font) {
  ptGridThumbnailLayouter::Init(thumbCount, font);

  // Add another padding to height to make the layout in rows visually clear.
  m_ThumbMetrics.CellHeight += m_ThumbMetrics.Padding;

  // max number of thumbs in a row
  // +Padding because we only take care of padding *between* thumbnails here.
  // -1 because rows are 0 indexed
  // This calculation does not yet take scrollbar width/height into account
  int RestrictedMax = Settings->GetInt("FileMgrUseThumbMaxRowCol") == 0 ?
                      INT_MAX-1 : Settings->GetInt("FileMgrThumbMaxRowCol")-1;
  m_ThumbMetrics.MaxCol =
      qMin(RestrictedMax,
           (m_View->width() + m_ThumbMetrics.Padding) / m_ThumbMetrics.CellWidth - 1);

  // Calc complete height to set the scene’s rect appropriately
  int FullHeight = qCeil((qreal)thumbCount / (qreal)(m_ThumbMetrics.MaxCol + 1)) *
                   m_ThumbMetrics.CellHeight;

  if (FullHeight >= m_View->height()) {
    // we have enough thumbs to produce a vertical scrollbar
    m_View->verticalScrollBar()->setSingleStep(m_ThumbMetrics.CellHeight);

    if((m_View->width() - (m_ThumbMetrics.MaxCol + 1)*m_ThumbMetrics.CellWidth) <
       m_View->verticalScrollBar()->width())
    { // empty space on the right is not wide enough for scrollbar
      m_ThumbMetrics.MaxCol--;
      FullHeight = qCeil((qreal)thumbCount / (qreal)(m_ThumbMetrics.MaxCol + 1)) *
                   m_ThumbMetrics.CellHeight;
    }
  }

  m_View->scene()->setSceneRect(
      0, 0,
      m_ThumbMetrics.CellWidth*(m_ThumbMetrics.MaxCol + 1) - m_ThumbMetrics.Padding,
      FullHeight);
}

//==============================================================================

void ptRowGridThumbnailLayouter::Layout(ptGraphicsThumbGroup* thumb) {
  if (m_LazyInit) {
    m_LazyInit = false;
    Init(m_ThumbCount, thumb->font());
  }

  thumb->setPos(m_ThumbMetrics.Col * m_ThumbMetrics.CellWidth,
                m_ThumbMetrics.Row * m_ThumbMetrics.CellHeight);

  if (m_ThumbMetrics.Col >= m_ThumbMetrics.MaxCol) {
    m_ThumbMetrics.Col = 0;
    m_ThumbMetrics.Row++;
  } else {
    m_ThumbMetrics.Col++;
  }
}

//==============================================================================

int ptRowGridThumbnailLayouter::MoveIndex(const int currentIdx, QKeyEvent* event) {
  // See .h for full documentation of behaviour
  int idx = qMax(0, currentIdx);
  int offset = idx % (m_ThumbMetrics.MaxCol+1);
  if (event->modifiers() == Qt::NoModifier) {
    switch (event->key()) {
      case Qt::Key_Left:
        return qMax(0, idx-1);
      case Qt::Key_Right:
        return qMin(m_ThumbCount-1, idx+1);
      case Qt::Key_Up:
        if (idx > m_ThumbMetrics.MaxCol)
          idx = idx - m_ThumbMetrics.MaxCol - 1;
        return idx;
      case Qt::Key_Down:
        if (idx < m_ThumbCount-1 - m_ThumbMetrics.MaxCol)
          idx = qMin(m_ThumbCount-1, idx + m_ThumbMetrics.MaxCol + 1);
        return idx;
      case Qt::Key_Home:  // start of line
        return qMax(0, idx - offset);
      case Qt::Key_End:   // end of line
        return qMin(m_ThumbCount-1, idx + m_ThumbMetrics.MaxCol - offset);
      case Qt::Key_PageUp:
        return
            qMax(0 + offset,
                 idx - (int)(m_View->height()/m_ThumbMetrics.CellHeight)*(m_ThumbMetrics.MaxCol+1));
      case Qt::Key_PageDown:
        return
            qMin(m_ThumbCount-1,
                 idx + (int)(m_View->height()/m_ThumbMetrics.CellHeight)*(m_ThumbMetrics.MaxCol+1));
      default:    // unrecognised key
        return -1;
    }

  } else if (event->modifiers() == Qt::ControlModifier) {
    switch (event->key()) {
      case Qt::Key_Home: return 0;                // first thumbnail in list
      case Qt::Key_End:  return m_ThumbCount-1;   // last thumbnail in list
      default:           return -1;               // unrecognised key
    }

  } else {
    return -1;    // unrecognised key
  }
}

//==============================================================================
