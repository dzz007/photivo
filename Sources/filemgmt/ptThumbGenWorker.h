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
#include <QObject>
#include <QSize>
#include <QMutex>
#include <wand/magick_wand.h>

class ptThumbCache;
class ptThumbQueue;
class ptFlowController;

/*!
  The ptThumbGenWorker class generates thumbnail images, either from the thumbnail cache
  or from files on disk, and also handles the thumbnails for broken/unsupported images
  and directories.

  It is a worker class for ptThumbGenMgr and not supposed to be used on its own.
*/
class ptThumbGenWorker: public QObject {
Q_OBJECT

public:
  explicit ptThumbGenWorker(ptThumbQueue* AQueue, ptThumbCache* ACache, ptFlowController* AAbortCtrl);
  ~ptThumbGenWorker();
  
  bool isRunning() const;
  void start();

signals:
  void broadcast(uint AReceiverId, TThumbPtr AImage);

private:
  void postProcessEvent();

  TThumbPtr generateThumb(const TThumbId& AThumbId);
  QSize scaleSize(int AWidth, int AHeight, int AMaxLongEdge);
  void setIsRunning(bool AValue);
  void transformImage(MagickWand* AInImage, ptImage8* AOutImage, const QSize& ASize);

  // Access to following bool variables MUST ALWAYS be protected via their respective mutexes.
  // Use ptMutexLocker for easy locking/unlocking. Do NOT use QMutexLocker.
  bool FIsRunning;    // access via isRunning() and setRunning(), they take care of locking
  mutable QMutex FIsRunningMutex;

  const ptFlowController* const FAbortCtrl;
  ptThumbCache* const FThumbCache;
  ptThumbQueue* const FThumbQueue;

  const char* Process_Func = "process";

private slots:
  void process();
};

#endif // PTTHUMBGENWORKER_H
