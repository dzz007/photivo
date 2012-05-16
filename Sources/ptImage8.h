/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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

// The image , assumed 3 channels and 0..0xff values.
// Representation that allows direct usage in Qt context.
// [0] = B
// [1] = G
// [2] = R
// [3] = A = 0xff
uint8_t (*m_Image)[4];

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

// Constructor
ptImage8();

// Just initialize a black image from the given sizes.
ptImage8(const uint16_t Width,
         const uint16_t Height,
         const short    NrColors = 3);

void setSize(const uint16_t AWidth, const uint16_t AHeight, const int AColorCount);

void setSize(const uint16_t AWidth, const uint16_t AHeight, const int AColorCount);

// Destructor
~ptImage8();

// Initialize it from a ptImage.
// Copying is always deep (so including copying the image).
ptImage8* Set(const ptImage *Origin);

// Scale with Facto 0..1
// Always in place !
ptImage8* SimpleScale(const float Factor);
ptImage8* FilteredScale(const float Factor,
                        const short Filter = ptResizeFilter_Triangle);

// Write the image as a ppm file.
// Be aware : the writing function does *not* add gamma, thus
// it needs to be done before if needed.
short WriteAsPpm(const char*  FileName);

// just write an image to disk
bool DumpImage(const char* FileName) const;
};

#endif
////////////////////////////////////////////////////////////////////////////////
