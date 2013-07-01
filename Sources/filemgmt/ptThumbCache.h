/*******************************************************************************
**
** Photivo
**
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
*******************************************************************************
**
** Shout-outs go out to Tim Day, whose article "LRU cache implementation in C++"
** <http://timday.bitbucket.org/lru.html> was a great inspiration for this
** implementation.
**
*******************************************************************************/

#ifndef PTTHUMBCACHE_H
#define PTTHUMBCACHE_H

#include "ptThumbDefines.h"
#include <QHash>
#include <QMap>
#include <QLinkedList>
#include <QString>
#include <QMutex>

/*! ptThumbCache implements a least-recently-used cache for thumbnail images. */
class ptThumbCache {

public:
  explicit ptThumbCache(uint AMaxSizeBytes);
  ~ptThumbCache();

  void clear();
  TThumbPtr find(const TThumbId &AThumbId);
  void insert(const TThumbId &AThumbId, TThumbPtr AThumbData);

private:
  // Types for the cache data structures. For an in-depth discussion of the
  // cache’s structure see http://timday.bitbucket.org/lru.html
  typedef QString TThumbKey;
  typedef QLinkedList<TThumbKey> TAccessTracker;

  // Size of the image is saved independently because it might change between cache insertion
  // and eviction. However both actions must be performed with the same size to ensure proper
  // occupancy calculation.
  struct TThumbValue {
    TThumbPtr                 Data;
    uint                      Size;
    TAccessTracker::iterator  AccessIter;
  };

  typedef QHash<TThumbKey, TThumbValue> TThumbCache;

  void      evict();
  TThumbKey makeHashable(const TThumbId& AThumbId) const;

  TAccessTracker  FAccess;
  TThumbCache     FCache;
  uint            FCapacity;  // in bytes
  uint            FOccupancy; // in bytes
  QMutex          FMutex;
};

#endif // PTTHUMBCACHE_H
