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

#include "ptThumbDefines.h"
#include "../ptDefines.h"

//==============================================================================

ptThumbId::ptThumbId() :
  FileName(),
  MaxSize()
{
  init();
}

//==============================================================================

ptThumbId::ptThumbId(QString AFileName, uint16_t AMaxSize) :
  FileName(AFileName),
  MaxSize(AMaxSize)
{
}

//==============================================================================

void ptThumbId::init()
{
  FileName = "";
  MaxSize  = 0;
}

//==============================================================================

bool ptThumbId::isEqual(const ptThumbId AId) const
{
  return (MaxSize  == AId.MaxSize) &&
         (FileName == AId.FileName);
}

//==============================================================================

void ptThumbData::init()
{
  Id.init();
  Thumbnail  = nullptr;
  LastAccess = ptNow();
}


