/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
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

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptDcRaw.h"
#include "../ptImage8.h"
#include "ptFileMgrDM.h"
#include "ptFileMgrConstants.h"

extern ptSettings* Settings;

//==============================================================================

ptFileMgrDM* ptFileMgrDM::m_Instance = NULL;

//==============================================================================

/*static*/
ptFileMgrDM* ptFileMgrDM::GetInstance() {
  if (m_Instance == NULL) {
    m_Instance = new ptFileMgrDM();
  }

  return m_Instance;
}

//==============================================================================

/*static*/
void ptFileMgrDM::DestroyInstance() {
  delete m_Instance;
  m_Instance = NULL;
}

//==============================================================================

ptFileMgrDM::ptFileMgrDM() :
  QObject(),
  FThumbDM(nullptr) // init of unique_ptr!
{
  m_DirModel    = new ptSingleDirModel;
  m_TagModel    = new ptTagModel;

  // Init stuff for thumbnail generation
  m_ThumbList   = new QList<ptGraphicsThumbGroup*>;
  m_Thumbnailer = new ptThumbnailer();
  m_Thumbnailer->setThumbList(m_ThumbList);

  // Init thumbnail cache
  m_Cache = new ptThumbnailCache(1000);
  m_Thumbnailer->setCache(m_Cache);

  m_FocusedThumb = -1;
}

//==============================================================================

ptFileMgrDM::~ptFileMgrDM() {
  StopThumbnailer();
  DelAndNull(m_ThumbList);
  DelAndNull(m_Thumbnailer);
  DelAndNull(m_Cache);
  DelAndNull(m_DirModel);
  DelAndNull(m_TagModel);
}

//==============================================================================

void ptFileMgrDM::Clear() {
  StopThumbnailer();
  m_Cache->Clear();
}

//==============================================================================

int ptFileMgrDM::setThumbnailDir(const QString path) {
  m_CurrentDir = path;
  return m_Thumbnailer->setDir(path);
}

//==============================================================================

void ptFileMgrDM::StartThumbnailer() {
#ifdef Q_OS_MAC
  m_Thumbnailer->run();
#else
  m_Thumbnailer->start();
#endif
}

//==============================================================================

void ptFileMgrDM::StopThumbnailer() {
  m_Thumbnailer->blockSignals(true);
  m_Thumbnailer->Abort();
  m_Thumbnailer->blockSignals(false);
}

//==============================================================================

bool ptFileMgrDM::getThumbnail(ptImage8     *&AImage,
                               const QString &AFileName,
                               const int      AMaxSize) {

  assert(!AFileName.isEmpty());

  ptDcRaw dcRaw;
  bool isRaw = false;
  MagickWand* image = NewMagickWand();
  QSize Size = QSize(AMaxSize, AMaxSize);

  if (dcRaw.Identify(AFileName) == 0 ) {
    // we have a raw image
    isRaw = true;
    std::vector<char> ImgData;
    if (dcRaw.thumbnail(ImgData)) {
      // raw thumbnail read successfully
      Size.setWidth(dcRaw.m_ThumbWidth);
      Size.setHeight(dcRaw.m_ThumbHeight);
      ScaleThumbSize(&Size, AMaxSize);
      MagickSetSize(image, 2*Size.width(), 2*Size.height());
      MagickReadImageBlob(image, (const uchar*)ImgData.data(), (const size_t)ImgData.size());
    }
  }

  if (!isRaw) {
    // no raw, try for bitmap
    MagickPingImage(image, AFileName.toAscii().data());
    Size.setWidth(MagickGetImageWidth(image));
    Size.setHeight(MagickGetImageHeight(image));
    ScaleThumbSize(&Size, AMaxSize);
    MagickSetSize(image, 2*Size.width(), 2*Size.height());
    MagickReadImage(image, AFileName.toAscii().data());
  }

  ExceptionType MagickExcept;
  char* MagickErrMsg = MagickGetException(image, &MagickExcept);
  if (MagickExcept != UndefinedException) {
    // error occurred: no raw thumbnail, no supported image type, any other GM error
    printf("%s\n", QString::fromAscii(MagickErrMsg).toAscii().data());
    DestroyMagickWand(image);
    if (!AImage) AImage = new ptImage8();
    AImage->FromQImage(QImage(QString::fromUtf8(":/dark/icons/broken-image-48px.png")));
    return false;

  } else {
    // no error: scale and rotate thumbnail
    if (!AImage) AImage = new ptImage8();
    GenerateThumbnail(image, AImage, Size);
    DestroyMagickWand(image);
    return true;
  }
}

//==============================================================================

ptThumbDM *ptFileMgrDM::getThumbDM()
{
  if(!FThumbDM) {
    FThumbDM = make_unique<ptThumbDM>();
  }

  return FThumbDM.get();
}

//==============================================================================

void ptFileMgrDM::GenerateThumbnail(MagickWand* AInImage, ptImage8 *AOutImage, const QSize tSize)
{
  // We want 8bit RGB data without alpha channel, scaled to thumbnail size
  MagickSetImageDepth(AInImage, 8);
  MagickSetImageFormat(AInImage, "RGB");
  MagickSetImageType(AInImage, TrueColorType);
  // 0 is used for escaping the scaling
  if (tSize.width() > 0 && tSize.height() > 0)
    MagickScaleImage(AInImage, tSize.width(), tSize.height());

  // read EXIF orientation and correct image
  int orientation = QString::fromAscii(MagickGetImageAttribute(AInImage, "EXIF:Orientation")).toInt();
  PixelWand* pxWand = NewPixelWand();
  switch (orientation) {
    case 2: MagickFlopImage(AInImage); break;
    case 3: MagickRotateImage(AInImage, pxWand, 180); break;
    case 4: MagickFlipImage(AInImage); break;
    case 5: MagickFlopImage(AInImage); MagickRotateImage(AInImage, pxWand, 270); break;
    case 6: MagickRotateImage(AInImage, pxWand, 90); break;
    case 7: MagickFlipImage(AInImage); MagickRotateImage(AInImage, pxWand, 270); break;
    case 8: MagickRotateImage(AInImage, pxWand, 270); break;
    default: break;
  }
  DestroyPixelWand(pxWand);

  // Get the raw image data from GM.
  uint w = MagickGetImageWidth(AInImage);
  uint h = MagickGetImageHeight(AInImage);

  AOutImage->setSize(w, h, 3);

  MagickGetImagePixels(AInImage, 0, 0, w, h, "BGRA", CharPixel, (uchar*)(AOutImage->m_Image));
}

//==============================================================================

void ptFileMgrDM::ScaleThumbSize(QSize* tSize, const int max) {
  if (tSize->width() < max && tSize->height() < max) return;

  if (tSize->width() == tSize->height()) {    // square image
    tSize->setWidth(max);
    tSize->setHeight(max);
  } else if (tSize->width() > tSize->height()) {    // landscape image
    tSize->setHeight(tSize->height()/(double)tSize->width() * max + 0.5);
    tSize->setWidth(max);
  } else if (tSize->width() < tSize->height()) {    // portrait image
    tSize->setWidth(tSize->width()/(double)tSize->height() * max + 0.5);
    tSize->setHeight(max);
  }
}

//==============================================================================

ptGraphicsThumbGroup* ptFileMgrDM::MoveFocus(const int index) {
  m_FocusedThumb = index;
  return m_ThumbList->at(index);
}

//==============================================================================

int ptFileMgrDM::focusedThumb(QGraphicsItem* group) {
  for (int i = 0; i < m_ThumbList->count(); i++) {
    if (m_ThumbList->at(i) == group) {
      m_FocusedThumb = i;
      return i;
    }
  }
  m_FocusedThumb = -1;
  return -1;
}

//==============================================================================
