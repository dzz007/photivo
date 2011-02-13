/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brother.john@photivo.org>
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

#ifndef PTENUM_H
#define PTENUM_H

// Crop: Position of the mouse when button pressed for dragging
enum ptMovingEdge {
  meNone,
  meTop,
  meRight,
  meBottom,
  meLeft,
  meTopLeft,
  meTopRight,
  meBottomLeft,
  meBottomRight,
  meCenter      // move crop rect instead of resize
};

// User interaction in the view window
enum ptViewportAction {
  vaNone,
  vaCrop,
  vaSelectRect,
  vaDrawLine
};

#endif // PTENUM_H
