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

#ifndef PTGRIDTHUMBNAILLAYOUTER_H
#define PTGRIDTHUMBNAILLAYOUTER_H

//==============================================================================

#include "ptAbstractThumbnailLayouter.h"

//==============================================================================

class ptGridThumbnailLayouter: public ptAbstractThumbnailLayouter {
public:
  /*! This is an abstract class! Do not instantiate! */
  explicit ptGridThumbnailLayouter(QGraphicsView* view);

  /*! Reimplemented from ptAbstractThumbnailLayouter::Init().
    Initializes Col, Row and Padding. Calculates default cell width and height.
  */
  virtual void Init(const int, const QFont& font);


protected:
  struct {
    int Col;      // zero based index
    int Row;      // zero based index
    int MaxCol;   // max column index
    int MaxRow;   // max row index
    int Padding;
    int CellHeight;   // cell dimensions include padding
    int CellWidth;    //
  } m_ThumbMetrics;


//==============================================================================
};
#endif // PTGRIDTHUMBNAILLAYOUTER_H
