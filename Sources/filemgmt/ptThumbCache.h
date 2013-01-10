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

#ifndef PTTHUMBCACHE_H
#define PTTHUMBCACHE_H

//==============================================================================

#include <memory>
#include <QObject>
#include <QString>

#include "ptThumbDefines.h"
#include "ptThumbGen.h"


//==============================================================================

/*! Thumbnail cache \class: responsible for storing and retrieving thumbnails.*/
class ptThumbCache: public QObject {
Q_OBJECT

public:
  explicit ptThumbCache();
          ~ptThumbCache();

  bool findThumb(ptThumbData &AThumb);

  void insertThumb(const ptThumbData AThumb);

  // functions for cache management
private:
  std::vector<ptThumbData> FThumbs;
};

//==============================================================================

#endif // PTTHUMBCACHE_H
