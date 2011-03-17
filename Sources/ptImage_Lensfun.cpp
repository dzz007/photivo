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

ptImage* ptImage::Lensfun(const int LfunActions, const lfModifier* LfunData, float* TransformedCoords) {

  // Stage 2: Vignetting.
  if ((LfunActions & LF_MODIFY_VIGNETTING) ||
      (LfunActions & LF_MODIFY_CCI) ||   // TODO: not yet supported by Photivo
      (LfunActions == LF_MODIFY_ALL) )
  {
    LfunData->ApplyColorModification(m_Image, 0.0, 0.0, m_Width, m_Height,
                                     LF_CR_3(RED, GREEN, BLUE),
                                     m_Width * 3 * 2);    // 3 color components, 2 bytes per color
  }

  // Stage 1+3: CA, lens Geometry/distortion
  if (((LfunActions & LF_MODIFY_TCA) &&
      (LfunActions & (LF_MODIFY_DISTORTION | LF_MODIFY_GEOMETRY))) ||
      (LfunActions == LF_MODIFY_ALL) )
  {
    TransformedCoords = (float*) CALLOC((uint32_t)m_Width * m_Height * 2, sizeof(float));
    LfunData->ApplySubpixelGeometryDistortion(0.0, 0.0, m_Width, m_Height, TransformedCoords);

  // Stage 1 only: CA
  } else if (LfunActions & LF_MODIFY_TCA) {
    TransformedCoords = (float*) CALLOC((uint32_t)m_Width * m_Height * 2, sizeof(float));
    LfunData->ApplySubpixelDistortion(0.0, 0.0, m_Width, m_Height, TransformedCoords);

  // Stage 3 only: lens Geometry/distortion
  } else if (LfunActions & (LF_MODIFY_DISTORTION | LF_MODIFY_GEOMETRY)) {
    TransformedCoords = (float*) CALLOC((uint32_t)m_Width * m_Height * 2, sizeof(float));
    LfunData->ApplyGeometryDistortion(0.0, 0.0, m_Width, m_Height, TransformedCoords);
  }

  return this;
}
