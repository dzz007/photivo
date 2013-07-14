/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2012-2013 Michael Munzert <mail@mm-log.com>
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

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "ptError.h"
#include "ptCalloc.h"
#include "ptImage8.h"
#include "ptImage.h"
#include "ptResizeFilters.h"

//==============================================================================

ptImage8::ptImage8():
  m_Width(0),
  m_Height(0),
  m_Colors(0),
  m_ColorSpace(ptSpace_sRGB_D65),
  m_SizeBytes(0)
{}

//==============================================================================

ptImage8::ptImage8(uint16_t AWidth,
                   uint16_t AHeight,
                   short    ANrColors)
{
  m_Image.clear();
  m_ColorSpace = ptSpace_sRGB_D65;
  setSize(AWidth, AHeight, ANrColors);
}

//==============================================================================

void ptImage8::setSize(uint16_t AWidth, uint16_t AHeight, int AColorCount) {
  m_Width     = AWidth;
  m_Height    = AHeight;
  m_Colors    = AColorCount;
  m_SizeBytes = m_Width * m_Height * m_Colors;

  m_Image.resize(m_Width * m_Height);
}

//==============================================================================

void ptImage8::fillColor(uint8_t ARed, uint8_t AGreen, uint8_t ABlue, uint8_t AAlpha) {
  std::array<uint8_t, 4> hTemp = {{ARed, AGreen, ABlue, AAlpha}};
  std::fill(std::begin(m_Image), std::end(m_Image), hTemp);
}

//==============================================================================

ptImage8::~ptImage8() {
  // nothing to do
}

//==============================================================================

ptImage8* ptImage8::Set(const ptImage *Origin) {

  assert(NULL != Origin);
  assert(ptSpace_Lab != Origin->m_ColorSpace);

  setSize(Origin->m_Width, Origin->m_Height, Origin->m_Colors);
  m_ColorSpace = Origin->m_ColorSpace;

  for (uint32_t i = 0; i < static_cast<uint32_t>(m_Width)*m_Height; i++) {
    for (short c = 0; c < 3; c++) {
      // Mind the R<->B swap !
      m_Image[i][2-c] = Origin->m_Image[i][c]>>8;
    }
    m_Image[i][3] = 0xff;
  }

  return this;
}

//==============================================================================

ptImage8 *ptImage8::Set(const ptImage8 *Origin)
{
  assert(NULL != Origin);

  setSize(Origin->m_Width,
          Origin->m_Height,
          Origin->m_Colors);

  m_ColorSpace = Origin->m_ColorSpace;
  m_Image      = Origin->m_Image;

  return this;
}

//==============================================================================

void ptImage8::FromQImage(const QImage AImage)
{
  setSize(AImage.width(), AImage.height(), 4);

  m_ColorSpace = ptSpace_sRGB_D65;
  memcpy((uint8_t (*)[4]) m_Image.data(), AImage.bits(), m_SizeBytes);
}
