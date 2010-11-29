////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include <QMessageBox>
#include "ptImage.h"
#include "ptConstants.h"
#include "ptError.h"
#include "ptSettings.h"

#include <Magick++.h>

using namespace Magick;

#ifdef _OPENMP
  #include <omp.h>
#endif

#include <lcms2.h>

// Lut
extern float ToFloatTable[0x10000];

// Rotation
ptImage* ptImage::ptGMRotate(const double Angle) {

  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,m_Image);

  image.backgroundColor(Magick::ColorRGB(0,0,0));
  image.rotate(Angle);
  uint16_t NewWidth = image.columns();
  uint16_t NewHeight = image.rows();

  FREE(m_Image);
  m_Width  = NewWidth;
  m_Height = NewHeight;
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  image.write(0,0,NewWidth,NewHeight,"RGB",ShortPixel,m_Image);

  return this;
}

// Identify
int ptImage::ptGMIdentify(const char* FileName,
                          uint16_t& InputWidth,
                          uint16_t& InputHeight) {
  try {
    Magick::Image image;

    image.ping(FileName);

    InputWidth = image.columns();
    InputHeight = image.rows();
  } catch (Exception &Error) {
    return 1;
  }

  return 0;
}

// Open Image
ptImage* ptImage::ptGMOpenImage(const char* FileName,
                                const short ColorSpace,
                                const short Intent,
                                const short ScaleFactor,
                                int& Success) {

  Magick::Image image;
  try {
    image.read(FileName);
  } catch (Exception &Error) {
    return this;
  }
  Success = 1;

  // Get the embedded profile
  cmsHPROFILE InProfile = NULL;

  if (image.type() != GrayscaleType) {
    Magick::Blob Profile = image.iccColorProfile();

    if (Profile.length() > 0) {
      InProfile = cmsOpenProfileFromMem(const_cast<void*>(Profile.data()), Profile.length());
    }
  }
  if (!InProfile) {
    InProfile = cmsCreate_sRGBProfile();
  }

  image.type(TrueColorType);
  image.magick( "RGB" );
  image.depth( 16 );

  uint16_t NewWidth = image.columns();
  uint16_t NewHeight = image.rows();

  float (*ImageBuffer)[3] =
    (float (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*ImageBuffer));
  ptMemoryError(ImageBuffer,__FILE__,__LINE__);

  image.write(0,0,NewWidth,NewHeight,"RGB",FloatPixel,ImageBuffer);
  m_Width = NewWidth;

  NewHeight >>= ScaleFactor;
  NewWidth >>= ScaleFactor;

  short Step = 1 << ScaleFactor;
  int Average = pow(2,2 * ScaleFactor);

  float (*NewImage)[3] =
    (float (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*NewImage));
  ptMemoryError(NewImage,__FILE__,__LINE__);

#pragma omp parallel for schedule(static)
  for (uint16_t Row=0; Row < NewHeight*Step; Row+=Step) {
    for (uint16_t Col=0; Col < NewWidth*Step; Col+=Step) {
      float  PixelValue[3] = {0.0,0.0,0.0};
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
  FREE(ImageBuffer);

  FREE(m_Image);
  m_Width  = NewWidth;
  m_Height = NewHeight;
  m_Colors = 3;
  m_ColorSpace = ColorSpace;
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

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
      uint16_t* Image = &m_Image[i][0];
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
  FREE(NewImage);

  return this;
}

// Write Image
ptImage* ptImage::ptGMWriteImage(const char* FileName,
                                 const short Format,
                                 const int Quality,
                                 const char* ColorProfileFileName,
                                 const int Intent) {

  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,m_Image);

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
  Magick::Blob ICCProfile(pchBuffer, lSize);
  image.iccColorProfile(ICCProfile);

  FCLOSE (pFile);
  FREE (pchBuffer);

  // Redering intent
  switch(Intent) {
    case 0:
      image.renderingIntent(PerceptualIntent);
      break;
    case 1:
      image.renderingIntent(RelativeIntent);
      break;
    case 2:
      image.renderingIntent(SaturationIntent);
      break;
    case 3:
      image.renderingIntent(AbsoluteIntent);
      break;
    default:
      assert(0);
  }

  // Depth
  if ((Format==ptSaveFormat_TIFF8) ||
      (Format==ptSaveFormat_PPM8) ||
      (Format==ptSaveFormat_JPEG) ||
      (Format==ptSaveFormat_PNG)) image.depth( 8 );

  // Quality
  if (Format==ptSaveFormat_PNG) image.quality(floor((double) Quality/10.0)*10+5);
  else image.quality(Quality);

  // Compression
  if ((Format==ptSaveFormat_TIFF8) ||
      (Format==ptSaveFormat_TIFF16)) image.compressType(LZWCompression);

  image.write(FileName);
  return this;
}

// Simple Open
ptImage* ptImage::ptGMSimpleOpen(const char* FileName) {

  Magick::Image image;
  try {
    image.read(FileName);
  } catch (Exception &Error) {
    return this;
  }

  image.type(TrueColorType);
  image.magick( "RGB" );
  image.depth( 16 );

  image.modifyImage();

  uint16_t NewWidth = image.columns();
  uint16_t NewHeight = image.rows();

  FREE(m_Image);
  m_Width  = NewWidth;
  m_Height = NewHeight;
  m_Colors = 3;
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  image.write(0,0,NewWidth,NewHeight,"RGB",ShortPixel,m_Image);

  return this;
}

// Resize
ptImage* ptImage::ptGMResize(uint16_t Size, const short Filter) {

  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,m_Image);
  FREE(m_Image);

  switch (Filter) {
    case ptIMFilter_Point:
      image.filterType(PointFilter);
      break;
    case ptIMFilter_Box:
      image.filterType(BoxFilter);
      break;
    case ptIMFilter_Triangle:
      image.filterType(TriangleFilter);
      break;
    case ptIMFilter_Hermite:
      image.filterType(HermiteFilter);
      break;
    case ptIMFilter_Hanning:
      image.filterType(HanningFilter);
      break;
    case ptIMFilter_Hamming:
      image.filterType(HammingFilter);
      break;
    case ptIMFilter_Blackman:
      image.filterType(BlackmanFilter);
      break;
    case ptIMFilter_Gaussian:
      image.filterType(GaussianFilter);
      break;
    case ptIMFilter_Quadratic:
      image.filterType(QuadraticFilter);
      break;
    case ptIMFilter_Cubic:
      image.filterType(CubicFilter);
      break;
    case ptIMFilter_Catrom:
      image.filterType(CatromFilter);
      break;
    case ptIMFilter_Mitchell:
      image.filterType(MitchellFilter);
      break;
    case ptIMFilter_Lanczos:
      image.filterType(LanczosFilter);
      break;
    //~ case ptIMFilter_Bessel:
      //~ image.filterType(BesselFilter);
      //~ break;
    //~ case ptIMFilter_Sinc:
      //~ image.filterType(SincFilter);
      //~ break;
    default:
      assert(0);
  }

  QString TempString = QString::number(Size);
  TempString += "x";

  // Leftover from ImageMagick
  // if (Width <= Height)  {
    // TempString.prepend("x");
  // }

  image.zoom(TempString.toStdString());
  image.modifyImage();

  m_Width  = image.columns();
  m_Height = image.rows();
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  image.write(0,0,image.columns(),image.rows(),"RGB",ShortPixel,m_Image);
  return this;
}

// Blur
ptImage* ptImage::ptGMBlur(const double Radius) {

  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,m_Image);

  image.blur(0,Radius);

  image.write(0,0,Width,Height,"RGB",ShortPixel,m_Image);
  return this;
}

// Unsharp Mask
ptImage* ptImage::ptGMUnsharp(const double Radius, const double Amount, const double Threshold) {

  assert (m_ColorSpace == ptSpace_Lab);

  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,m_Image);

  double Sigma = Radius;
  image.unsharpmaskChannel(RedChannel,0,Sigma,Amount,Threshold);
  image.modifyImage();

  image.write(0,0,Width,Height,"RGB",ShortPixel,m_Image);
  return this;
}

// Normalize
ptImage* ptImage::ptGMNormalize(const double Opacity) {

  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;

  double WP = 0xFFFF;

  Magick::Image image(Width,Height,"RGB",ShortPixel,m_Image);

  image.modifyImage();

  const Magick::PixelPacket *Pixels2;
  Magick::ColorRGB Color;
  image.normalize();

  image.modifyImage();

#pragma omp parallel for default(shared) private(Pixels2, Color)
  for (uint16_t Row=0; Row < Height; Row++) {
    Pixels2 = image.getConstPixels(0, Row, Width, 1);
    for (uint16_t Col=0; Col < Width; Col++) {
    Color = (*(Pixels2 + Col));
    m_Image[Row*Width+Col][0] = CLIP((int32_t)
                                    ((1-Opacity)*m_Image[Row*Width+Col][0]
                                     + Opacity*Color.red()*WP));
    m_Image[Row*Width+Col][1] = CLIP((int32_t)
                                    ((1-Opacity)*m_Image[Row*Width+Col][1]
                                     + Opacity*Color.green()*WP));
    m_Image[Row*Width+Col][2] = CLIP((int32_t)
                                    ((1-Opacity)*m_Image[Row*Width+Col][2]
                                     + Opacity*Color.blue()*WP));
    }
  }
  return this;
}
