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
  m_Image(nullptr),
  m_Width(0),
  m_Height(0),
  m_Colors(0),
  m_ColorSpace(ptSpace_sRGB_D65),
  m_SizeBytes(0)
{}

//==============================================================================

ptImage8::ptImage8(const uint16_t Width,
                   const uint16_t Height,
                   const short    NrColors)
{
  m_Image              = nullptr;
  m_ColorSpace = ptSpace_sRGB_D65;
  setSize(Width, Height, NrColors);
}

//==============================================================================

void ptImage8::setSize(const uint16_t AWidth, const uint16_t AHeight, const int AColorCount) {
  m_Width      = AWidth;
  m_Height     = AHeight;
  m_Colors     = AColorCount;
  m_SizeBytes  = m_Width * m_Height * sizeof(*m_Image);

  if (m_Image) FREE(m_Image);

  m_Image      = (uint8_t (*)[4]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);
}

//==============================================================================

ptImage8::~ptImage8() {
  FREE(m_Image);
}

////////////////////////////////////////////////////////////////////////////////
//
// Set
//
////////////////////////////////////////////////////////////////////////////////

ptImage8* ptImage8::Set(const ptImage *Origin) { // Always deep

  assert(NULL != Origin);
  assert(ptSpace_Lab != Origin->m_ColorSpace);

  m_Width      = Origin->m_Width;
  m_Height     = Origin->m_Height;
  m_Colors     = Origin->m_Colors;
  m_ColorSpace = Origin->m_ColorSpace;
  m_SizeBytes  = m_Width * m_Height * sizeof(*m_Image);

  // Free maybe preexisting.
  FREE(m_Image);

  m_Image = (uint8_t (*)[4]) MALLOC(m_SizeBytes);
  for (uint32_t i=0 ; i<(uint32_t)m_Width*m_Height; i++) {
    for (short c=0; c<3; c++) {
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

  memcpy(m_Image, Origin->m_Image, m_Width*m_Height*sizeof(*m_Image));

  return this;
}

//==============================================================================

void ptImage8::FromQImage(const QImage AImage)
{
  setSize(AImage.width(), AImage.height(), 4);

  m_ColorSpace = ptSpace_sRGB_D65;
  memcpy(m_Image, AImage.bits(), m_Width*m_Height*sizeof(*m_Image));
}

//------------------------------------------------------------------------------
uint ptImage8::sizeBytes() const {
  return m_SizeBytes;
}
