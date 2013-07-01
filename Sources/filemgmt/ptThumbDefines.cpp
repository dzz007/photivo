/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptThumbDefines.h"

// Register user-defined types with the Qt meta object system.
// Needed for communication between the thumbnail and GUI thread.
auto TThumbPtr_Dummy = qRegisterMetaType<TThumbPtr>("photivo_TThumbPtr");
auto TThumbId_Dummy  = qRegisterMetaType<TThumbId>("photivo_TThumbId");
auto TThumbAssoc_Dummy   = qRegisterMetaType<TThumbAssoc>("photivo_TThumbAssoc");
auto QLTThumbAssoc_Dummy = qRegisterMetaType<QList<TThumbAssoc>>("photivo_QList_TThumbAssoc");

//------------------------------------------------------------------------------
TThumbId::operator bool() const {
  return !FilePath.isEmpty() &&
         Timestamp.isValid() &&
         (LongEdgeSize >= 0);
}

//------------------------------------------------------------------------------
bool operator==(const TThumbId& lhs, const TThumbId& rhs) {
  return (lhs.FilePath == rhs.FilePath) &&
         (lhs.Timestamp == rhs.Timestamp) &&
         (lhs.Type == rhs.Type) &&
         (lhs.LongEdgeSize == rhs.LongEdgeSize);
}

//------------------------------------------------------------------------------
bool operator!=(const TThumbId& lhs, const TThumbId& rhs) {
  return !(lhs == rhs);
}

//------------------------------------------------------------------------------
TThumbAssoc::operator bool() const {
  return (bool)ThumbId;
}
