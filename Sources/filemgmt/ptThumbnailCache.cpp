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
#include <QFileInfo>

#include "../ptDefines.h"
#include "ptThumbnailCache.h"

//==============================================================================

ptThumbnailCacheKey ptThumbnailCache::GetKey(const QString fileName) {
  QFileInfo file = QFileInfo(fileName);

  if (file.exists() && file.isFile()) {
    // full path + last modified time ensure uniqueness
    return file.canonicalFilePath() + file.lastModified().toString(Qt::ISODate);
  } else {
    return "";
  }
}

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
  QHashIterator<ptThumbnailCacheKey, ptThumbnailCacheObject*> i(*m_Data);
  while (i.hasNext()) {
    ptGraphicsThumbGroup::RemoveRef(i.next().value()->Thumbnail);
    delete i.value();
  }
  m_Data->clear();
}

//==============================================================================

void ptThumbnailCache::CacheThumbnail(ptGraphicsThumbGroup* thumbnail) {
  // Checking for existing cache entry is not necessary here because
  // the thumbnailer takes care of that.
  ptThumbnailCacheObject* obj = new ptThumbnailCacheObject;
  obj->key = GetKey(thumbnail->fullPath());
  obj->lastHit = QDateTime::currentDateTime();
  obj->Thumbnail = ptGraphicsThumbGroup::AddRef(thumbnail);
  m_Data->insert(obj->key, obj);
}

//==============================================================================

ptGraphicsThumbGroup* ptThumbnailCache::RequestThumbnail(const ptThumbnailCacheKey key) {
  ptGraphicsThumbGroup* result = NULL;
  ptThumbnailCacheObject* obj = m_Data->value(key, NULL);

  if (obj) {
    result = obj->Thumbnail;
    obj->lastHit = QDateTime::currentDateTime();
  }

  return result;
}

//==============================================================================

void ptThumbnailCache::Consolidate() {
  int overflow = m_Data->size() - m_Capacity;

  // abort when cache is not too full
  if (overflow <= 0) {
printf("######## Cache not yet full (%d of %d)\n", m_Data->size(), m_Capacity);
    return;
  }

printf("######## CONSOLIDATING (%d of %d, overflow %d)\n", m_Data->size(), m_Capacity, overflow);
  // We iterate through m_Data and add its items to the kill list until that
  // list is full. Then we compare the current m_Data item with the first
  // (i.e. youngest) killList entry to determine if it must be added.
  // QMap is always sorted by key. We use lastHit timestamp converted to
  // milliseconds elapsed since start of Unix time to ensure that the kill list
  // is sorted from youngest to oldest entry.
  QMap<qint64, ptThumbnailCacheKey> killList;
  QHashIterator<ptThumbnailCacheKey, ptThumbnailCacheObject*> i(*m_Data);
  while (i.hasNext()) {
    i.next();
    if (killList.size() < overflow) {
      // kill list not yet full: simply add cache item
      killList.insert(i.value()->lastHit.toMSecsSinceEpoch(), i.value()->key);

    } else {
      if (i.value()->lastHit.toMSecsSinceEpoch() > killList.begin().key()) {
        // current cache item is older than youngest one in killLis
        killList.erase(killList.begin());
        killList.insert(i.value()->lastHit.toMSecsSinceEpoch(), i.value()->key);
      }
    }
  }
printf("######## size of killList: %d\n", killList.count());
  // remove the actual "too old" cache entries
  QMapIterator<qint64, ptThumbnailCacheKey> j(killList);
  while (j.hasNext()) {
    ptThumbnailCacheObject* t = m_Data->take(j.next().value());
    ptGraphicsThumbGroup::RemoveRef(t->Thumbnail);
    delete t;
  }

printf("######## DONE (%d of %d)\n", m_Data->size(), m_Capacity);
}

//==============================================================================

void ptThumbnailCache::RemoveThumbnail(const ptThumbnailCacheKey key) {
  ptThumbnailCacheObject* obj = m_Data->take(key);
  if (obj) {
    ptGraphicsThumbGroup::RemoveRef(obj->Thumbnail);
    delete obj;
  }
}

//==============================================================================
