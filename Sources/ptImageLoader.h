/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTIMAGELOADER_H
#define PTIMAGELOADER_H

//==============================================================================

#include <QImage>

#include <wand/magick_wand.h>

//==============================================================================

class ptImageLoader {
public:
  /*! Returns a pointer to the thumbnail.*/
  static QImage* getThumbnail(const QString FileName,
                              const int     MaxSize);


private:
  // prevent instatiation
  ptImageLoader() {}
  ~ptImageLoader() {}

  static QImage* GenerateThumbnail(MagickWand* image, const QSize tSize);
  static void ScaleThumbSize(QSize* tSize, const int max);


//==============================================================================
};
#endif // PTIMAGELOADER_H
