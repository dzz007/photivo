/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#include "ptFileMgrDM.h"

//==============================================================================

void ClearThumbnailData( ptThumbnailData &Data) {
  if (Data.Thumbnail) delete Data.Thumbnail;
  Data.Thumbnail = 0;
  Data.Location  = "";
}

//==============================================================================

ptFileMgrDM* ptFileMgrDM::m_Instance = 0;

//==============================================================================

ptFileMgrDM& ptFileMgrDM::Instance_GoC() {
  if ( m_Instance == 0 ) m_Instance = new ptFileMgrDM();

  return *m_Instance;
}

//==============================================================================

void ptFileMgrDM::Clear() {
  // Dummy
}

//==============================================================================

void ptFileMgrDM::Instance_Destroy() {
  if ( m_Instance != 0 ) delete m_Instance;

  m_Instance = 0;
}

//==============================================================================
