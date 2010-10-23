////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender
// Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

#include "ptImageMagickC.h"
#include "ptError.h"
#include <wand/magick_wand.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

#include <stdlib.h>
#include <stdio.h>

// Lut
extern float ToFloatTable[0x10000];
/*
void ptIMRotateC(ptImage* Image, const double Angle) {

  long unsigned int Width  = Image->m_Width;
  long unsigned int Height = Image->m_Height;

  double WP = 0xFFFF;
  double invWP = 1/WP;

  MagickWand *mw;
  PixelIterator *inmw, *outmw;
  PixelWand **inpmw, **outpmw;

  MagickWandGenesis();
  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:none");
  MagickSetFormat(mw,"RGB");
  MagickSetDepth(mw,16);
  MagickSetType(mw,TrueColorType);
  //  UndefinedInterpolatePixel, AverageInterpolatePixel, BicubicInterpolatePixel, BilinearInterpolatePixel,
  //  FilterInterpolatePixel, IntegerInterpolatePixel, MeshInterpolatePixel, NearestNeighborInterpolatePixel,
  //  SplineInterpolatePixel
  MagickSetImageInterpolateMethod(mw,BicubicInterpolatePixel);
  MagickSetImageVirtualPixelMethod(mw,BlackVirtualPixelMethod);

  inmw = NewPixelIterator(mw);
  for (long unsigned int Row=0; Row < Height; Row++) {
    inpmw = PixelGetNextIteratorRow(inmw, &Width);
    for (long unsigned int Col=0; Col < Width; Col++) {
      PixelSetRed(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][0]]);
      PixelSetGreen(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][1]]);
      PixelSetBlue(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][2]]);
    }
    PixelSyncIterator(inmw);
  }
  PixelWand *bg;
  bg = NewPixelWand();
  PixelSetColor(bg, "#0000ff");
  MagickRotateImage(mw, bg, Angle);
  //~ MagickDistortImage(mw,ScaleRotateTranslateDistortion,1,&Angle,MagickTrue);
  long unsigned int NewWidth = MagickGetImageWidth(mw);
  long unsigned int NewHeight = MagickGetImageHeight(mw);

  FREE(Image->m_Image);
  Image->m_Width  = NewWidth;
  Image->m_Height = NewHeight;
  Image->m_Image = (uint16_t (*)[3]) CALLOC(Image->m_Width*Image->m_Height,sizeof(*Image->m_Image));
  ptMemoryError(Image->m_Image,__FILE__,__LINE__);

  outmw = NewPixelIterator(mw);
  for (long unsigned int Row=0; Row < NewHeight; Row++) {
    outpmw = PixelGetNextIteratorRow(outmw, &NewWidth);
    for (long unsigned int Col=0; Col < NewWidth; Col++) {
      Image->m_Image[Row*NewWidth+Col][0] = CLIP((int32_t) (PixelGetRed(outpmw[Col])*WP));
      Image->m_Image[Row*NewWidth+Col][1] = CLIP((int32_t) (PixelGetGreen(outpmw[Col])*WP));
      Image->m_Image[Row*NewWidth+Col][2] = CLIP((int32_t) (PixelGetBlue(outpmw[Col])*WP));
    }
  }

  inmw = DestroyPixelIterator(inmw);
  outmw = DestroyPixelIterator(outmw);
  mw = DestroyMagickWand(mw);
  MagickWandTerminus();
}
*/
/*
void ptIMContrastStretch(ptImage* Image,
                        const double BlackPoint,
                        const double WhitePoint,
                        const double Opacity) {

  long unsigned int Width  = Image->m_Width;
  long unsigned int Height = Image->m_Height;

  double WP = 0xFFFF;
  double invWP = 1/WP;

  MagickWand *mw;
  PixelIterator *inmw, *outmw;
  PixelWand **inpmw, **outpmw;

  MagickWandGenesis();
  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:none");
  MagickSetFormat(mw,"RGB");
  MagickSetDepth(mw,16);
  MagickSetType(mw,TrueColorType);
  //  UndefinedInterpolatePixel, AverageInterpolatePixel, BicubicInterpolatePixel, BilinearInterpolatePixel,
  //  FilterInterpolatePixel, IntegerInterpolatePixel, MeshInterpolatePixel, NearestNeighborInterpolatePixel,
  //  SplineInterpolatePixel

  inmw = NewPixelIterator(mw);
  for (long unsigned int Row=0; Row < Height; Row++) {
   inpmw = PixelGetNextIteratorRow(inmw, &Width);
    for (long unsigned int Col=0; Col < Width; Col++) {
      PixelSetRed(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][0]]);
      PixelSetGreen(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][1]]);
      PixelSetBlue(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][2]]);
    }
   PixelSyncIterator(inmw);
  }

  MagickContrastStretchImage(mw, 0.02,0.01);// BlackPoint, WhitePoint);

  outmw = NewPixelIterator(mw);
  for (long unsigned int Row=0; Row < Height; Row++) {

  outpmw = PixelGetNextIteratorRow(outmw, &Width);
    for (long unsigned int Col=0; Col < Width; Col++) {
    Image->m_Image[Row*Width+Col][0] = CLIP((int32_t)
                                            ((1-Opacity) * Image->m_Image[Row*Width+Col][0]
                                             + Opacity*PixelGetRed(outpmw[Col])*WP));
    Image->m_Image[Row*Width+Col][1] = CLIP((int32_t)
                                            ((1-Opacity) * Image->m_Image[Row*Width+Col][1]
                                             + Opacity*PixelGetGreen(outpmw[Col])*WP));
    Image->m_Image[Row*Width+Col][2] = CLIP((int32_t)
                                            ((1-Opacity) * Image->m_Image[Row*Width+Col][2]
                                             + Opacity*PixelGetBlue(outpmw[Col])*WP));
    }
  }

  inmw = DestroyPixelIterator(inmw);
  outmw = DestroyPixelIterator(outmw);
  mw = DestroyMagickWand(mw);
  MagickWandTerminus();
}
*/
/*

void IM_C_WriteImage(ptImage* Image,
         const char* FileName,
         const short Format,
         const int Quality,
         const int Sampling,
         const char* ColorProfileFileName,
         const int Intent) {

  long unsigned int Width  = Image->m_Width;
  long unsigned int Height = Image->m_Height;

  double WP = 0xFFFF;
  double invWP = 1/WP;

  MagickWand *mw;
  PixelIterator *inmw;
  PixelWand **inpmw;

  MagickWandGenesis();
  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:none");
  MagickSetFormat(mw,"RGB");
  MagickSetImageDepth(mw,16);
  MagickSetType(mw,TrueColorType);
  //  UndefinedInterpolatePixel, AverageInterpolatePixel, BicubicInterpolatePixel, BilinearInterpolatePixel,
  //  FilterInterpolatePixel, IntegerInterpolatePixel, MeshInterpolatePixel, NearestNeighborInterpolatePixel,
  //  SplineInterpolatePixel

  inmw = NewPixelIterator(mw);
  for (long unsigned int Row=0; Row < Height; Row++) {
    inpmw = PixelGetNextIteratorRow(inmw, &Width);
    for (long unsigned int Col=0; Col < Width; Col++) {
      PixelSetRed(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][0]]);
      PixelSetGreen(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][1]]);
      PixelSetBlue(inpmw[Col],ToFloatTable[Image->m_Image[Row*Width+Col][2]]);
    }
    PixelSyncIterator(inmw);
  }

  // Color profile
  // Color profile in PNG is not supported, due to limitations in imagemagick
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

  double factors[3];
  if (Format==ptSaveFormat_JPEG) {
    if (Sampling==ptSaveSampling_111) {
      factors={1,1,1};
    } else {
      factors={2,1,1};
    }
    MagickSetSamplingFactors(mw,3,factors);
  }

  // Quality
  if (Format==ptSaveFormat_PNG)
    MagickSetImageCompressionQuality(mw, floor((double) Quality/10.0)*10+5);
  else
    MagickSetImageCompressionQuality(mw,Quality);

  // Compression
  if ((Format==ptSaveFormat_TIFF8) ||
      (Format==ptSaveFormat_TIFF16)) {
    MagickSetImageCompression(mw, LZWCompression);
  }

  MagickWriteImage(mw, FileName);
  inmw = DestroyPixelIterator(inmw);
  mw = DestroyMagickWand(mw);
  MagickWandTerminus();
}
*/
/*
int IM_C_OpenImage(ptImage* Image,
                   const char* FileName,
                   const short ColorSpace,
                   uint16_t& InputWidth,
                   uint16_t& InputHeight,
                   const short Identify,
                   const short ScaleFactor,
                   const short IsLinear) {

  MagickWand *mw;

  MagickBooleanType status;

  MagickWandGenesis();
  mw = NewMagickWand();
  if (Identify)
    status = MagickPingImage(mw,FileName);
  else
    status = MagickReadImage(mw,FileName);
  if (status == MagickFalse) {
    mw = DestroyMagickWand(mw);
    MagickWandTerminus();
    return 1;
  }
  InputWidth = MagickGetImageWidth(mw);
  InputHeight = MagickGetImageHeight(mw);
  if (Identify) {
    mw = DestroyMagickWand(mw);
    MagickWandTerminus();
    return 0;
  }

  PixelIterator *outmw;
  PixelWand **outpmw;

  // Get the embedded profile
  size_t ProfileLength = 0;
  MagickGetImageProfile(mw,"ICC",&ProfileLength);

  uint8_t* ProfileBuffer = (uint8_t*) CALLOC(ProfileLength,sizeof(uint8_t));
  ptMemoryError(ProfileBuffer,__FILE__,__LINE__);

  ProfileBuffer = MagickGetImageProfile(mw,"ICC",&ProfileLength);

  cmsHPROFILE InProfile = NULL;
  InProfile = cmsOpenProfileFromMem(ProfileBuffer, ProfileLength);
  if (!InProfile) {
    InProfile = cmsCreate_sRGBProfile();
  }

  FREE (ProfileBuffer);

  // Paste the image data
  long unsigned int NewWidth = MagickGetImageWidth(mw);
  long unsigned int NewHeight = MagickGetImageHeight(mw);

  // the next hast to be double for lcms
  double (*ImageBuffer)[3] =
    (double (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*ImageBuffer));
  ptMemoryError(ImageBuffer,__FILE__,__LINE__);

  outmw = NewPixelIterator(mw);
  for (long unsigned int Row=0; Row < NewHeight; Row++) {
    outpmw = PixelGetNextIteratorRow(outmw, &NewWidth);
    for (long unsigned int Col=0; Col < NewWidth; Col++) {
      ImageBuffer[Row*NewWidth+Col][0] = PixelGetRed(outpmw[Col]);
      ImageBuffer[Row*NewWidth+Col][1] = PixelGetGreen(outpmw[Col]);
      ImageBuffer[Row*NewWidth+Col][2] = PixelGetBlue(outpmw[Col]);
    }
  }
  outmw = DestroyPixelIterator(outmw);

  for (short Loop=0; Loop<ScaleFactor; Loop++) {
    for (uint16_t Row=0; Row < NewHeight/2; Row++) {
      for (uint16_t Col=0; Col < NewWidth/2; Col++) {
        uint32_t Target = Row*(NewWidth/2)+Col;
        uint32_t Pixel1 = Row*2*NewWidth+Col*2;
        uint32_t Pixel2 = ((Col*2) < (NewWidth-1) )? Pixel1+1       : Pixel1;
        uint32_t Pixel3 = ((Row*2) < (NewHeight-1))? Pixel1+NewWidth : Pixel1;
        uint32_t Pixel4 = ((Col*2) < (NewWidth-1) )? Pixel3+1       : Pixel3;
        for (short c=0; c < 3; c++) {
          float  PixelValue = ImageBuffer[Pixel1][c];
          PixelValue += ImageBuffer[Pixel2][c];
          PixelValue += ImageBuffer[Pixel3][c];
          PixelValue += ImageBuffer[Pixel4][c];
          PixelValue /= 4; // is averaging over the four.
          ImageBuffer[Target][c] = PixelValue;
        }
      }
    }
    NewHeight    >>= 1;
    NewWidth     >>= 1;
  }

  FREE(Image->m_Image);
  Image->m_Width  = NewWidth;
  Image->m_Height = NewHeight;
  Image->m_Colors = 3;
  Image->m_ColorSpace = ColorSpace;
  Image->m_Image = (uint16_t (*)[3]) CALLOC(Image->m_Width*Image->m_Height,sizeof(*Image->m_Image));
  ptMemoryError(Image->m_Image,__FILE__,__LINE__);

  if (!IsLinear) {
    // Transform to linear representation
    LPGAMMATABLE    Gamma10[3];
    Gamma10[0] = Gamma10[1] = Gamma10[2] = cmsBuildGamma(256, 1.0);

    cmsHPROFILE OutProfile = 0;

    cmsCIExyY       D65;
    cmsCIExyY       D50;
    cmsWhitePointFromTemp(6503, &D65);
    cmsWhitePointFromTemp(5003, &D50);
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
                                     Gamma10);

    if (!OutProfile) {
      ptLogError(ptError_Profile,"Could not generate linear output profile.");
      return NULL;
    }

    cmsFreeGamma(Gamma10[0]);

    cmsHTRANSFORM Transform;
    Transform = cmsCreateTransform(InProfile,
                                   TYPE_RGB_DBL,
                                   OutProfile,
                                   TYPE_RGB_16,
                                   INTENT_PERCEPTUAL,
                                   cmsFLAGS_NOTPRECALC);

    cmsDoTransform(Transform,ImageBuffer,Image->m_Image,Image->m_Width*Image->m_Height);
    cmsDeleteTransform(Transform);
    cmsCloseProfile(InProfile);
    cmsCloseProfile(OutProfile);
  } else {
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row < NewHeight; Row++) {
      for (uint16_t Col=0; Col<NewWidth; Col++) {
        for (short c=0; c<3; c++) {
          Image->m_Image[Row*NewWidth+Col][c] = CLIP((int32_t) (ImageBuffer[Row*NewWidth+Col][c]*0xffff));
        }
      }
    }
  }
  FREE(ImageBuffer);

  //~ // Write test
  //~ double WP = 0xFFFF;
  //~ double invWP = 1/WP;
  //~ MagickWand *mw1;
  //~ mw1 = NewMagickWand();
  //~ PixelIterator *inmw;
  //~ PixelWand **inpmw;
//~
  //~ MagickSetSize(mw1,NewWidth, NewHeight);
  //~ MagickReadImage(mw1,"xc:none");
  //~ MagickSetFormat(mw1,"RGB");
  //~ MagickSetImageDepth(mw1,16);
  //~ MagickSetType(mw1,TrueColorType);
//~
  //~ inmw = NewPixelIterator(mw1);
  //~ for (long unsigned int Row=0; Row < NewHeight; Row++) {
  //~ inpmw = PixelGetNextIteratorRow(inmw, &NewWidth);
    //~ for (long unsigned int Col=0; Col < NewWidth; Col++) {
      //~ PixelSetRed(inpmw[Col],invWP * (double)Image->m_Image[Row*NewWidth+Col][0]);
      //~ PixelSetGreen(inpmw[Col],invWP * (double)Image->m_Image[Row*NewWidth+Col][1]);
      //~ PixelSetBlue(inpmw[Col],invWP * (double)Image->m_Image[Row*NewWidth+Col][2]);
    //~ }
  //~ PixelSyncIterator(inmw);
  //~ }
//~
  //~ MagickWriteImage(mw1, "test.jpg");
  //~ inmw = DestroyPixelIterator(inmw);

  // Close what is left
  mw = DestroyMagickWand(mw);
  MagickWandTerminus();

  return 0;
}
*/
