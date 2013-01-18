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

/*static*/
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
  m_NextIdx = 0;
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
//    ptGraphicsThumbGroup::RemoveRef(i.next().value()->Thumbnail);
    delete i.value();
  }
  m_Data->clear();
}

//==============================================================================

void ptThumbnailCache::CacheThumbnail(ptGraphicsThumbGroup* thumbnail) {
  // Checking for existing cache entry is not necessary here because
  // the thumbnailer takes care of that and QHash does not allow duplicate
  // keys anyway.
  ptThumbnailCacheObject* obj = new ptThumbnailCacheObject;
  obj->key = GetKey(thumbnail->fullPath());
  obj->lastHit = GetIdx();
//  obj->Thumbnail = ptGraphicsThumbGroup::AddRef(thumbnail);
  m_Data->insert(obj->key, obj);

  //
  if (m_Data->count() > m_Capacity) {
    uint age = UINT_MAX;
    ptThumbnailCacheKey killKey = "";
    QHashIterator<ptThumbnailCacheKey, ptThumbnailCacheObject*> i(*m_Data);
    while (i.hasNext()) {
      i.next();
      if (i.value()->lastHit < age) {
        killKey = i.value()->key;
        age = i.value()->lastHit;
      }
    }
    RemoveThumbnail(killKey);
  }
}

//==============================================================================

ptGraphicsThumbGroup* ptThumbnailCache::RequestThumbnail(const ptThumbnailCacheKey key) {
  ptGraphicsThumbGroup* result = NULL;
  ptThumbnailCacheObject* obj = m_Data->value(key, NULL);

  if (obj) {
    result = obj->Thumbnail;
    obj->lastHit = GetIdx();
  }

  return result;
}

//==============================================================================

// Not used atm. Keeping it for easy access should we need it again. Is probably
// not fully functional in current form.
//
//void ptThumbnailCache::Consolidate() {
//  int overflow = m_Data->size() - m_Capacity;

//  // abort when cache is not too full
//  if (overflow <= 0) {
//printf("######## Cache not yet full (%d of %d)\n", m_Data->size(), m_Capacity);
//    return;
//  }

//printf("######## CONSOLIDATING (%d of %d, overflow %d)\n", m_Data->size(), m_Capacity, overflow);
//  // We iterate through m_Data and add its items to the kill list until that
//  // list is full. Then we compare the current m_Data item with the last
//  // (i.e. youngest) killList entry to determine if it must be added.
//  // QMap is always sorted by key. We use lastHit timestamp converted to
//  // milliseconds elapsed since start of Unix time to ensure that the kill list
//  // is sorted from youngest to oldest entry.
//  QMap<uint, ptThumbnailCacheKey> killList;
//  QHashIterator<ptThumbnailCacheKey, ptThumbnailCacheObject*> i(*m_Data);
////  QTime timer;
////  timer.start();
//  while (i.hasNext()) {
//    i.next();
//    if (killList.size() < overflow) {
//      // kill list not yet full: simply add cache item
//      killList.insert(i.value()->lastHit, i.value()->key);

//    } else {
//      if (i.value()->lastHit > killList.end().key()) {
//        // current cache item is older than youngest one in killLis
//        killList.erase(killList.end());
//        killList.insert(i.value()->lastHit, i.value()->key);
//      }
//    }
////printf("%d(%d)\n", killList.count(), i.value()->lastHit);
//  }
////  printf("********** %d\n", timer.elapsed());
//printf("######## size of killList: %d\n", killList.count());
//  // remove the actual "too old" cache entries
//  QMapIterator<uint, ptThumbnailCacheKey> j(killList);
//  while (j.hasNext()) {
//    ptThumbnailCacheObject* t = m_Data->take(j.next().value());
//printf("%d  ", t->lastHit);
//    ptGraphicsThumbGroup::RemoveRef(t->Thumbnail);
//    delete t;
//  }

//printf("######## DONE (%d of %d)\n", m_Data->size(), m_Capacity);
//}

////==============================================================================

void ptThumbnailCache::RemoveThumbnail(const ptThumbnailCacheKey key) {
  ptThumbnailCacheObject* obj = m_Data->take(key);
  if (obj) {
//    ptGraphicsThumbGroup::RemoveRef(obj->Thumbnail);
    delete obj;
  }
}

//==============================================================================

uint ptThumbnailCache::GetIdx() {
  // Prevent last hit counter from overflowing, even if that is extremely unlikely
  // to happen before Photivo is restarted.
  if (m_NextIdx == UINT_MAX) {
    m_NextIdx = 0;
    Clear();
    return m_NextIdx;

  } else {
    return m_NextIdx++;
  }
}

//==============================================================================
