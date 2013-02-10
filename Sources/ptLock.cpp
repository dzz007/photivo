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
#include <cstdio>
#include <QMutex>
#include <QString>

#include "ptLock.h"

//==============================================================================

struct ptLockData {
  ptLockType              Type;
  std::shared_ptr<QMutex> Lock;
  QString                 Name;
};
// The QMutex is in a shared pointer, to have someone, who cares for construction
// and destruction.

//==============================================================================

const std::vector<ptLockData> ptLock::FLockData = {
  {ptLockType::ThumbCache,   std::make_shared<QMutex>(QMutex::NonRecursive), "Cache"},
  {ptLockType::ThumbQueue,   std::make_shared<QMutex>(QMutex::NonRecursive), "Queue"},
  {ptLockType::ThumbDisplay, std::make_shared<QMutex>(QMutex::NonRecursive), "Display"},
  {ptLockType::ThumbGen,     std::make_shared<QMutex>(QMutex::NonRecursive), "ThumbGen"},
  {ptLockType::ThumbLayout,  std::make_shared<QMutex>(QMutex::NonRecursive), "ThumbLayout"},
  {ptLockType::Progressbar,  std::make_shared<QMutex>(QMutex::NonRecursive), "Progressbar"}
};

//==============================================================================

ptLock::ptLock(const ptLockType ALockType) :
  FCurrentType(ALockType),
  FUnlocked(false)
{
  WorkOnCurrentType([&](ptLockData ALockData){
    (ALockData.Lock)->lock();
  });
}

//==============================================================================

ptLock::~ptLock()
{  
  WorkOnCurrentType([&](ptLockData ALockData) {
    (ALockData.Lock)->unlock();
  });
}

//==============================================================================

void ptLock::unlock()
{
  WorkOnCurrentType([&](ptLockData ALockData){
    if (!FUnlocked) {
      (ALockData.Lock)->unlock();
      FUnlocked = true;
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
  printf("Type was not found in FLockData.");
  throw;
}

//==============================================================================
