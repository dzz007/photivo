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

#include "ptThumbGenHelpers.h"

/*! Creates a ptFlowController object and sets its open state to AInitiallyOpen. */
ptFlowController::ptFlowController(bool AInitiallyOpen):
  FFlowing(AInitiallyOpen)
{}

/*! Destroys a ptFlowController object. */
ptFlowController::~ptFlowController() {}

/*! Sets the open state. */
void ptFlowController::setOpen(bool AIsOpen) {
  ptMutexLocker hLock(&FFlowMutex);
  FFlowing = AIsOpen;
}

/*! Switches from open to closed state or vice-versa and returns the new state. */
bool ptFlowController::toggleOpen() {
  ptMutexLocker hLock(&FFlowMutex);
  FFlowing = !FFlowing;
  return FFlowing;
}

/*! Return true if the state is “open”, false otherwise. */
bool ptFlowController::isOpen() const {
  ptMutexLocker hLock(&FFlowMutex);
  return FFlowing;
}

//------------------------------------------------------------------------------

ptThumbQueue::ptThumbQueue() {}
ptThumbQueue::~ptThumbQueue() {}

/*! Removes all items from the queue. */
void ptThumbQueue::clear() {
  ptMutexLocker hLock(&FItemsMutex);
  FItems.clear();
}

/*! Returns the next item in the queue or an invalid TThumbId if the queue is empty. */
TThumbAssoc ptThumbQueue::dequeue() {
  ptMutexLocker hLock(&FItemsMutex);
  if (FItems.isEmpty()) {
    return TThumbAssoc();
  } else {
    return FItems.takeFirst();
  }
}

/*!
  Adds a new item to the queue.
  APriority controls where the item is placed. Normal-priority items are added at the end
  of the queue; high-priority items are placed at the beginning so that the next call to
  dequeue() will return that item.
*/
void ptThumbQueue::enqueue(const TThumbAssoc& AItem, TThumbQPrio APriority) {
//  ptMutexLocker hLock(&FItemsMutex);
  switch (APriority) {
    case TThumbQPrio::High: FItems.prepend(AItem); break;
    case TThumbQPrio::Normal: FItems.append(AItem); break;
    default: GInfo->Raise("Unexpected case branch", AT);
  }
}

/*! This function overloads enqueue(). Instead of a single item it enqueues a list of items. */
void ptThumbQueue::enqueue(const QList<TThumbAssoc>& AItems, TThumbQPrio APriority) {
//  ptMutexLocker hLock(&FItemsMutex);

  for (auto& hItem: AItems) {
    switch (APriority) {
      case TThumbQPrio::High: FItems.prepend(hItem); break;
      case TThumbQPrio::Normal: FItems.append(hItem); break;
      default: GInfo->Raise("Unexpected case branch", AT);
    }
  }
}

/*! Returns true if no items are in the queue. */
bool ptThumbQueue::isEmpty() const {
  ptMutexLocker hLock(&FItemsMutex);
  return FItems.isEmpty();
}

//------------------------------------------------------------------------------

TThumbId makeThumbId(const QString& AFilename, int ALongEdgeMax, ptFSOType AType) {
  return makeThumbId(QFileInfo(AFilename), ALongEdgeMax, AType);
}


TThumbId makeThumbId(const QFileInfo& AFileInfo, int ALongEdgeMax, ptFSOType AType) {
  TThumbId hThumbId { AFileInfo.canonicalFilePath(), AFileInfo.lastModified(), AType, ALongEdgeMax };

  if (AType == fsoUnknown) {
    (AFileInfo.isFile()) ? (hThumbId.Type = fsoFile) : (hThumbId.Type = fsoDir);
  }

  return hThumbId;
}
