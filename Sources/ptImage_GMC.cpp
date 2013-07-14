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

#include <QtGlobal>
#include <QFile>
#include <QString>

#include <wand/magick_wand.h>
#ifdef _OPENMP
  #include <omp.h>
#endif

#include <cassert>

#include "ptImage.h"
#include "ptImage8.h"
#include "ptError.h"
#include "ptCalloc.h"

// Lut
extern float ToFloatTable[0x10000];

//==============================================================================

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
  MagickReadImage(mw,"xc:white");
  MagickSetImageFormat(mw,"RGB");
  MagickSetImageDepth(mw,16);
  MagickSetImageType(mw,TrueColorType);

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

//==============================================================================

// just write an image to disk for testing
bool ptImage::DumpImage(const char* FileName) const {

  long unsigned int Width  = m_Width;
  long unsigned int Height = m_Height;

  MagickWand *mw;
  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:white");
  MagickSetImageFormat(mw,"RGB");
  MagickSetImageDepth(mw,16);
  MagickSetImageType(mw,TrueColorType);

  MagickSetImagePixels(mw,0,0,Width,Height,"RGB",ShortPixel,(unsigned char*) m_Data.data());

  MagickSetImageDepth(mw,8);

  MagickSetImageCompression(mw, LZWCompression);

  bool Result = MagickWriteImage(mw, FileName);
  DestroyMagickWand(mw);
  return Result;
}

//==============================================================================

// Open Image
ptImage* ptImage::ptGMCOpenImage(const char*        FileName,
                                 short              ColorSpace,
                                 short              Intent,
                                 short              ScaleFactor,
                                 bool               IsRAW,
                                 TImage8RawData*    ImgData,
                                 int&               Success)
{
  Success = 0;

  MagickWand* image = NewMagickWand();
  ExceptionType MagickExcept;

  if (IsRAW) {
    MagickReadImageBlob(image, (const uchar*)ImgData->data(), (const size_t)ImgData->size());
  } else {
    if (!QFile::exists(QString(FileName))) return this;
    MagickReadImage(image, FileName);
  }

  MagickGetException(image, &MagickExcept);
  if (MagickExcept != UndefinedException) {
    return this;
  }

  Success = 1;

  // Get the embedded profile
  cmsHPROFILE InProfile = NULL;

  if (MagickGetImageType(image) != GrayscaleType) {
    ulong ProfileLength = 0;
    uchar* IccProfile = MagickGetImageProfile(image, "ICC", &ProfileLength);

    if (ProfileLength > 0) {
      InProfile = cmsOpenProfileFromMem(IccProfile, ProfileLength);
    }
  }
  if (!InProfile) {
    InProfile = cmsCreate_sRGBProfile();
  }

  MagickSetImageType(image, TrueColorType);
  MagickSetImageFormat(image, "RGB");
  MagickSetImageDepth(image, 16);

  uint16_t NewWidth = MagickGetImageWidth(image);
  uint16_t NewHeight = MagickGetImageHeight(image);

  // Buffer for the data from Magick
  std::vector<std::array<float, 3> > ImageBuffer;
  ImageBuffer.resize((size_t) NewWidth*NewHeight);

  MagickGetImagePixels(image, 0, 0, NewWidth, NewHeight, "RGB", FloatPixel, (uchar*)ImageBuffer.data());

  m_Width = NewWidth;
  DestroyMagickWand(image);

  // Image before color transform, may be binned
  std::vector<std::array<float, 3> > NewImage;

  if (ScaleFactor != 0) {
    // Binning
    NewHeight >>= ScaleFactor;
    NewWidth >>= ScaleFactor;

    short Step = 1 << ScaleFactor;
    int Average = pow(2,2 * ScaleFactor);

    NewImage.resize((size_t)NewWidth*NewHeight);

#pragma omp parallel for schedule(static)
    for (uint16_t Row=0; Row < NewHeight*Step; Row+=Step) {
      for (uint16_t Col=0; Col < NewWidth*Step; Col+=Step) {
        float  PixelValue[3] = {0.0f,0.0f,0.0f};
        for (uint8_t sRow=0; sRow < Step; sRow++) {
          for (uint8_t sCol=0; sCol < Step; sCol++) {
            int32_t index = (Row+sRow)*m_Width+Col+sCol;
            for (short c=0; c < 3; c++) {
              PixelValue[c] += ImageBuffer[index][c];
            }
          }
        }
        for (short c=0; c < 3; c++) {
          NewImage[Row/Step*NewWidth+Col/Step][c]
            = PixelValue[c] / Average;
        }
      }
    }
  } else {
    NewImage = ImageBuffer;
  }

  m_Width      = NewWidth;
  m_Height     = NewHeight;
  m_Colors     = 3;
  m_ColorSpace = ColorSpace;

  // Alloc for the resulting image
  setSize((size_t) m_Width*m_Height);

  if (1) { // was a check if the image is not linear data
    // Transform to linear representation
    cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
    cmsToneCurve* Gamma3[3];
    Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

    cmsHPROFILE OutProfile = 0;

    cmsCIExyY       D65;
    cmsCIExyY       D50;
    cmsWhitePointFromTemp(&D65, 6503);
    cmsWhitePointFromTemp(&D50, 5003);
    cmsCIExyY       DToReference;

    switch (ColorSpace) {
      case ptSpace_sRGB_D65 :
      case ptSpace_AdobeRGB_D65 :
        DToReference = D65;
        break;
      case ptSpace_WideGamutRGB_D50 :
      case ptSpace_ProPhotoRGB_D50 :
        DToReference = D50;
        break;
      default:
        assert(0);
    }

    OutProfile = cmsCreateRGBProfile(&DToReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ColorSpace],
                                     Gamma3);

    if (!OutProfile) {
      ptLogError(ptError_Profile,"Could not generate linear output profile.");
      return NULL;
    }

    cmsFreeToneCurve(Gamma);

    cmsHTRANSFORM Transform;
    Transform = cmsCreateTransform(InProfile,
                                   TYPE_RGB_FLT,
                                   OutProfile,
                                   TYPE_RGB_16,
                                   Intent,
                                   cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

    int32_t Size = m_Width*m_Height;
    int32_t Step = 100000;
#pragma omp parallel for schedule(static)
    for (int32_t i = 0; i < Size; i+=Step) {
      int32_t Length = (i+Step)<Size ? Step : Size - i;
      float* Buffer = &NewImage[i][0];
      uint16_t* Image = &m_Data[i][0];
      cmsDoTransform(Transform,
                     Buffer,
                     Image,
                     Length);
    }

    cmsDeleteTransform(Transform);
    cmsCloseProfile(InProfile);
    cmsCloseProfile(OutProfile);
  } else {
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row < NewHeight; Row++) {
      for (uint16_t Col=0; Col<NewWidth; Col++) {
        for (short c=0; c<3; c++) {
          m_Image[Row*NewWidth+Col][c] = CLIP((int32_t) (NewImage[Row*NewWidth+Col][c]*0xffff));
        }
      }
    }
  }

  return this;
}

//==============================================================================

// just write an image to disk
bool ptImage8::DumpImage(const char* FileName, const bool BGR) const {

  long unsigned int Width  = m_Width;
  long unsigned int Height = m_Height;

  MagickWand *mw;
  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:white");
  MagickSetImageFormat(mw,"RGBA");
  MagickSetImageDepth(mw,8);
  MagickSetImageType(mw,TrueColorType);

  if (BGR)
    MagickSetImagePixels(mw,0,0,Width,Height,"BGRA",CharPixel,(unsigned char*) m_Image.data());
  else
    MagickSetImagePixels(mw,0,0,Width,Height,"RGBA",CharPixel,(unsigned char*) m_Image.data());

  MagickSetImageDepth(mw,8);

  MagickSetImageFormat(mw,"RGB");
  MagickSetCompressionQuality(mw,95);
  MagickSetImageCompression(mw, LZWCompression);

  bool Result = MagickWriteImage(mw, FileName);
  DestroyMagickWand(mw);
  return Result;
}
