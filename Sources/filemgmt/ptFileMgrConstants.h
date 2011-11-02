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

#ifndef PTFILEMGRCONSTANTS_H
#define PTFILEMGRCONSTANTS_H

//==============================================================================

#include <QString>

//==============================================================================

const QString MyComputerIdString = "{{MyComputer}}";

//==============================================================================

/*! This enum defines the possible actions when you click on a thumbnail
    in the file manager. */
enum ptThumbnailAction {
  tnaChangeDir = 0,
  tnaLoadImage = 1,
  tnaViewImage = 2
};

/*! This enum defines how thumbnails are arranged in the file manager. */
enum ptThumbnailLayout {
  tlVerticalByRow       = 0,
  tlHorizontalByColumn  = 1,
  tlDetailedList        = 2
};

#endif // PTFILEMGRCONSTANTS_H
