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

#ifndef PTTHUMBDM_H
#define PTTHUMBDM_H

//==============================================================================

#include <deque>
#include <QObject>

#include "ptThumbDefines.h"
#include "ptThumbCache.h"
#include "ptThumbGen.h"

//==============================================================================

class ptThumbDM : public QObject {
Q_OBJECT

public:
  explicit ptThumbDM();
          ~ptThumbDM();
    
  /*! Ask for a specific thumbnail. It will be provided immediately, if it is in
   *  the cache, otherwise it will be generated. */
  void orderThumb(const ptThumbId AThumbId,
                  const bool      ACacheThumb);

  /*! Cancel an order. The thumbnail will be removed from the queue for
   *  generation.*/
  void cancelThumb(const ptThumbId AThumbId);

  /*! By default the thumbnail generator is used asynchronously.*/
  void setSyncMode(const bool AAsync);

signals:
  /*! Signal to distribute the thumbnail.*/
  void thumbnail(const ptThumbId  AThumbId,
                 const ptThumbPtr AThumb);
    
private slots:
  void finishedThumbGen();

private:
  // We need additional data for each thumb.
  struct ptThumbInfo {
    ptThumbId Id;
    bool      CacheThumb;
  };

  ptThumbCache            FThumbCache;   // Cache for storing thumbs
  ptThumbGen              FThumbGen;     // Thread for generating thumbs
  std::deque<ptThumbInfo> FNeededThumbs; // Thumbs to generate

  bool                    FAsync;

  void startThumbGen();

  void distributeThumbnail(const ptThumbData AThumbData);
};

//==============================================================================

#endif // PTTHUMBDM_H
