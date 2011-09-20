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
#include <QQueue>
#include <QHash>

//==============================================================================

class ptThumbnailCache {
public:
  ptThumbnailCache(const int capacity);
  ~ptThumbnailCache();
  void Clear();
  void EnqueueObject(const QString key, QGraphicsItemGroup* object);
  QGraphicsItemGroup* RequestObject(const QString key);

private:
  QHash<QString, QGraphicsItemGroup*>* m_Lookup;
  QQueue<QGraphicsItemGroup*>* m_Queue;
  int m_Capacity;

};

//==============================================================================

#endif // PTTHUMBNAILCACHE_H
