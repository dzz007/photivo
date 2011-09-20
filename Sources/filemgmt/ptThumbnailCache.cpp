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

#include <QMap>

#include "../ptDefines.h"
#include "ptThumbnailCache.h"

//==============================================================================

ptThumbnailCache::ptThumbnailCache(const int capacity) {
  m_Data = new QHash<QString, ptThumbnailCacheObject*>;
  m_Capacity = capacity;
}

//==============================================================================

ptThumbnailCache::~ptThumbnailCache() {
  Clear();
  DelAndNull(m_Data);
}

//==============================================================================

void ptThumbnailCache::Clear() {
  QHashIterator<QString, ptThumbnailCacheObject*> i(*m_Data);
  while (i.hasNext()) {
    i.next();
    delete i.value();
  }

  m_Data->clear();
}

//==============================================================================

void ptThumbnailCache::CacheThumbnail(const QString key, QGraphicsItemGroup* thumbnail) {
  // TODO: Do we need a deep copy of the ItemGroup to avoid problems with groups that
  // are still displayed in the scene but might get removed from cache? I.e. how do we
  // ensure that no ItemGroup can avoid deletion at some point in time?
  ptThumbnailCacheObject* obj = new ptThumbnailCacheObject;
  obj->key = key;
  obj->lastHit = QDateTime::currentDateTime();
  obj->Thumbnail = thumbnail;
  m_Data->insert(key, obj);
}

//==============================================================================

QGraphicsItemGroup* ptThumbnailCache::RequestThumbnail(const QString key) {
  // TODO: same deep copy question as above ;)
  ptThumbnailCacheObject* co = m_Data->value(key, NULL);
  QGraphicsItemGroup* result = NULL;

  if (co) {
    result = co->Thumbnail;
    co->lastHit = QDateTime::currentDateTime();
  }

  return result;
}

//==============================================================================

void ptThumbnailCache::Consolidate() {
  int overflow = m_Data->size() - m_Capacity;

  // abort when cache is not too full
  if (overflow <= 0) {
    return;
  }

  QMap<QDateTime, QString> killList;

  // We iterate through m_Data and add its items to the kill list until that
  // list is full. Then we compare the current m_Data item with the last
  // (i.e. youngest) killList entry to determine if it must be added.
  QHashIterator<QString, ptThumbnailCacheObject*> i(*m_Data);
  while (i.hasNext()) {
    i.next();
    if (killList.size() < overflow) {
      killList.insert(i.value()->lastHit, i.value()->key);
    } else {
      if (i.value()->lastHit < killList.end().key()) {
        killList.erase(killList.end());
        killList.insert(i.value()->lastHit, i.value()->key);
      }
    }
  }


  // remove the actual "too old" cache entries
  QMapIterator<QDateTime, QString> j(killList);
  while (j.hasNext()) {
    j.next();
    ptThumbnailCacheObject* t = m_Data->take(j.value());
    delete t;
  }
}

//==============================================================================
