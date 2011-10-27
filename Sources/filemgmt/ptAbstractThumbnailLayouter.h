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

#ifndef PTABSTRACTTHUMBNAILLAYOUTER_H
#define PTABSTRACTTHUMBNAILLAYOUTER_H

//==============================================================================

#include <QGraphicsView>

#include "../ptConstants.h"
#include "ptGraphicsThumbGroup.h"

//==============================================================================
/*!
  \class ptAbstractThumbnailLayouter

  \brief Pure abstract base class for the thumbnail layouter.
*/
class ptAbstractThumbnailLayouter {
public:
  /*! This is an abstract class! Do not instantiate! */
  explicit ptAbstractThumbnailLayouter(QGraphicsView* view)
    : m_LazyInit(false), m_ThumbCount(0), m_View(view) {}

  /*! Initializes the layouter. You must initialize before calling \c Layout().
      \ param font
        The font used to calculate text dimensions.
  */
  virtual void Init(const int thumbCount, const QFont& font) = 0;

  /*! Tells the layouter to initialize the next time you call \c Layout().
      Uses the thumbnail’s \c font() function to determine the font to use.
  */
  virtual void LazyInit(const int thumbCount)
    { m_LazyInit = true; m_ThumbCount = thumbCount; }

  /*! Sets the proper position of a thumbnail.
    \param thumb
      A pointer to the thumbnail.
  */
  virtual void Layout(ptGraphicsThumbGroup* thumb) = 0;

  /*! Returns the scroll step, i.e. the amount of pixels that need to be scrolled
      to advance by one thumbnail.
  */
  virtual int Step() = 0;


protected:
  bool            m_LazyInit;
  int             m_ThumbCount;
  QGraphicsView*  m_View;


//==============================================================================
};

#endif // PTABSTRACTTHUMBNAILLAYOUTER_H
