/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#include <lensfun.h>
#include "ptImage.h"

ptImage* Lensfun(const uint32_t LfunActions, const lfModifier* LfunData) {

  // Stage 2: Vignetting and CCI
  if ((LfunActions & LF_MODIFY_VIGNETTING) ||
      (LfunActions & LF_MODIFY_CCI) ||
      (LfunActions == LF_MODIFY_ALL) )
  {
    // TODO BJ: stage 2 stuff here
  }

  // Stage 1+3: CA, lens Geometry/distortion
  if ((LfunActions & LF_MODIFY_GEOMETRY) ||
      (LfunActions & LF_MODIFY_DISTORTION) ||
      (LfunActions == LF_MODIFY_ALL) )
  {
    // TODO BJ: stage 1+3 stuff here
  }
}
