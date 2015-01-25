/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Michael Munzert <mail@mm-log.com>
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

#include "ptMutexLocker.h"
#include <stdexcept>

/*!
  Creates a ptMutexLocker object and locks AMutex. If the lock cannot be acquired after
  ATimeoutMsec throws a std::runtime_error exception.
*/
ptMutexLocker::ptMutexLocker(QMutex* AMutex, int ATimeoutMsec):
  FMutex(AMutex),
  FTimeout(ATimeoutMsec),
  FUnlocked(false)
{
  this->lock();
}

/*! Destroys a ptMutexLocker object and releases the lock if necessary. */
ptMutexLocker::~ptMutexLocker() {
  this->unlock();
}

/*!
  Re-acquires the lock after a call to unlock(). If the lock cannot be acquired after the
  timeout defined in the ctor throws a std::runtime_error exception.
  Does nothing if unlock() was not called previously.
*/
void ptMutexLocker::relock() {
  if (FUnlocked){
    this->lock();
    FUnlocked = false;
  }
}

/*! Releases the lock. Consecutive calls to unlock() are allowed. */
void ptMutexLocker::unlock() {
  if (!FUnlocked) {
    FMutex->unlock();
    FUnlocked = true;
  }
}

void ptMutexLocker::lock() {
  if (!FMutex->tryLock(FTimeout)) {
    throw std::runtime_error("ptMutexLocker: Failed to acquire lock.");
  }
}
