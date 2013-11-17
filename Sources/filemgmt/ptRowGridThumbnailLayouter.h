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

#ifndef PTROWGRIDTHUMBNAILLAYOUTER_H
#define PTROWGRIDTHUMBNAILLAYOUTER_H

//==============================================================================

#include "../ptSettings.h"
#include "ptGridThumbnailLayouter.h"

#include <QGraphicsView>

extern ptSettings* Settings;

//==============================================================================

class ptRowGridThumbnailLayouter: public ptGridThumbnailLayouter {
public:
  /*! Creates an instance of a row-grid layouter.
    \param view
      A pointer to the \c QGraphcisView that contains the \c QGraphicsScene
      where the thumbnails are displayed.
  */
  explicit ptRowGridThumbnailLayouter(QGraphicsView* view);

  /*! Reimplemented from ptGridThumbnailLayouter::Init() */
  virtual void Init(const int thumbCount, const QFont& font);

  /*! Reimplemented from \c ptGridThumbnailLayouter::Layout()  */
  virtual void Layout(ptGraphicsThumbGroup* thumb);

  /*! Reimplemented from \c ptGridThumbnailLayouter::MoveIndex()\n
      Recognised keys are listed below. Their behaviour is modelled after the common navigation
      key behaviour in most text processing applications.
      - <b>Cursor left/right:</b> Moves one thumbnail to the left/right, changing lines if necessary
        and clamping to the beginning and end of the list. I.e. pressing left (right) at the
        first (last) thumbnail will not jump to the last (first) thumbnail. Instead the focus will
        stay unchanged.
      - <b>Cursor up/down:</b> Moves one line up/down without changing the relative position in
        the line. Pressing Up on the first line or Down on the last line has no effect. Pressing
        Down on the second-to-last line moves to the last line in any case. The relative position
        in the line is kept if possible. Otherwise the last thumbnail becomes the new focused item.
      - <b>Page up/down:</b> Acts in the same way as Cursor up/down but moves as many lines as fit
        on the screen. Only fully visible lines count and clamping to the first/last line applies.
        When all thumbnails fit on one screen jumps from the first to the last line and vice versa.
      - <b>Home:</b> Moves to the beginning of the current line.
      - <b>End:</b> Moves to the end of the current line.
      - <b>Ctrl+Home:</b> Moves to the first thumbnail.
      - <b>Ctrl+End:</b> Moves to the last thumbnail.
  */
  virtual int MoveIndex(const int currentIdx, QKeyEvent* event);

  /*! Reimplemented from \c ptGridThumbnailLayouter::Step() */
  virtual int Step() { return m_ThumbMetrics.CellHeight; }


//==============================================================================
};
#endif // PTROWGRIDTHUMBNAILLAYOUTER_H
