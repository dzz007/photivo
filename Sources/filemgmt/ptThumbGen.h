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

#ifndef PTTHUMBGEN_H
#define PTTHUMBGEN_H

#include <queue>
#include <vector>
#include <functional>
#include <QThread>
#include <QSize>

#include <wand/magick_wand.h>

#include "ptThumbDefines.h"

//==============================================================================

class ptThumbGen: public QThread {
Q_OBJECT

public:
  ptThumbGen();
  ~ptThumbGen();

  void       setCurrentThumb(const ptThumbId AThumbId);
  ptThumbId  getCurrentThumb() const;
  ptThumbPtr getThumbnail() const;

  /*! run is public to allow sync work.*/
  void run();

private:
  ptThumbId  FCurrentThumb;
  ptThumbPtr FThumbnail;

  void generateThumb(const ptThumbId AThumbId);
  void ScaleThumbSize(QSize &ASize, const int AMax);
  void TransformImage(MagickWand *AInImage, ptImage8 *AOutImage, const QSize ASize);
};

//==============================================================================

#endif // PTTHUMBGEN_H
