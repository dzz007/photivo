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

#ifndef DLIMAGE8_H
#define DLIMAGE8_H

#include <QImage>

#include "ptDefines.h"
#include "ptConstants.h"

//==============================================================================

// forward for faster compilation
class ptImage;

//==============================================================================

// Class containing an 8 bit image. Intentionally very limited.
// Only meant as interface to display.

class ptImage8 {
public:

  // Constructor
  ptImage8();

  // Destructor
  ~ptImage8();

  // Just initialize a black image from the given sizes.
  ptImage8(uint16_t AWidth,
           uint16_t AHeight,
           short    ANrColors = 3);

  void setSize(uint16_t AWidth,
               uint16_t AHeight,
               int      AColorCount);

  void fillColor(uint8_t ARed,
                 uint8_t AGreen,
                 uint8_t ABlue,
                 uint8_t AAlpha);

  // Initialize it from a ptImage.
  // Copying is always deep (so including copying the image).
  ptImage8* Set(const ptImage *Origin);

  // Deep copy
  ptImage8* Set(const ptImage8 *Origin);

  // We copy a QImage
  void FromQImage(const QImage AImage);

  // just write an image to disk
  bool DumpImage(const char* FileName, const bool BGR) const;

  TImage8Data& image() { return m_Image; }

  uint16_t height()     const { return m_Height; }
  uint16_t width()      const { return m_Width; }
  short    colors()     const { return m_Colors; }
  short    colorspace() const { return m_ColorSpace; }
  uint     sizeBytes()  const { return m_SizeBytes; }

private:
  // The image , assumed 3 channels and 0..0xff values.
  // Representation that allows direct usage in Qt context.
  // [0] = B
  // [1] = G
  // [2] = R
  // [3] = A = 0xff
  TImage8Data m_Image;

  // Width and height of the image
  uint16_t m_Width;
  uint16_t m_Height;

  // Nr of colors in the image (probably always 3 ?)
  short m_Colors;

  // Color space.
  // Can be one of
  //   ptSpace_sRGB_D65         1
  //   ptSpace_AdobeRGB_D65     2
  //   ptSpace_WideGamutRGB_D65 3
  //   ptSpace_ProPhotoRGB_D65  4
  short m_ColorSpace;

  uint m_SizeBytes;
};

#endif
////////////////////////////////////////////////////////////////////////////////
