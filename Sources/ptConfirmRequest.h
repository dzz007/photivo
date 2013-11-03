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
/*!
  \class ptConfirmRequest

  \brief A class dealing with confirmation requests.

  This class handles Photivo’s user confirmation dialogs and takes care of associated
  actions like saving the current image before loading a new one.
*/
#ifndef PTCONFIRMREQUEST_H
#define PTCONFIRMREQUEST_H

#include "ptConstants.h"

#include <QString>

class ptConfirmRequest {
public:
  static bool loadConfig(const ptLoadCfgMode mode, QString newFilename = "");
  static bool saveImage(QString newFilename = "");
};
#endif // PTCONFIRMREQUEST_H
