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

#include "../ptSettings.h"
#include "ptColumnGridThumbnailLayouter.h"

extern ptSettings* Settings;

//==============================================================================

ptColumnGridThumbnailLayouter::ptColumnGridThumbnailLayouter(QGraphicsView* view)
: ptGridThumbnailLayouter(view)
{}

//==============================================================================

void ptColumnGridThumbnailLayouter::Init(const int thumbCount, const QFont& font) {
  ptGridThumbnailLayouter::Init(thumbCount, font);

  // Add another padding to width to make the layout in rows visually clear.
  m_ThumbMetrics.CellWidth += m_ThumbMetrics.Padding;

  // For detailed comments see ptRowGridThumbnailLayouter
  int RestrictedMax = Settings->GetInt("FileMgrUseThumbMaxRowCol") == 0 ?
                      INT_MAX-1 : Settings->GetInt("FileMgrThumbMaxRowCol")-1;
  m_ThumbMetrics.MaxRow =
      qMin(RestrictedMax,
           (m_View->height() + m_ThumbMetrics.Padding) / m_ThumbMetrics.CellHeight - 1);

  int FullWidth = qCeil((qreal)thumbCount / (qreal)(m_ThumbMetrics.MaxRow + 1)) *
                  m_ThumbMetrics.CellWidth;

  if (FullWidth >= m_View->width()) {
    // we have enough thumbs to produce a vertical scrollbar
    m_View->horizontalScrollBar()->setSingleStep(m_ThumbMetrics.CellWidth);

    if((m_View->height() - (m_ThumbMetrics.MaxRow + 1)*m_ThumbMetrics.CellHeight) <
       m_View->horizontalScrollBar()->height())
    { // empty space on the bottom is not tall enough for scrollbar
      m_ThumbMetrics.MaxRow--;
      FullWidth = qCeil((qreal)thumbCount / (qreal)(m_ThumbMetrics.MaxRow + 1)) *
                   m_ThumbMetrics.CellWidth;
    }
  }

  m_View->scene()->setSceneRect(
      0, 0,
      FullWidth,
      m_ThumbMetrics.CellHeight*(m_ThumbMetrics.MaxRow + 1) - m_ThumbMetrics.Padding);
}

//==============================================================================

void ptColumnGridThumbnailLayouter::Layout(ptGraphicsThumbGroup* thumb) {
  if (m_LazyInit) {
    m_LazyInit = false;
    Init(m_ThumbCount, thumb->font());
  }

  thumb->setPos(m_ThumbMetrics.Col * m_ThumbMetrics.CellWidth,
                m_ThumbMetrics.Row * m_ThumbMetrics.CellHeight);

  if (m_ThumbMetrics.Row >= m_ThumbMetrics.MaxRow) {
    m_ThumbMetrics.Row = 0;
    m_ThumbMetrics.Col++;
  } else {
    m_ThumbMetrics.Row++;
  }
}

//==============================================================================

int ptColumnGridThumbnailLayouter::MoveIndex(const int currentIdx, QKeyEvent* event) {
  // See .h for full documentation of behaviour
  int idx = qMax(0, currentIdx);
  int offset = idx % (m_ThumbMetrics.MaxRow+1);
  if (event->modifiers() == Qt::NoModifier) {
    switch (event->key()) {
      case Qt::Key_Left:
        if (idx > m_ThumbMetrics.MaxRow)
          idx = idx - m_ThumbMetrics.MaxRow - 1;
        return idx;
      case Qt::Key_Right:
        if (idx < m_ThumbCount-1 - m_ThumbMetrics.MaxRow)
          idx = qMin(m_ThumbCount-1, idx + m_ThumbMetrics.MaxRow + 1);
        return idx;
      case Qt::Key_Up:
        return qMax(0, idx-1);
      case Qt::Key_Down:
        return qMin(m_ThumbCount-1, idx+1);
      case Qt::Key_Home:  // start of column
        return qMax(0, idx - offset);
      case Qt::Key_End:   // end of column
        return qMin(m_ThumbCount-1, idx + m_ThumbMetrics.MaxRow - offset);
      case Qt::Key_PageUp:
        return
            qMax(0 + offset,
                 idx - (int)(m_View->width()/m_ThumbMetrics.CellWidth)*(m_ThumbMetrics.MaxRow+1));
      case Qt::Key_PageDown:
        return
            qMin(m_ThumbCount-1,
                 idx + (int)(m_View->width()/m_ThumbMetrics.CellWidth)*(m_ThumbMetrics.MaxRow+1));
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
