/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILEMGRDM_h
#define PTFILEMGRDM_h

//==============================================================================

#include <QObject>
#include <QPixmap>

//==============================================================================

/*! This \c struct carries all valuable information for each thumbnail. */
struct ptThumbnailData {
  QPixmap* Thumbnail;
  QString  Location;
};

//==============================================================================

/*! Clear for \c ptThumbnailData. */
void ClearThumbnailData( ptThumbnailData &Data);

//==============================================================================

/*!
  \class ptFileMgrDM

  \brief Data module for file management.

  This data module will handle the thumbnail creation and manage the corresponding
  memory. It's designed as a singleton.
*/

class ptFileMgrDM: public QObject {
Q_OBJECT

private:
  static ptFileMgrDM* m_Instance;

  ptFileMgrDM() {}
  ptFileMgrDM(const ptFileMgrDM&) : QObject() {}
  ~ptFileMgrDM() {}

public:
  /*! Get or create the singleton instance of \c ptFileMgrDM */
  static ptFileMgrDM* Instance_GoC();
  /*! Destroy the singleton instance of \c ptFileMgrDM */
  static void         Instance_Destroy();

  /*! Clear the data cache of \c ptFileMgrDM */
  void Clear();

};

//==============================================================================

#endif // PTFILEMGRDM_h
