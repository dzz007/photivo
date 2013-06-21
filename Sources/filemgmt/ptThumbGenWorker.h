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
*******************************************************************************/

#ifndef PTTHUMBGENWORKER_H
#define PTTHUMBGENWORKER_H

#include "ptThumbDefines.h"
#include "ptThumbCache.h"
#include <QObject>
#include <QSize>
#include <QQueue>
#include <QMutex>
#include <wand/magick_wand.h>
#include <memory>

/*!
  The ptThumbGenWorker class generates thumbnail images, either from the thumbnail cache
  or from files on disk, and also handles the thumbnails for broken/unsupported images
  and directories.

  It is a worker class for ptThumbGenMgr and not supposed to be used on its own.
*/
class ptThumbGenWorker: public QObject {
Q_OBJECT

public:
  explicit ptThumbGenWorker();
  ~ptThumbGenWorker();
  
  void abort();
  void clear();
  bool isRunning() const;

public slots:
  void request(QList<TThumbAssoc> AThumbList);

signals:
  void broadcast(uint AReceiverId, TThumbPtr AImage);

private:
  TThumbPtr generate(const TThumbId& AThumbId);
  void processRequests();
  void stopProcessing();
  void scaleSize(QSize& ASize, int ALongEdge);
  void setIsRunning(bool AValue);
  void transformImage(MagickWand* AInImage, ptImage8* AOutImage, const QSize& ASize);

  // Access to following bool variables MUST ALWAYS be protected via their respective mutexes.
  // Use ptMutexLocker for easy locking/unlocking. Do NOT use QMutexLocker.
  bool   FAbortSignaled;
  QMutex FAbortMutex;
  bool   FIsRunning;
  mutable QMutex FIsRunningMutex;

  ptThumbCache FThumbCache;
  QQueue<TThumbAssoc> FThumbQueue;
};

#endif // PTTHUMBGENWORKER_H
