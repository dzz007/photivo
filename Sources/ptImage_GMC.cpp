/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include <cstdlib>
#include <cstdio>

#include "ptImage.h"
#include "ptError.h"
#include <wand/magick_wand.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

// Lut
extern float ToFloatTable[0x10000];

// Write Image
bool ptImage::ptGMCWriteImage(const char* FileName,
                              const short Format,
                              const int Quality,
                              const int Sampling,
                              const int Resolution,
                              const char* ColorProfileFileName,
                              const int Intent) {

  long unsigned int Width  = m_Width;
  long unsigned int Height = m_Height;

  MagickWand *mw;

  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:none");
  MagickSetImageFormat(mw,"RGB");
  MagickSetImageDepth(mw,16);
  MagickSetImageType(mw,TrueColorType);
  MagickSetImageOption(mw, "tiff", "alpha", "associated");

  MagickSetImagePixels(mw,0,0,Width,Height,"RGB",ShortPixel,(unsigned char*) m_Image);

  // Color profile
  // Color profile in PNG is not supported, due to limitations in imagemagick
  if (ColorProfileFileName != NULL) {
    FILE* pFile = fopen (ColorProfileFileName, "rb" );
    if (pFile==NULL) {
      ptLogError(ptError_FileOpen,FileName);
      exit(EXIT_FAILURE);
    }
    fseek (pFile , 0 , SEEK_END);
    long lSize = ftell (pFile);
    rewind (pFile);

    uint8_t* pchBuffer = (uint8_t*) CALLOC(lSize,sizeof(uint8_t));
    ptMemoryError(pchBuffer,__FILE__,__LINE__);

    size_t RV = fread (pchBuffer, 1, lSize, pFile);
    if (RV != (size_t) lSize) {
      ptLogError(ptError_FileOpen,FileName);
      exit(EXIT_FAILURE);
    }
    MagickSetImageProfile(mw,"ICC",pchBuffer,lSize);

    FCLOSE (pFile);
    FREE (pchBuffer);
  }
  // Redering intent
  switch(Intent) {
    case 0:
      MagickSetImageRenderingIntent(mw,PerceptualIntent);
      break;
    case 1:
      MagickSetImageRenderingIntent(mw,RelativeIntent);
      break;
    case 2:
      MagickSetImageRenderingIntent(mw,SaturationIntent);
      break;
    case 3:
      MagickSetImageRenderingIntent(mw,AbsoluteIntent);
      break;
    default:
      assert(0);
  }

  // Depth
  if ((Format==ptSaveFormat_TIFF8) ||
      (Format==ptSaveFormat_PPM8) ||
      (Format==ptSaveFormat_JPEG) ||
      (Format==ptSaveFormat_PNG)) {
    MagickSetImageDepth(mw,8);
  }

  // Jpeg Sampling
  double factors111[3] = {1.0,1.0,1.0};
  double factors211[3] = {2.0,1.0,1.0};
  if (Format==ptSaveFormat_JPEG) {
    if (Sampling==ptSaveSampling_111) {
      MagickSetSamplingFactors(mw,3,factors111);
    } else {
      MagickSetSamplingFactors(mw,3,factors211);
    }
  }

  // Quality
  if (Format==ptSaveFormat_PNG || Format==ptSaveFormat_PNG16)
    MagickSetCompressionQuality(mw, floor((double) Quality/10.0)*10+5);
  else
    MagickSetCompressionQuality(mw,Quality);

  // Compression
  if ((Format==ptSaveFormat_TIFF8) ||
      (Format==ptSaveFormat_TIFF16)) {
    MagickSetImageCompression(mw, LZWCompression);
  }

  // Resolution
  MagickSetImageUnits(mw, PixelsPerInchResolution);
  MagickSetImageResolution(mw, Resolution, Resolution);

  bool Result = MagickWriteImage(mw, FileName);
  DestroyMagickWand(mw);
  return Result;
}



