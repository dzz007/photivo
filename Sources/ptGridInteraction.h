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
/*!
** Displays a rectangular grid on top of the preview image.
**
** - Does not emit the finished() signal because no lengthy interaction is going
**   on, only simple enabling, show(), and disabling, hide(), of the grid.
** - You do not need to call hide() explicitely before destroying a
**   ptGridInteraction object.
**/

#ifndef PTGRIDINTERACTION_H
#define PTGRIDINTERACTION_H

//==============================================================================

#include <QList>

#include "ptAbstractInteraction.h"
#include <QGraphicsLineItem>

//==============================================================================

class ptGridInteraction : public ptAbstractInteraction {
Q_OBJECT

public:
  explicit ptGridInteraction(QGraphicsView* AView);
  ~ptGridInteraction();

  /*! Reimplemented from base class. */
  virtual Qt::KeyboardModifiers modifiers()    const { return Qt::NoModifier; }

  /*! Reimplemented from base class. */
  virtual ptMouseActions        mouseActions() const { return maNone; }

  /*! Reimplemented from base class. */
  virtual Qt::MouseButtons      mouseButtons() const { return Qt::NoButton; }

  void show(const uint ALinesX, const uint ALinesY);
  inline void hide() { ClearList(); }

//------------------------------------------------------------------------------

private:
  QList<QGraphicsLineItem*> FGridLines;
  void ClearList();


};
#endif // PTGRIDINTERACTION_H
