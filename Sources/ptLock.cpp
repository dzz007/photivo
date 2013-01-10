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

#include <memory>
#include <QMutex>

#include "ptLock.h"
#include "ptInfo.h"

//==============================================================================

struct ptLockData {
  ptLockType              Type;
  std::shared_ptr<QMutex> Lock;
  bool                    Unlocked;
};

//==============================================================================

const std::vector<ptLockData> ptLock::FLockData = {
  {ptLockType::ThumbCache, std::make_shared<QMutex>(QMutex::NonRecursive), false},
  {ptLockType::ThumbQueue, std::make_shared<QMutex>(QMutex::NonRecursive), false}
};

//==============================================================================

ptLock::ptLock(const ptLockType ALockType) :
  FCurrentType(ALockType)
{
  WorkOnCurrentType([](ptLockData ALockData){
    (ALockData.Lock)->lock();
  });
}

//==============================================================================

ptLock::~ptLock()
{
  WorkOnCurrentType([](ptLockData ALockData){
    if (!ALockData.Unlocked) {
      (ALockData.Lock)->unlock();
    }
  });
}

//==============================================================================

void ptLock::unlock()
{
  WorkOnCurrentType([](ptLockData ALockData){
    if (!ALockData.Unlocked) {
      (ALockData.Lock)->unlock();
      ALockData.Unlocked = true;
    }
  });
}

//==============================================================================

void ptLock::WorkOnCurrentType(std::function<void(ptLockData &)> AFunction)
{
  foreach(ptLockData hLockData, FLockData) {
    if (hLockData.Type == FCurrentType) {
      AFunction(hLockData);

      return;
    }
  }
  // We only get here, if we haven't found the type...
  GInfo->Raise("Type was not found in FLockData.", AT);
}

//==============================================================================
