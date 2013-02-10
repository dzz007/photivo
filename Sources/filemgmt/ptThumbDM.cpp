/*******************************************************************************
**
** Photivo
**
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

#include "ptThumbDM.h"
#include "../ptInfo.h"
#include "../ptLock.h"
#include "../ptImage8.h"

//==============================================================================

ptThumbDM::ptThumbDM() :
  QObject(),
  FThumbCache(),
  FThumbGen(),
  FThumbGenSync(),
  FNeededThumbs(),
  FAsync(true),
  FThreadRunning(false),
  FThumbReciever(),
  FRestartTimer()
{
  connect(&FThumbGen, SIGNAL(finished()), this, SLOT(finishedThumbGen()), Qt::QueuedConnection);
  //connect(this,       SIGNAL(restart()),  this, SLOT(startThumbGen()),    Qt::QueuedConnection);

  FRestartTimer.setInterval(5);
  FRestartTimer.setSingleShot(true);
  connect(&FRestartTimer, SIGNAL(timeout()), this, SLOT(startThumbGen()));
}

//==============================================================================

ptThumbDM::~ptThumbDM()
{
}

//==============================================================================
// First we check, if the thumbnail is cached.
// If so, we distribute it immediately.
// Otherwise we generate it.
void ptThumbDM::orderThumb(const ptThumbId AThumbId, const bool ACacheThumb)
{
  ptThumbData hThumb;
  hThumb.init();
  hThumb.Id = AThumbId;

  // Access to the thumb cache is serialized
  ptLock hCacheLock(ptLockType::ThumbCache);

  if (FThumbCache.findThumb(hThumb)) {
    distributeThumbnail(hThumb);
    hThumb.LastAccess = ptNow();
    hCacheLock.unlock();
  } else {
    hCacheLock.unlock();
    { // Access to the thumb queue is serialized
      ptLock hLock(ptLockType::ThumbQueue);
      if (AThumbId.MaxSize == 0) {
        FNeededThumbs.push_front({AThumbId, ACacheThumb});
      } else {
        FNeededThumbs.push_back({AThumbId, ACacheThumb});
      }
      hLock.unlock();
    }
    FRestartTimer.start();
  }
}

//==============================================================================

void ptThumbDM::cancelThumb(const ptThumbId AThumbId)
{
  ptLock hLock(ptLockType::ThumbQueue);
  for (auto it = FNeededThumbs.begin(); it != FNeededThumbs.end(); ++it) {
    if (AThumbId.isEqual(it->Id)) {
      FNeededThumbs.erase(it);
      return;
    }
  }
}

//==============================================================================

void ptThumbDM::setSyncMode(const bool AAsync)
{
  FAsync = AAsync;
}

//==============================================================================

void ptThumbDM::addThumbReciever(ptThumbReciever *AReciever)
{
  if (!AReciever) {
    GInfo->Raise("No valid thumbnail reciever", AT);
  }

  for (auto it = FThumbReciever.begin(); it != FThumbReciever.end(); ++it) {
    if (*it == AReciever) {
      return;
    }
  }
  FThumbReciever.push_back(AReciever);
}

//==============================================================================

void ptThumbDM::removeThumbReciever(ptThumbReciever *AReciever)
{
  for (auto it = FThumbReciever.begin(); it != FThumbReciever.end(); ++it) {
    if (*it == AReciever) {
      FThumbReciever.erase(it);
      return;
    }
  }
}

//==============================================================================

ptThumbData ptThumbDM::getThumbnail(const ptThumbId AThumbId)
{
  FThumbGenSync.setCurrentThumb(AThumbId);
  FThumbGenSync.run();
  FThumbGenSync.wait();

  ptThumbData hThumb;
  hThumb.init();

  hThumb.Id         = FThumbGenSync.getCurrentThumb();
  hThumb.Thumbnail  = FThumbGenSync.getThumbnail();
  hThumb.LastAccess = ptNow();

  return hThumb;
}

//==============================================================================
// We distribute the new thumb and we insert it into the cache, if needed.
void ptThumbDM::finishedThumbGen()
{
  ptThumbData hThumb;
  hThumb.init();

  hThumb.Id         = FThumbGen.getCurrentThumb();
  hThumb.Thumbnail  = FThumbGen.getThumbnail();
  hThumb.LastAccess = ptNow();

  distributeThumbnail(hThumb);

  ptLock hThreadLock(ptLockType::ThumbGen);
  FThreadRunning = false;
  hThreadLock.unlock();
  FRestartTimer.start();
}

//==============================================================================
// If the thread is not running: get the first needed thumb and start ThumbGen.
void ptThumbDM::startThumbGen()
{
  ptLock hThreadLock(ptLockType::ThumbGen);
  if (!FThreadRunning) {
    ptLock hLock(ptLockType::ThumbQueue);
    if (FNeededThumbs.empty()) {
      return;
    }

    FThreadRunning = true;
    hThreadLock.unlock();

    FThumbGen.setCurrentThumb(FNeededThumbs.front().Id);
    FNeededThumbs.pop_front();
    hLock.unlock();

    if (FAsync) {
      FThumbGen.start();
    } else {
      FThumbGen.run();
    }
  } else {
    // nothing to do.
  }
}

//==============================================================================
// We copy the thumbnail to decouple from the cache
void ptThumbDM::distributeThumbnail(const ptThumbData AThumbData)
{
  if (!AThumbData.Thumbnail) {
    return;
  }

  // generate a copy and distribute it
  ptThumbPtr hCopy = std::make_shared<ptImage8>();
  hCopy->Set(AThumbData.Thumbnail.get());
  for (auto it = FThumbReciever.begin(); it != FThumbReciever.end(); ++it) {
    (*it)->thumbnail(AThumbData.Id, hCopy);
  }
}
