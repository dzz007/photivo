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

#include "../ptSettings.h"
#include "ptGridThumbnailLayouter.h"
#include "ptGraphicsThumbGroup.h"

extern ptSettings* Settings;

//==============================================================================

ptGridThumbnailLayouter::ptGridThumbnailLayouter(QGraphicsView* view)
: ptAbstractThumbnailLayouter(view),
  m_ThumbMetrics()    // init to zeros
{}

//==============================================================================

/*virtual*/
void ptGridThumbnailLayouter::Init(const int, const QFont& font) {
  // Current row and column
  m_ThumbMetrics.Row = 0;
  m_ThumbMetrics.Col = 0;

  // Padding between thumbnails
  m_ThumbMetrics.Padding = Settings->GetInt("FileMgrThumbnailPadding");

  // Width of a cell, i.e. thumb box width + padding
  m_ThumbMetrics.CellWidth = Settings->GetInt("FileMgrThumbnailSize") +
                             m_ThumbMetrics.Padding +
                             ptGraphicsThumbGroup::InnerPadding*2;

  // Height of a cell, i.e. thumb box height + padding
  m_ThumbMetrics.CellHeight = m_ThumbMetrics.CellWidth +
                              QFontMetrics(font).height() +
                              m_ThumbMetrics.Padding;
}

//==============================================================================
