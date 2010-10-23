////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DLLENSFUN_H
#define DLLENSFUN_H

#include <QString>

#include "lensfun.h"
#include <stdint.h>

struct ptLensfunCamera {
  QString   Make;
  QString   Model;
  QString   Variant;
  QString   Mount;
  const lfCamera* Camera;
};

struct ptLensfunLens {
  QString   Make;
  QString   Model;
  QString   Mount; // We'll make a lens of each mount too.
  const lfLens*   Lens;
};

class ptLensfun {
public :
ptLensfunCamera* m_Cameras;
uint16_t         m_NrCameras;
ptLensfunLens*   m_Lenses;
uint16_t         m_NrLenses;
ptLensfun();
lfError LoadDir(const char* Directory);
private:
lfDatabase* m_Database;
};

extern ptLensfun* LensfunData;

#endif
////////////////////////////////////////////////////////////////////////////////
