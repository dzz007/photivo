////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>)
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

#include <QString>
#include <QMessageBox>

#include "ptImageMagick.h"
#include "ptConstants.h"
#include "ptError.h"
#include "ptSettings.h"

using namespace Magick;

// Lut
extern float ToFloatTable[0x10000];
/*
void ptIMResize(ptImage* Image, uint16_t Size, const short Filter) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,Image->m_Image);
  FREE(Image->m_Image);

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
  if (Width <= Height)  {
    TempString.prepend("x");
  }

  image.zoom(TempString.toStdString());
  image.modifyImage();

  Image->m_Width  = image.columns();
  Image->m_Height = image.rows();
  Image->m_Image = (uint16_t (*)[3]) CALLOC(Image->m_Width*Image->m_Height,sizeof(*Image->m_Image));
  ptMemoryError(Image->m_Image,__FILE__,__LINE__);

  image.write(0,0,image.columns(),image.rows(),"RGB",ShortPixel,Image->m_Image);
}
*/

/*
void ptIMRotate(ptImage* Image, const double Angle) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  double WP = 0xFFFF;
  double invWP = 1/WP;

  Magick::Image image;
  image.size(Geometry(Width, Height));
  image.type(TrueColorType);
  image.magick( "RGB" );
  image.depth( 16 );

  image.modifyImage();

  Magick::PixelPacket *Pixels;
  const Magick::PixelPacket *Pixels2;
  Magick::ColorRGB Color;

  for (uint16_t Row=0; Row < Height; Row++) {
    Pixels = image.setPixels(0, Row, Width, 1);
    for (uint16_t Col=0; Col < Width; Col++) {
      Color.red(ToFloatTable[Image->m_Image[Row*Width+Col][0]]);
      Color.green(ToFloatTable[Image->m_Image[Row*Width+Col][1]]);
      Color.blue(ToFloatTable[Image->m_Image[Row*Width+Col][2]]);
      *Pixels++ = Color;
    }
    image.syncPixels();
  }

  image.modifyImage();
  image.virtualPixelMethod(BlackVirtualPixelMethod);
  image.distort(ScaleRotateTranslateDistortion,1,&Angle,1);
  uint16_t NewWidth = image.columns();
  uint16_t NewHeight = image.rows();
  image.modifyImage();

  FREE(Image->m_Image);
  Image->m_Width  = NewWidth;
  Image->m_Height = NewHeight;
  Image->m_Image = (uint16_t (*)[3]) CALLOC(Image->m_Width*Image->m_Height,sizeof(*Image->m_Image));
  ptMemoryError(Image->m_Image,__FILE__,__LINE__);

  for (uint16_t Row=0; Row < NewHeight; Row++) {
    Pixels2 = image.getConstPixels(0, Row, NewWidth, 1);
    for (uint16_t Col=0; Col < NewWidth; Col++) {
      Color = (*(Pixels2 + Col));
      Image->m_Image[Row*NewWidth+Col][0] = CLIP((int32_t) (Color.red()*WP));
      Image->m_Image[Row*NewWidth+Col][1] = CLIP((int32_t) (Color.green()*WP));
      Image->m_Image[Row*NewWidth+Col][2] = CLIP((int32_t) (Color.blue()*WP));
    }
  }

}
*/

/*
void ptIMReadJpg(ptImage* Image, const char* FileName) {

  double WP = 0xFFFF;

  Magick::Image image;
  const Magick::PixelPacket *Pixels2;
  Magick::ColorRGB Color;

  image.read(FileName);
  image.type(TrueColorType);
  image.magick( "RGB" );
  image.depth( 16 );

  image.modifyImage();

  uint16_t NewWidth = image.columns();
  uint16_t NewHeight = image.rows();
  image.modifyImage();

  FREE(Image->m_Image);
  Image->m_Width  = NewWidth;
  Image->m_Height = NewHeight;
  Image->m_Image = (uint16_t (*)[3]) CALLOC(Image->m_Width*Image->m_Height,sizeof(*Image->m_Image));
  ptMemoryError(Image->m_Image,__FILE__,__LINE__);

  for (uint16_t Row=0; Row < NewHeight; Row++) {
    Pixels2 = image.getConstPixels(0, Row, NewWidth, 1);
    for (uint16_t Col=0; Col < NewWidth; Col++) {
      Color = (*(Pixels2 + Col));
      Image->m_Image[Row*NewWidth+Col][0] = CLIP((int32_t) (Color.red()*WP));
      Image->m_Image[Row*NewWidth+Col][1] = CLIP((int32_t) (Color.green()*WP));
     Image->m_Image[Row*NewWidth+Col][2] = CLIP((int32_t) (Color.blue()*WP));
    }
  }

}
*/

/*
void ptIMUnsharp(ptImage* Image, const double Radius, const double Amount, const double Threshold) {

  assert (Image->m_ColorSpace == ptSpace_Lab);

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,Image->m_Image);

  double Sigma = Radius;
  image.unsharpmaskChannel(RedChannel,0,Sigma,Amount,Threshold);
  image.modifyImage();

  image.write(0,0,Width,Height,"RGB",ShortPixel,Image->m_Image);
}
*/

/*
void ptIMBlur(ptImage* Image, const double Radius) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,Image->m_Image);

  image.blur(Radius);
  image.modifyImage();

  image.write(0,0,Width,Height,"RGB",ShortPixel,Image->m_Image);
}
*/

/*
void ptIMNormalize(ptImage* Image, const double Opacity) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  double WP = 0xFFFF;
  double invWP = 1/WP;

  Magick::Image image;
  image.size(Geometry(Width, Height));
  image.type(TrueColorType);
  image.magick( "RGB" );
  image.depth( 16 );

  image.modifyImage();

  Magick::PixelPacket *Pixels;
  const Magick::PixelPacket *Pixels2;
  Magick::ColorRGB Color;
#pragma omp parallel for default(shared) private(Pixels, Color)
  for (uint16_t Row=0; Row < Height; Row++) {
  Pixels = image.setPixels(0, Row, Width, 1);
    for (uint16_t Col=0; Col < Width; Col++) {
      Color.red(invWP * Image->m_Image[Row*Width+Col][0]);
      Color.green(invWP * Image->m_Image[Row*Width+Col][1]);
    Color.blue(invWP * Image->m_Image[Row*Width+Col][2]);
    *Pixels++ = Color;
    }
    image.syncPixels();
  }

  image.modifyImage();
  image.normalize();

  image.modifyImage();

#pragma omp parallel for default(shared) private(Pixels2, Color)
  for (uint16_t Row=0; Row < Height; Row++) {
    Pixels2 = image.getConstPixels(0, Row, Width, 1);
    for (uint16_t Col=0; Col < Width; Col++) {
    Color = (*(Pixels2 + Col));
    Image->m_Image[Row*Width+Col][0] = CLIP((int32_t)
                                            ((1-Opacity)*Image->m_Image[Row*Width+Col][0]
                                             + Opacity*Color.red()*WP));
    Image->m_Image[Row*Width+Col][1] = CLIP((int32_t)
                                            ((1-Opacity)*Image->m_Image[Row*Width+Col][1]
                                             + Opacity*Color.green()*WP));
    Image->m_Image[Row*Width+Col][2] = CLIP((int32_t)
                                            ((1-Opacity)*Image->m_Image[Row*Width+Col][2]
                                             + Opacity*Color.blue()*WP));
    }
  }

}
*/

/*
void IM_WriteImage(ptImage* Image,
                   const char* FileName,
                   const short Format,
                   const int Quality,
                   const char* ColorProfileFileName,
                   const int Intent) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  Magick::Image image(Width,Height,"RGB",ShortPixel,Image->m_Image);

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
  image.profile("icc", ICCProfile);

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
}
*/
