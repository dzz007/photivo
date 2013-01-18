/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Michael Munzert <mail@mm-log.com>
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

#include <chrono>

#include "../ptInfo.h"
#include "../ptLock.h"
#include "../ptImage8.h"
#include "../ptDcRaw.h"
#include "ptThumbGen.h"

//==============================================================================

ptThumbGen::ptThumbGen() :
  QThread(),
  FCurrentThumb(),
  FThumbnail(nullptr)  // init of shared_ptr!
{
}

//==============================================================================

ptThumbGen::~ptThumbGen()
{
}

//==============================================================================

void ptThumbGen::setCurrentThumb(const ptThumbId AThumbId)
{
  FCurrentThumb = AThumbId;
}

//==============================================================================

ptThumbId ptThumbGen::getCurrentThumb() const
{
  return FCurrentThumb;
}

//==============================================================================

ptThumbPtr ptThumbGen::getThumbnail() const
{
  return FThumbnail;
}

//==============================================================================
// forwarding...
void ptThumbGen::run()
{
  generateThumb(FCurrentThumb);
}

//==============================================================================

void ptThumbGen::generateThumb(const ptThumbId AThumbId)
{
  const QString hFileName = AThumbId.FileName; //"d:\photivoLogo.png";

  ptDcRaw     dcRaw;
  MagickWand* image = NewMagickWand();
  QSize       hSize = QSize(AThumbId.MaxSize, AThumbId.MaxSize);

  if (dcRaw.Identify(hFileName) == 0 ) {
    // we have a raw image
    std::vector<char> ImgData;
    if (dcRaw.thumbnail(ImgData)) {
      // raw thumbnail read successfully
      hSize.setWidth(dcRaw.m_ThumbWidth);
      hSize.setHeight(dcRaw.m_ThumbHeight);
      ScaleThumbSize(hSize, AThumbId.MaxSize);
      MagickSetSize(image, 2*hSize.width(), 2*hSize.height());
      MagickReadImageBlob(image, (const uchar*)ImgData.data(), (const size_t)ImgData.size());
    }
  } else {
    // no raw, try for bitmap
    MagickPingImage(image, hFileName.toAscii().data());
    hSize.setWidth(MagickGetImageWidth(image));
    hSize.setHeight(MagickGetImageHeight(image));
    ScaleThumbSize(hSize, AThumbId.MaxSize);
    MagickSetSize(image, 2*hSize.width(), 2*hSize.height());
    MagickReadImage(image, hFileName.toAscii().data());
  }

  FThumbnail = std::make_shared<ptImage8>();

  ExceptionType MagickExcept;
  char* MagickErrMsg = MagickGetException(image, &MagickExcept);
  if (MagickExcept != UndefinedException) {
    // error occurred: no raw thumbnail, no supported image type, any other GM error
    printf("%s\n", QString::fromAscii(MagickErrMsg).toAscii().data());
    DestroyMagickWand(image);
    GInfo->Raise("Bähh");
    // FThumbnail.get()->FromQImage(QImage(QString::fromUtf8(":/dark/icons/broken-image-48px.png")));
    return;
  } else {
    // no error: scale and rotate thumbnail
    TransformImage(image, FThumbnail.get(), hSize);
    DestroyMagickWand(image);
    printf("Thread done \n");
    return;
  }
}

//==============================================================================

void ptThumbGen::ScaleThumbSize(QSize &ASize, const int AMax)
{
  if ((ASize.width()  < AMax) &&
      (ASize.height() < AMax)   )
    return;

  if (ASize.width() == ASize.height()) {          // square image
    ASize.setWidth(AMax);
    ASize.setHeight(AMax);
  } else if (ASize.width() > ASize.height()) {    // landscape image
    ASize.setHeight(ASize.height()/(double)ASize.width() * AMax + 0.5);
    ASize.setWidth(AMax);
  } else if (ASize.width() < ASize.height()) {    // portrait image
    ASize.setWidth(ASize.width()/(double)ASize.height() * AMax + 0.5);
    ASize.setHeight(AMax);
  }
}

//==============================================================================

void ptThumbGen::TransformImage(MagickWand *AInImage, ptImage8 *AOutImage, const QSize ASize)
{
  // We want 8bit RGB data without alpha channel, scaled to thumbnail size
  MagickSetImageDepth( AInImage, 8);
  MagickSetImageFormat(AInImage, "RGB");
  MagickSetImageType(  AInImage, TrueColorType);
  // 0 is used for escaping the scaling
  if (ASize.width() > 0 && ASize.height() > 0)
    MagickScaleImage(  AInImage, ASize.width(), ASize.height());

  // read EXIF orientation and correct image
  int orientation = QString::fromAscii(MagickGetImageAttribute(AInImage, "EXIF:Orientation")).toInt();
  PixelWand* pxWand = NewPixelWand();
  switch (orientation) {
    case 2: MagickFlopImage(  AInImage); break;
    case 3: MagickRotateImage(AInImage, pxWand, 180); break;
    case 4: MagickFlipImage(  AInImage); break;
    case 5: MagickFlopImage(  AInImage); MagickRotateImage(AInImage, pxWand, 270); break;
    case 6: MagickRotateImage(AInImage, pxWand, 90); break;
    case 7: MagickFlipImage(  AInImage); MagickRotateImage(AInImage, pxWand, 270); break;
    case 8: MagickRotateImage(AInImage, pxWand, 270); break;
    default: break;
  }
  DestroyPixelWand(pxWand);

  // Get the raw image data from GM.
  uint w = MagickGetImageWidth( AInImage);
  uint h = MagickGetImageHeight(AInImage);

  AOutImage->setSize(w, h, 3);

  MagickGetImagePixels(AInImage, 0, 0, w, h, "BGRA", CharPixel, (uchar*)(AOutImage->m_Image));
}

//==============================================================================
