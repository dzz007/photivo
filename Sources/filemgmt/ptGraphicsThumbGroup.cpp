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

#include <QCursor>

#include "../ptDefines.h"
#include "../ptTheme.h"
#include "../ptSettings.h"
#include "ptGraphicsThumbGroup.h"

extern ptSettings* Settings;
extern ptTheme* Theme;

//==============================================================================

ptGraphicsThumbGroup::ptGraphicsThumbGroup(QGraphicsItem* parent /*= 0*/)
: QGraphicsRectItem(parent)
{
  setAcceptHoverEvents(true);
  setCursor(QCursor(Qt::PointingHandCursor));
  setPen(QPen(Qt::NoPen));
  setBrush(QBrush(Qt::NoBrush));
}

//==============================================================================

void ptGraphicsThumbGroup::addItems(QGraphicsPixmapItem* pixmap, QGraphicsTextItem* description) {
  qreal ThumbSize = (qreal)Settings->GetInt("ThumbnailSize");

  // center pixmap in the cell if it is not square
  // the +2 offset is for the hover border
  pixmap->setPos(ThumbSize / 2 - pixmap->pixmap().width() / 2 + 2,
                 ThumbSize / 2- pixmap->pixmap().height() / 2 + 2);
  pixmap->setAcceptHoverEvents(false);
  pixmap->setParentItem(this);

  description->setAcceptHoverEvents(false);
  description->setPos(2, ThumbSize + 2);
  description->setParentItem(this);

  // set rectangle size for the hover border
  this->setRect(0,
                0,
                ThumbSize + 4,
                ThumbSize + description->boundingRect().height() + 4);
}

//==============================================================================

void ptGraphicsThumbGroup::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
  event->accept();
  this->setPen(QPen(Theme->ptHighLight));
}

//==============================================================================

void ptGraphicsThumbGroup::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
  event->accept();
  this->setPen(QPen(Qt::NoPen));
}
