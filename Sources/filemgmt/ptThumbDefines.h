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

#ifndef PTTHUMBDEFINES_H
#define PTTHUMBDEFINES_H

#include "../ptConstants.h"
#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <memory>

class ptImage8;
class ptGraphicsThumbGroup;

//------------------------------------------------------------------------------
/*! A smart pointer to transport ptImage8 data safely between thread boundaries. */
typedef std::shared_ptr<ptImage8> TThumbPtr;
Q_DECLARE_METATYPE(TThumbPtr)

//------------------------------------------------------------------------------
/*! The TThumbId struct is a unique identifier for a thumbnail image of a specific size. */
struct TThumbId {
  QString   FilePath;     /*! absolute path to the file. Case-sensitive! */
  QDateTime Timestamp;    /*! "last access" time of the file */
  ptFSOType Type;         /*! marks the path as a directory */
  int       LongEdgeSize; /*! length of the thumbnail’s long edge in pixels */

  explicit operator bool() const;
};
Q_DECLARE_METATYPE(TThumbId)
// See also the .cpp for the qRegisterMetaType() call

bool operator==(const TThumbId& lhs, const TThumbId& rhs);
bool operator!=(const TThumbId& lhs, const TThumbId& rhs);

//------------------------------------------------------------------------------
/*! Struct to associate a specific thumbgroup object with a specific image file on disk. */
struct TThumbAssoc {
  TThumbId ThumbId;   /*! the thumbnail’s unique ID */
  uint     GroupId;   /*! the ptGraphicsThumbGroup’s unique ID */

  explicit operator bool() const;
};
Q_DECLARE_METATYPE(TThumbAssoc)
Q_DECLARE_METATYPE(QList<TThumbAssoc>)

#endif // PTTHUMBDEFINES_H
