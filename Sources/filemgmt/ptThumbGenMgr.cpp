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

#include "ptThumbGenMgr.h"
#include "../ptInfo.h"
#include <QApplication>

//------------------------------------------------------------------------------
// Register user-defined types with the Qt meta object system.
// Needed for communication between the thumbnail and GUI thread.
auto TThumbAssoc_Dummy   = qRegisterMetaType<TThumbAssoc>("photivo_TThumbAssoc");
auto QLTThumbAssoc_Dummy = qRegisterMetaType<QList<TThumbAssoc>>("photivo_QList_TThumbAssoc");

//------------------------------------------------------------------------------
/*! Creates a ptThumbGenMgr object and starts the threaded worker. */
ptThumbGenMgr::ptThumbGenMgr() {
  FThread.start(QThread::LowPriority);
  FWorker.moveToThread(&FThread);
}

//------------------------------------------------------------------------------
/*! Destroys a ptThumbGenMgr object. Also stops processing and shuts down threads. */
ptThumbGenMgr::~ptThumbGenMgr() {
  if (FWorker.isRunning())
    this->abort();

  // Quit the thread. The loop makes sure the QThread object is not deleted before
  // the thread has actually stopped. Deleting a QThread with a running thread is
  // likely to produce a crash.
  FThread.quit();
  FThread.wait();
}

//------------------------------------------------------------------------------
/*! Aborts the currently running thumbnail generation. */
void ptThumbGenMgr::abort() {
  FWorker.abort();

  while (this->isRunning()) {
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  }
}

//------------------------------------------------------------------------------
/*! Aborts thumbnail generation and clears the thumbnail cache. */
void ptThumbGenMgr::clear() {
  FWorker.clear();
}

//------------------------------------------------------------------------------
void ptThumbGenMgr::connectBroadcast(const QObject* AReceiver, const char* ABroadcastSlot) {
  if (!QObject::connect(&FWorker, SIGNAL(broadcast(uint,TThumbPtr)), AReceiver, ABroadcastSlot))
    GInfo->Raise("Could not connect thumbnail worker's broadcast signal.", AT);
}

//------------------------------------------------------------------------------
bool ptThumbGenMgr::isRunning() const {
  return FWorker.isRunning();
}

//------------------------------------------------------------------------------
/*!
  Requests a list of thumbnail images and returns immediately. When the actual image data
  is ready the broadcast() signal is emitted by ptThumbGenWorker once for each image.
*/
void ptThumbGenMgr::request(QList<TThumbAssoc> AThumbList) {
  /*
    When you write a new method that calls into the worker it MUST route the call through
    the worker thread’s event loop as shown below.
    That is required to hide the details of thread management. From the caller’s point of view
    calling ptThumbGenMgr/Worker methods look like simple function calls even though they are
    queued slot calls behind the scenes. In effect this approach ensures that any code that
    accesses data on the thumbnail worker thread is always executed from within that thread,
    eliminating the need for any explict thread-safety measures.
    Note that parameters of user-defined type must be registered with the meta object system first.
    See the declaration of TThumbId for details.
  */
  QMetaObject::invokeMethod(&FWorker, "request", Qt::QueuedConnection,
                            Q_ARG(QList<TThumbAssoc>, AThumbList));
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
