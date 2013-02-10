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

#ifndef PTTHUMBDEFINES_H
#define PTTHUMBDEFINES_H

#include <memory>
#include <ctime>
#include <QString>

#include "ptFileMgrConstants.h"

//==============================================================================

class ptImage8;
class ptGraphicsThumbGroup;

//==============================================================================

typedef std::shared_ptr<ptImage8> ptThumbPtr;

//==============================================================================

struct ptThumbId {
  explicit ptThumbId();
  explicit ptThumbId(const QString        &AFileName,
                     const uint16_t        AMaxSize,
                     ptGraphicsThumbGroup *AThumbGroup);

  void init();
  bool isEqual(const ptThumbId AId) const;
  bool isEmpty() const;

  QString  FileName;
  uint16_t MaxSize; // longest edge in pixel
  // Metadata
  ptGraphicsThumbGroup* ThumbGroup;
};

//==============================================================================

struct ptThumbData {
  ptThumbId   Id;
  ptThumbPtr  Thumbnail;
  std::time_t LastAccess;

  void init();
  bool isEmpty() const;
};

//==============================================================================
// Interface for synchronous events
class ptThumbGroupEvents {
public:
  virtual void thumbnailAction(const ptThumbnailAction AAction,
                               const QString           AFilename) = 0;

  virtual void currentThumbnail(const QString AFilename) = 0;

  virtual bool focusChanged() = 0;
};

//==============================================================================
// Interface for distributing the thumbnail
class ptThumbReciever {
public:
  virtual void thumbnail(const ptThumbId  AThumbId,
                         const ptThumbPtr AThumb) = 0;
};

//==============================================================================

#endif // PTTHUMBDEFINES_H
