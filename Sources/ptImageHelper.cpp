/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <exiv2/image.hpp>
#pragma GCC diagnostic pop
#include <wand/magick_wand.h>

#include <QStringList>

#include "ptDefines.h"
#include "ptCalloc.h"
#include "ptError.h"
#include "ptConstants.h"
#include "ptImageHelper.h"
#include "ptSettings.h"
#include "ptMessageBox.h"

//==============================================================================

const unsigned char CExifHeader[]    = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};
const unsigned int  CMaxHeaderLength = 65527;

//==============================================================================

void StringClean(QString& AString) {
  while (AString.contains("  "))
    AString.replace("  "," ");

  AString = AString.trimmed();
}

//==============================================================================

bool ptImageHelper::WriteExif(const QString AFileName, const Exiv2::ExifData AExifData) {
  try {
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
    Exiv2::ExifData hInExifData = AExifData;

    // Reset orientation
    Exiv2::ExifData::iterator pos = hInExifData.begin();
    if ((pos = hInExifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation"))) != hInExifData.end()) {
      pos->setValue("1"); // Normal orientation
    }

    // Adapted from UFRaw, necessary for Tiff files
    QStringList ExifKeys;
    ExifKeys  << "Exif.Image.ImageWidth"
              << "Exif.Image.ImageLength"
              << "Exif.Image.BitsPerSample"
              << "Exif.Image.Compression"
              << "Exif.Image.PhotometricInterpretation"
              << "Exif.Image.FillOrder"
              << "Exif.Image.SamplesPerPixel"
              << "Exif.Image.StripOffsets"
              << "Exif.Image.RowsPerStrip"
              << "Exif.Image.StripByteCounts"
              << "Exif.Image.XResolution"
              << "Exif.Image.YResolution"
              << "Exif.Image.PlanarConfiguration"
              << "Exif.Image.ResolutionUnit";

    for (short i = 0; i < ExifKeys.count(); i++) {
      if ((pos = hInExifData.findKey(Exiv2::ExifKey(ExifKeys.at(i).toAscii().data()))) != hInExifData.end())
        hInExifData.erase(pos);
    }

    if (Settings->GetInt("EraseExifThumbnail")) {
      Exiv2::ExifThumb Thumb(hInExifData);
      Thumb.erase();
    }

    QStringList JpegExtensions;
    JpegExtensions << "jpg" << "JPG" << "Jpg" << "jpeg" << "Jpeg" << "JPEG";
    bool deleteDNGdata = false;
    for (short i = 0; i < JpegExtensions.count(); i++) {
      if (!AFileName.endsWith(JpegExtensions.at(i))) deleteDNGdata = true;
    }

    Exiv2::Image::AutoPtr Exiv2Image = Exiv2::ImageFactory::open(AFileName.toAscii().data());

    Exiv2Image->readMetadata();
    Exiv2::ExifData outExifData = Exiv2Image->exifData();

    for (auto hPos = hInExifData.begin(); hPos != hInExifData.end(); hPos++) {
      if (!deleteDNGdata || (*hPos).key() != "Exif.Image.DNGPrivateData") {
        outExifData.add(*hPos);
      }
    }

    // IPTC data
    Exiv2::IptcData iptcData;

    QStringList Tags        = Settings->GetStringList("TagsList");
    QStringList DigikamTags = Settings->GetStringList("DigikamTagsList");

    Exiv2::StringValue StringValue;
    for (int i = 0; i < Tags.size(); i++) {
      StringValue.read(Tags.at(i).toStdString());
      iptcData.add(Exiv2::IptcKey("Iptc.Application2.Keywords"), &StringValue);
    }

    // XMP data
    Exiv2::XmpData xmpData;

    for (int i = 0; i < Tags.size(); i++) {
      xmpData["Xmp.dc.subject"] = Tags.at(i).toStdString();
    }
    for (int i = 0; i < DigikamTags.size(); i++) {
      xmpData["Xmp.digiKam.TagsList"] = DigikamTags.at(i).toStdString();
    }

    // Image rating
    outExifData["Exif.Image.Rating"] = Settings->GetInt("ImageRating");
    xmpData["Xmp.xmp.Rating"]        = Settings->GetInt("ImageRating");

    // Program name
    outExifData["Exif.Image.ProcessingSoftware"] = ProgramName;
    outExifData["Exif.Image.Software"]           = ProgramName;
    iptcData["Iptc.Application2.Program"]        = ProgramName;
    iptcData["Iptc.Application2.ProgramVersion"] = "idle";
    xmpData["Xmp.xmp.CreatorTool"]               = ProgramName;
    xmpData["Xmp.tiff.Software"]                 = ProgramName;

    // Title
    QString TitleWorking = Settings->GetString("ImageTitle");
    StringClean(TitleWorking);
    if (TitleWorking != "") {
      outExifData["Exif.Photo.UserComment"] = TitleWorking.toStdString();
      iptcData["Iptc.Application2.Caption"] = TitleWorking.toStdString();
      xmpData["Xmp.dc.description"]         = TitleWorking.toStdString();
      xmpData["Xmp.exif.UserComment"]       = TitleWorking.toStdString();
      xmpData["Xmp.tiff.ImageDescription"]  = TitleWorking.toStdString();
    }

    // Copyright
    QString CopyrightWorking = Settings->GetString("Copyright");
    StringClean(CopyrightWorking);
    if (CopyrightWorking != "") {
      outExifData["Exif.Image.Copyright"]     = CopyrightWorking.toStdString();
      iptcData["Iptc.Application2.Copyright"] = CopyrightWorking.toStdString();
      xmpData["Xmp.tiff.Copyright"]           = CopyrightWorking.toStdString();
    }

    Exiv2Image->setExifData(outExifData);
    Exiv2Image->setIptcData(iptcData);
    Exiv2Image->setXmpData( xmpData);
    Exiv2Image->writeMetadata();
    return true;
#endif
  } catch (Exiv2::AnyError& Error) {
    if (Settings->GetInt("JobMode") == 0) {
      ptMessageBox::warning(0 ,"Exiv2 Error","No exif data written!\nCaught Exiv2 exception '" + QString(Error.what()) + "'\n");
    } else {
      std::cout << "Caught Exiv2 exception '" << Error << "'\n";
    }
  }
  return false;
}

//==============================================================================

bool ptImageHelper::ReadExif(const QString AFileName, Exiv2::ExifData &AExifData, unsigned char *&AExifBuffer, unsigned int &AExifBufferLength)
{
  if (AFileName.trimmed().isEmpty()) return false;

  try {
    if (Exiv2::ImageFactory::getType(AFileName.toAscii().data()) == Exiv2::ImageType::none)
      return false;

    Exiv2::Image::AutoPtr hImage = Exiv2::ImageFactory::open(AFileName.toAscii().data());

    if (!hImage.get()) return false;

    hImage->readMetadata();

    AExifData = hImage->exifData();
    if (AExifData.empty()) {
      ptLogWarning(ptWarning_Argument, "No Exif data found in %s", AFileName.toAscii().data());
      return false;
    }

    Exiv2::ExifData::iterator Pos;
    size_t                    Size;

#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
    Exiv2::Blob               Blob;
    Exiv2::ExifParser::encode(Blob, Exiv2::bigEndian, AExifData);
    Size = Blob.size();
#else
    Exiv2::DataBuf            Buf(AExifData.copy());
    Size = Buf.size_;
#endif

    /* If buffer too big for JPEG, try deleting some stuff. */
    if (Size + sizeof(CExifHeader) > CMaxHeaderLength) {
      if ((Pos = AExifData.findKey(Exiv2::ExifKey("Exif.Photo.MakerNote"))) != AExifData.end() ) {
        AExifData.erase(Pos);
        ptLogWarning(ptWarning_Argument, "Exif buffer too big, erasing Exif.Photo.MakerNote");
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
        Exiv2::ExifParser::encode(Blob, Exiv2::bigEndian, AExifData);
        Size = Blob.size();
#else
        Buf = AExifData.copy();
        Size = Buf.size_;
#endif
      }
    }

    // Erase embedded thumbnail if needed
    if (Settings->GetInt("EraseExifThumbnail") || Size + sizeof(CExifHeader) > CMaxHeaderLength ) {
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
      Exiv2::ExifThumb Thumb(AExifData);
      Thumb.erase();
#else
      AExifData.eraseThumbnail();
#endif

      if (!Settings->GetInt("EraseExifThumbnail"))
        ptLogWarning(ptWarning_Argument, "Exif buffer too big, erasing Thumbnail");

#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
      Exiv2::ExifParser::encode(Blob ,Exiv2::bigEndian, AExifData);
      Size = Blob.size();
#else
      Buf = AExifData.copy();
      Size = Buf.size_;
#endif
    }

    AExifBufferLength = Size + sizeof(CExifHeader);
    if (AExifBuffer) FREE(AExifBuffer);
    AExifBuffer = (unsigned char*) MALLOC(AExifBufferLength);
    ptMemoryError(AExifBuffer,__FILE__,__LINE__);

    memcpy(AExifBuffer, CExifHeader, sizeof(CExifHeader));
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
    memcpy(AExifBuffer+sizeof(CExifHeader), &Blob[0], Blob.size());
#else
    memcpy(AExifBuffer+sizeof(CExifHeader), Buf.pData_, Buf.size_);
#endif
    return true;

  } catch(Exiv2::Error& Error) {
    // Exiv2 errors are in this context hopefully harmless
    // (unsupported tags etc.)

    ptLogWarning(ptWarning_Exiv2,"Exiv2 : %s\n",Error.what());
  }
  return false;
}

//==============================================================================

bool ptImageHelper::TransferExif(const QString ASourceFile, const QString ATargetFile)
{
  if (ASourceFile == ATargetFile      ||
      ASourceFile.trimmed().isEmpty() ||
      ATargetFile.trimmed().isEmpty())
    return false;

  try {
    if (Exiv2::ImageFactory::getType(ASourceFile.toAscii().data()) == Exiv2::ImageType::none)
      return false;

    Exiv2::Image::AutoPtr hSourceImage = Exiv2::ImageFactory::open(ASourceFile.toAscii().data());

    if (!hSourceImage.get()) return false;

    hSourceImage->readMetadata();

    Exiv2::Image::AutoPtr hTargetImage = Exiv2::ImageFactory::open(ATargetFile.toAscii().data());

    hTargetImage->clearMetadata();
    hTargetImage->setMetadata(*hSourceImage);
    hTargetImage->writeMetadata();
    return true;
  } catch (Exiv2::AnyError& Error) {
    if (Settings->GetInt("JobMode") == 0) {
      ptMessageBox::warning(0 ,"Exiv2 Error","No exif data written!\nCaught Exiv2 exception '" + QString(Error.what()) + "'\n");
    } else {
      std::cout << "Caught Exiv2 exception '" << Error << "'\n";
    }
  }
  return false;
}

//==============================================================================
// We don't want to use the saving function of QImage, since this requires the
// format plugins to be shipped.
bool ptImageHelper::DumpImage(QImage* AImage, const QString AFileName) {
  long unsigned int Width  = AImage->width();
  long unsigned int Height = AImage->height();

  MagickWand *mw;
  mw = NewMagickWand();
  MagickSetSize(mw, Width, Height);
  MagickReadImage(mw,"xc:white");
  MagickSetImageFormat(mw,"BGRA");
  MagickSetImageDepth(mw,8);
  MagickSetImageType(mw,TrueColorType);

  MagickSetImagePixels(mw,0,0,Width,Height,"BGRA",CharPixel,(unsigned char*) AImage->bits());

  MagickSetImageDepth(mw,8);

  MagickSetImageFormat(mw,"RGB");
  MagickSetCompressionQuality(mw,95);
  MagickSetImageCompression(mw, LZWCompression);

  bool Result = MagickWriteImage(mw, AFileName.toAscii().data());
  DestroyMagickWand(mw);
  return Result;
}

//==============================================================================

ptImageHelper::ptImageHelper()
{
}

//==============================================================================

ptImageHelper::~ptImageHelper()
{
}

//==============================================================================
