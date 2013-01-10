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

#ifndef PTLOCK_H
#define PTLOCK_H

//==============================================================================

#include <vector>
#include <functional>

//==============================================================================

enum class ptLockType {ThumbCache, ThumbQueue};

//==============================================================================

struct ptLockData;

//==============================================================================

class ptLock
{
public:
  ptLock(const ptLockType ALockType);
  ~ptLock();

  void unlock();
private:
  static const std::vector<ptLockData> FLockData;
  ptLockType                           FCurrentType;

  void WorkOnCurrentType(std::function<void(ptLockData &)> AFunction);
};

//==============================================================================

#endif // PTLOCK_H
