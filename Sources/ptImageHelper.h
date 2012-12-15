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

#ifndef PTIMAGEHELPER_H
#define PTIMAGEHELPER_H

//==============================================================================

#include <exiv2/exif.hpp>

#include <QString>
#include <QImage>

//==============================================================================

/*!
  \class ptImageHelper

  \brief Collection of functions to work with images on disk.

  This class provides (only static) functions to work with images on disk.
  */

//==============================================================================

class ptImageHelper
{
public:
  /*! Write a given exif buffer to a file.*/
  static bool WriteExif(const QString    AFileName,
                        const Exiv2::ExifData AExifData);

  /*! Read exif data from file.*/
  static bool ReadExif(const QString    AFileName,
                       Exiv2::ExifData &AExifData,
                       unsigned char  *&AExifBuffer,
                       unsigned int    &AExifBufferLength);

  /*! Transfer exif data from one image to another.*/
  static bool TransferExif(const QString ASourceFile,
                           const QString ATargetFile);

  /*! Write a QImage to disk.*/
  static bool DumpImage(QImage       *AImage,
                        const QString AFileName);
private:
  ptImageHelper();
  ~ptImageHelper();
};

//==============================================================================

#endif // PTIMAGEHELPER_H
