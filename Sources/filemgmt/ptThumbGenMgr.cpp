/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptThumbGenMgr.h"
#include "../ptInfo.h"
#include <QApplication>

//------------------------------------------------------------------------------
/*! Creates a ptThumbGenMgr object and starts the threaded workers. */
ptThumbGenMgr::ptThumbGenMgr():
  FAbortCtrl(false),
  FThumbCache(200*1024*1024) // TODO BJ: Make cache size configurable
{
  // Determine worker count. Then create each worker object and an associated QThread object,
  // start the thread and move the worker to that thread.
  int hThreadCount = qMax(CMinWorkerThreads, QThread::idealThreadCount());

  for (int i = 0; i < hThreadCount; ++i) {
    FWorkers.append(new ptThumbGenWorker(&FThumbQueue, &FThumbCache, &FAbortCtrl));

    FThreadpool.append(new QThread);
    FThreadpool.last()->start(QThread::LowPriority);

    FWorkers.last()->moveToThread(FThreadpool.last());
  }
}

//------------------------------------------------------------------------------
/*! Destroys a ptThumbGenMgr object. Also stops processing and shuts down threads. */
ptThumbGenMgr::~ptThumbGenMgr() {
  this->abort();

  // Quit the threads. The two loops are intentional to safe time. There is not reason not to
  // continue to the next while the previous is still shutting down.
  for (auto& hThread: FThreadpool) {
    hThread->quit();
  }
  for (auto& hThread: FThreadpool) {
    hThread->wait();
    DelAndNull(hThread);
  }

  for (auto& hWorker: FWorkers) {
    DelAndNull(hWorker);
  }
}

//------------------------------------------------------------------------------
/*! Aborts the currently running thumbnail generation. */
void ptThumbGenMgr::abort() {
  FAbortCtrl.setOpen(false);

  while (this->isRunning()) {
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  }
}

//------------------------------------------------------------------------------
/*! Aborts thumbnail generation and clears the thumbnail cache. */
void ptThumbGenMgr::clear() {
  this->abort();
  FThumbCache.clear();
}

//------------------------------------------------------------------------------
void ptThumbGenMgr::connectBroadcast(const QObject* AReceiver, const char* ABroadcastSlot) {
  for (auto& hWorker: FWorkers) {
    if (!QObject::connect(hWorker, SIGNAL(broadcast(uint,TThumbPtr)),
                          AReceiver, ABroadcastSlot,
                          Qt::QueuedConnection))
    {
      GInfo->Raise("Could not connect thumbnail worker's broadcast signal.", AT);
    }
  }
}

//------------------------------------------------------------------------------
bool ptThumbGenMgr::isRunning() const {
  for (auto& hWorker: FWorkers) {
    if (hWorker->isRunning()) {
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------
/*!
  Requests a thumbnail images and returns immediately. When the actual image data
  is ready the broadcast() signal is emitted by ptThumbGenWorker.
*/
void ptThumbGenMgr::request(const TThumbAssoc& AThumb) {
  FThumbQueue.enqueue(AThumb);
  this->start();
}

//------------------------------------------------------------------------------
/*!
  This function overloads request(). Requests a list of thumbnail images. When the image
  data is ready the broadcast() signal is emitted by ptThumbGenWorker once for each image.
*/
void ptThumbGenMgr::request(const QList<TThumbAssoc>& AThumbList) {
  FThumbQueue.enqueue(AThumbList);
  this->start();
}

//------------------------------------------------------------------------------
// Starts the thumbnail workers. If they’re already running this function has no effect.
void ptThumbGenMgr::start() {
  FAbortCtrl.setOpen(true);
  for (auto& hWorker: FWorkers) {
    hWorker->start();
  }
}

//------------------------------------------------------------------------------
TThumbId makeThumbId(const QString& AFilename, int ALongEdgeMax, ptFSOType AType) {
  return makeThumbId(QFileInfo(AFilename), ALongEdgeMax, AType);
}

//------------------------------------------------------------------------------
TThumbId makeThumbId(const QFileInfo& AFileInfo, int ALongEdgeMax, ptFSOType AType) {
  TThumbId hThumbId { AFileInfo.canonicalFilePath(), AFileInfo.lastModified(), AType, ALongEdgeMax };

  if (AType == fsoUnknown) {
    (AFileInfo.isFile()) ? (hThumbId.Type = fsoFile) : (hThumbId.Type = fsoDir);
  }

  return hThumbId;
}
