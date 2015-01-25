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

#ifndef PTCOLUMNGRIDTHUMBNAILLAYOUTER_H
#define PTCOLUMNGRIDTHUMBNAILLAYOUTER_H

//==============================================================================

#include "ptGridThumbnailLayouter.h"

#include <QGraphicsView>

//==============================================================================

class ptColumnGridThumbnailLayouter: public ptGridThumbnailLayouter {
public:
  /*! Creates an instance of a column-grid layouter.
    \param view
      A pointer to the \c QGraphcisView that contains the \c QGraphicsScene
      where the thumbnails are displayed.
  */
  explicit ptColumnGridThumbnailLayouter(QGraphicsView* view);

  /*! Reimplemented from ptGridThumbnailLayouter::Init() */
  virtual void Init(const int thumbCount, const QFont& font);

  /*! Reimplemented from \c ptGridThumbnailLayouter::Layout()  */
  virtual void Layout(ptGraphicsThumbGroup* thumb);

  /*! Reimplemented from \c ptGridThumbnailLayouter::MoveIndex()\n
      Recognised keys are listed below. Their behaviour is modelled after the common navigation
      key behaviour in most text processing applications.
      - <b>Cursor up/down:</b> Moves one thumbnail up/down, changing columns if necessary
        and clamping to the beginning and end of the list. I.e. pressing Up (Down) at the
        first (last) thumbnail will not jump to the last (first) thumbnail. Instead the focus will
        stay unchanged.
      - <b>Cursor left/right:</b> Moves one column to the left/right without changing the relative
        position in the line. Pressing Left on the first column or Right on the last column has
        no effect. Pressing Right on the second-to-last column moves to the last column in any case.
        The relative position in the column is kept if possible. Otherwise the last thumbnail
        becomes the new focused item.
      - <b>Page up/down:</b> Acts in the same way as Cursor left/right but moves as many columns as
        fit on the screen. Only fully visible columns count and clamping to the first/last column
        applies. When all thumbnails fit on one screen jumps from the first to the last column and
        vice versa.
      - <b>Home:</b> Moves to the beginning of the current column.
      - <b>End:</b> Moves to the end of the current column.
      - <b>Ctrl+Home:</b> Moves to the first thumbnail.
      - <b>Ctrl+End:</b> Moves to the last thumbnail.
  */
  virtual int MoveIndex(const int currentIdx, QKeyEvent* event);

  /*! Reimplemented from \c ptGridThumbnailLayouter::Step() */
  virtual int Step() { return m_ThumbMetrics.CellWidth; }


//==============================================================================
};
#endif // PTCOLUMNGRIDTHUMBNAILLAYOUTER_H
