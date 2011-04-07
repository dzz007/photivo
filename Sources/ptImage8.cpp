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

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "ptError.h"
#include "ptImage8.h"
#include "ptImage.h"
#include "ptResizeFilters.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage8::ptImage8() {
m_Width              = 0;
m_Height             = 0;
m_Image              = NULL;
m_Colors             = 0;
m_ColorSpace         = ptSpace_sRGB_D65;
};

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage8::ptImage8(const uint16_t Width,
                   const uint16_t Height,
                   const short    NrColors) {
  m_Width              = Width;
  m_Height             = Height;
  m_Image              = NULL;
  m_Colors             = NrColors;
  m_ColorSpace         = ptSpace_sRGB_D65;

  m_Image = (uint8_t (*)[4]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);
};

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage8::~ptImage8() {
  FREE(m_Image);
};

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

  // Free maybe preexisting.
  FREE(m_Image);

  m_Image = (uint8_t (*)[4]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  for (uint32_t i=0 ; i<(uint32_t)m_Width*m_Height; i++) {
    for (short c=0; c<3; c++) {
      // Mind the R<->B swap !
      m_Image[i][2-c] = Origin->m_Image[i][c]>>8;
    }
    m_Image[i][3] = 0xff;
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// SimpleScale
//
////////////////////////////////////////////////////////////////////////////////

ptImage8* ptImage8::SimpleScale(const float Factor) {

  assert(m_Colors ==3);
  assert (Factor>0.0 && Factor<=1.0);

  if (fabs(Factor-1.0) < 0.01 ) return this;

  uint16_t   Divider = MAX(m_Height,m_Width);
  uint16_t   Multiplier = (uint16_t)(Divider*Factor);
  uint32_t   Normalizer = Divider * Divider;

  uint16_t NewHeight = m_Height * Multiplier / Divider;
  uint16_t NewWidth  = m_Width  * Multiplier / Divider;

  uint64_t (*Image64Bit)[3] =
    (uint64_t (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*Image64Bit));
  ptMemoryError(Image64Bit,__FILE__,__LINE__);

  for(uint16_t r=0; r<m_Height; r++) {
    /* r should be divided between ri and rii */
    uint16_t ri  = r * Multiplier / Divider;
    uint16_t rii = (r+1) * Multiplier / Divider;
    /* with weights riw and riiw (riw+riiw==Multiplier) */
    int64_t riw  = rii * Divider - r * Multiplier;
    int64_t riiw = (r+1) * Multiplier - rii * Divider;
    if (rii>=NewHeight) {
      rii  = NewHeight-1;
      riiw = 0;
    }
    if (ri>=NewHeight) {
      ri  = NewHeight-1;
      riw = 0;
    }
    for(uint16_t c=0; c<m_Width; c++) {
      uint16_t ci   = c * Multiplier / Divider;
      uint16_t cii  = (c+1) * Multiplier / Divider;
      int64_t  ciw  = cii * Divider - c * Multiplier;
      int64_t  ciiw = (c+1) * Multiplier - cii * Divider;
      if (cii>=NewWidth) {
        cii  = NewWidth-1;
        ciiw = 0;
      }
      if (ci>=NewWidth) {
        ci  = NewWidth-1;
        ciw = 0;
      }
      for (short cl=0; cl<3; cl++) {
        Image64Bit[ri *NewWidth+ci ][cl] += m_Image[r*m_Width+c][cl]*riw *ciw ;
        Image64Bit[ri *NewWidth+cii][cl] += m_Image[r*m_Width+c][cl]*riw *ciiw;
        Image64Bit[rii*NewWidth+ci ][cl] += m_Image[r*m_Width+c][cl]*riiw*ciw ;
        Image64Bit[rii*NewWidth+cii][cl] += m_Image[r*m_Width+c][cl]*riiw*ciiw;
      }
    }
  }

  FREE(m_Image); // free the old image.
  m_Image = NULL;

  m_Image =
    (uint8_t (*)[4]) CALLOC(NewWidth*NewHeight,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  // Fill the image from the Image64Bit.
  for (uint32_t c=0; c<(uint32_t)NewHeight*NewWidth; c++) {
    for (short cl=0; cl<3; cl++) {
      m_Image[c][cl] = Image64Bit[c][cl]/Normalizer;
    }
    m_Image[c][3] = 0xff;
  }

  FREE(Image64Bit);

  m_Width  = NewWidth;
  m_Height = NewHeight;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// FilteredScale
// Scale with a resampling filter.
//
////////////////////////////////////////////////////////////////////////////////

// TODO
// Note : I have tried this in all possible combinations with
// integer types, in which case 64 bit needed for image, or with
// combinations of integer images and floating weights but nothing
// comes close to the float performance.
// Someone else ?
//
// Also this implementation has been tweaked by taking as much
// as possible calculations (especially * and /) from the inner loops.

ptImage8* ptImage8::FilteredScale(const float Factor,
                                  const short ResizeFilter) {

  assert(m_Colors ==3);
  assert (Factor>0.0);

  const float Lobes = FilterLobes[ResizeFilter];

  if (not FilterTableInited[ResizeFilter]) {
    FilterTableSize[ResizeFilter] = (uint16_t) (Lobes*SamplesPerLobe+1);
    FilterTable[ResizeFilter] =
      (float*) CALLOC(FilterTableSize[ResizeFilter],sizeof(*FilterTable));
    ptMemoryError(FilterTable,__FILE__,__LINE__);
    for (uint16_t i=0; i<FilterTableSize[ResizeFilter]; i++) {
      float x = (float) i / SamplesPerLobe;
      FilterTable[ResizeFilter][i] = (*FilterFunction[ResizeFilter])(x);
    }
    FilterTableInited[ResizeFilter] = 1;
  }

  float*      Table = FilterTable[ResizeFilter];

  // Some precalculations based on the resizing factor
  // MIN/MAX corrections are for upsampling.
  float    Ratio     = 1/Factor;
  uint16_t NewHeight = (uint16_t)(m_Height / Ratio+0.5);
  uint16_t NewWidth  = (uint16_t)(m_Width / Ratio+0.5);
  float    Scale      = MIN(1.0,1.0/Ratio);
  float    Radius     = MAX(Lobes* Ratio,Lobes);
  int32_t  ScaledLobe = (int32_t)(SamplesPerLobe*Scale);

  // X Size change

  // Be aware, height still that of the original image in this pass!
  float (*DstImageX)[3] =
    (float (*)[3]) CALLOC(NewWidth*m_Height,sizeof(*DstImageX));
  ptMemoryError(DstImageX,__FILE__,__LINE__);

  for (uint16_t OrgRow=0; OrgRow<m_Height; OrgRow++) {
    for (uint16_t DstCol=0; DstCol<NewWidth; DstCol++) {
      uint32_t DstPointer = OrgRow*NewWidth+DstCol;
      float    OrgCenter  = (DstCol+0.5) * Ratio; // Checked.OK.
      int32_t  OrgLeft    = (int32_t)(OrgCenter-Radius);
      int32_t  OrgRight   = (int32_t)(OrgCenter+Radius);
      float    SumWeight = 0;
      float    x = (OrgCenter-OrgLeft-0.5)*Scale; // TODO -0.5 correct ?
      int32_t  idx = (int32_t)(x*SamplesPerLobe);
      for (int32_t i=OrgLeft; i<=OrgRight; i++,x-=Scale,idx-=ScaledLobe) {
        if (i<0 || i>=m_Width) continue;
        if (fabs(x) <= Lobes) {
          float Weight = Table[abs(idx)];
          SumWeight+=Weight;
          uint32_t OrgPointer = OrgRow*m_Width+i;
          DstImageX[DstPointer][0] += m_Image[OrgPointer][0] * Weight;
          DstImageX[DstPointer][1] += m_Image[OrgPointer][1] * Weight;
          DstImageX[DstPointer][2] += m_Image[OrgPointer][2] * Weight;
        }
      }
      // One division & three multiplications is faster then three divisions
      SumWeight = 1/SumWeight;
      DstImageX[DstPointer][0] *= SumWeight;
      DstImageX[DstPointer][1] *= SumWeight;
      DstImageX[DstPointer][2] *= SumWeight;
    }
  }

  // At this stage we can free memory of the original image to
  // reduce a bit the memory demand.
  FREE(m_Image);

  // Y Size reduction

  float (*DstImageY)[3] =
    (float (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*DstImageY));
  ptMemoryError(DstImageY,__FILE__,__LINE__);

  // NewWidth as the X direction is resized already.
  for (uint16_t OrgCol=0; OrgCol<NewWidth; OrgCol++) {
    for (uint16_t DstRow=0; DstRow<NewHeight; DstRow++) {
      uint32_t DstPointer = DstRow*NewWidth+OrgCol;
      float    OrgCenter  = (DstRow+0.5)*Ratio; // Checked. OK.
      int32_t  OrgLeft    = (int32_t)(OrgCenter-Radius);
      int32_t  OrgRight   = (int32_t)(OrgCenter+Radius);
      float    SumWeight = 0;
      float    x   = (OrgCenter-OrgLeft-0.5)*Scale; // TODO -0.5 correct ?
      int32_t  idx = (int32_t)(x*SamplesPerLobe);
      for (int32_t i=OrgLeft; i<=OrgRight; i++,x-=Scale,idx-=ScaledLobe) {
        if (i<0 || i>=m_Height) continue;
        if (fabs(x) <= Lobes) {
          float Weight = Table[abs(idx)];
          SumWeight+=Weight;
          uint32_t OrgPointer = i*NewWidth+OrgCol;
          DstImageY[DstPointer][0] += DstImageX[OrgPointer][0]*Weight;
          DstImageY[DstPointer][1] += DstImageX[OrgPointer][1]*Weight;
          DstImageY[DstPointer][2] += DstImageX[OrgPointer][2]*Weight;
        }
      }
      // One division & three multiplications is faster then three divisions
      SumWeight = 1/SumWeight;
      DstImageY[DstPointer][0] *= SumWeight;
      DstImageY[DstPointer][1] *= SumWeight;
      DstImageY[DstPointer][2] *= SumWeight;
    }
  }

  // At this stage we can free memory of the DstImageX image to
  // reduce a bit the memory demand.
  FREE(DstImageX);

  // The image worked finally upon is 'this' or a new created one.

  m_Image =
    (uint8_t (*)[4]) CALLOC(NewWidth*NewHeight,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  // Fill the image from the DstImage.
  for (uint32_t c=0; c<(uint32_t)NewHeight*NewWidth; c++) {
    for (short cl=0; cl<3; cl++) {
      m_Image[c][cl] = (uint16_t)CLIP(DstImageY[c][cl]+0.5);
    }
    m_Image[c][3] = 0xff;
  }

  FREE(DstImageY);

  m_Width  = NewWidth;
  m_Height = NewHeight;

  return this;
}
////////////////////////////////////////////////////////////////////////////////
//
// WriteAsPpm
//
////////////////////////////////////////////////////////////////////////////////

short ptImage8::WriteAsPpm(const char*  FileName) {

  FILE *OutputFile = fopen(FileName,"wb");
  if (!OutputFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  fprintf(OutputFile,"P%d\n%d %d\n%d\n",m_Colors/2+5, m_Width, m_Height, 0xff);

  uint8_t*  PpmRow = (uint8_t *) CALLOC(m_Width,m_Colors);
  ptMemoryError(PpmRow,__FILE__,__LINE__);

  for (uint16_t Row=0; Row<m_Height; Row++) {
    for (uint16_t Col=0; Col<m_Width; Col++) {
      for (short c=0;c<3;c++) {
        // Mind the R<->B swap !
        PpmRow [Col*m_Colors+c] = m_Image[Row*m_Width+Col][2-c];
      }
    }
    assert ( m_Width == fwrite(PpmRow,m_Colors,m_Width,OutputFile) );
  }

  FREE(PpmRow);
  FCLOSE(OutputFile);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
