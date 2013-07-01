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

#ifndef PTMUTEXLOCKER_H
#define PTMUTEXLOCKER_H

#include <QMutex>

/*!
   *ptMutexLocker* is a simple helper class for locking/unlocking a QMutex. Always prefer it
   over QMutexLocker because QMutexLocker does not support locking with a timeout.
*/
class ptMutexLocker {
public:
  explicit ptMutexLocker(QMutex *AMutex, int ATimeoutMsec = 5000);
  ~ptMutexLocker();

  void relock();
  void unlock();

private:
  void lock();

  QMutex* FMutex;
  int     FTimeout;
  bool    FUnlocked;
};

#endif // PTMUTEXLOCKER_H
