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

#ifndef PTTHUMBDEFINES_H
#define PTTHUMBDEFINES_H

#include <memory>
#include <ctime>
#include <QString>

//==============================================================================

class ptImage8;

//==============================================================================

typedef std::shared_ptr<ptImage8> ptThumbPtr;

//==============================================================================

struct ptThumbId {
  explicit ptThumbId();
  explicit ptThumbId(QString AFileName, uint16_t AMaxSize);

  void init();
  bool isEqual(const ptThumbId AId) const;

  QString  FileName;
  uint16_t MaxSize;
};

//==============================================================================

struct ptThumbData {
  ptThumbId   Id;
  ptThumbPtr  Thumbnail;
  std::time_t LastAccess;

  void init();
};

//==============================================================================

#endif // PTTHUMBDEFINES_H
