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

#include <QGraphicsView>

#include "../ptSettings.h"
#include "ptGridThumbnailLayouter.h"

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

  /*! Reimplemented from \c ptGridThumbnailLayouter::Step() */
  virtual int Step() { return m_ThumbMetrics.CellHeight; }


//==============================================================================
};
#endif // PTROWGRIDTHUMBNAILLAYOUTER_H
