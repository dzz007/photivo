/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Michael Munzert <mail@mm-log.com>
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

#include "ptThumbCache.h"

ptThumbCache::ptThumbCache() {}

//------------------------------------------------------------------------------

ptThumbCache::~ptThumbCache() {}

//------------------------------------------------------------------------------
/*! Immediately empties the entire cache. */
void ptThumbCache::clear() {
}

//------------------------------------------------------------------------------

/*!
  Returns the image matching AThumbId from the cache, or an empty TThumbPtr if
  no matching image can be found.
*/
TThumbPtr ptThumbCache::find(const TThumbId& AThumbId) {
  // TODO: actually implement the cache
  return false;
}

//------------------------------------------------------------------------------

/*! Inserts a new image into the cache. */
void ptThumbCache::insert(const TThumbId& AThumbId, TThumbPtr AThumbData) {
}
