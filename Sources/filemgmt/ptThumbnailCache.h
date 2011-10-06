/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#ifndef PTTHUMBNAILCACHE_H
#define PTTHUMBNAILCACHE_H

//==============================================================================

#include <QGraphicsItemGroup>
#include <QHash>
#include <QDateTime>

//==============================================================================

struct ptThumbnailCacheObject {
  QGraphicsItemGroup* Thumbnail;
  QDateTime           lastHit;
  QString             key;    // full path + last modified time
};

//==============================================================================

class ptThumbnailCache {
public:
  /*! Creates a \c ptThumbnailCache instance.
    \param capacity
      The maximum number of entries in the cache. When the cache is full, objects
      that have not been hit the longest are removed first.
  */
  ptThumbnailCache(const int capacity);

  /*! Destroys a \c ptThumbnailCache object. */
  ~ptThumbnailCache();

  /*! Removes all entries from the cache. */
  void Clear();

  /*! Adds a new entry to the cache.
    \param key
      A unique identifier for this entry. You should use the full path name of the
      resp. image file immediately followed by its last modified date. The date is
      necessary to catch changed/replaced files with the same path.
    \param thumbnail
      A pointer to the \c QGraphicsItemGroup object that should be cached.
  */
  void CacheThumbnail(const QString key, QGraphicsItemGroup* thumbnail);

  /*! Removes the oldest (i.e. least recently hit) entries from the cache,
      if there are more entries than the capacity.
  */
  void Consolidate();

  /*! Request a cache entry for the given \c key.
      If a corresponding entry is found a pointer to the appropriate
      \c QGraphicsItemGroup object is returned, otherwise a NULL pointer.
  */
  QGraphicsItemGroup* RequestThumbnail(const QString key);


private:
  QHash<QString, ptThumbnailCacheObject*>* m_Data;
  int m_Capacity;

};

//==============================================================================

#endif // PTTHUMBNAILCACHE_H
