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

#include "../ptDefines.h"
#include "ptThumbnailCache.h"

//==============================================================================

ptThumbnailCache::ptThumbnailCache(const int capacity) {
  m_Lookup = new QHash<QString, QGraphicsItemGroup*>;
  m_Queue = new QQueue<QGraphicsItemGroup*>;
  m_Capacity = capacity;
}

//==============================================================================

ptThumbnailCache::~ptThumbnailCache() {
  Clear();
  DelAndNull(m_Lookup);
  DelAndNull(m_Queue);
}

//==============================================================================

void ptThumbnailCache::Clear() {
  QGraphicsItemGroup* thumb;
  foreach(thumb, m_Queue) {
    delete thumb;
  }
  m_Queue->clear();
  m_Lookup->clear();
}

//==============================================================================

void ptThumbnailCache::EnqueueObject(const QString key, QGraphicsItemGroup* object) {
  if (m_Queue->count() == m_Capacity) {
    m_Lookup->remove(m_Lookup->keys(m_Queue->dequeue()).at(0));
  }

  m_Lookup->insert(key, object);
}

//==============================================================================

QGraphicsItemGroup* ptThumbnailCache::RequestObject(const QString key) {
  QGraphicsItemGroup* result = m_Lookup->value(key, NULL);

  // if we have a cache hit move that object to first pos in cache queue
  if (result) {
    // Search from queue’s end asuming newer entries get requested again more often
    for (int i = m_Queue->count() - 1; i >= 0 ; i--) {
      if (m_Queue->at(i) == result) {
        m_Queue->enqueue(m_Queue->takeAt(i));
      }
    }
  }

  return result;
}

//==============================================================================
