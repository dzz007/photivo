/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include <QFile>
#include <QDir>

#include "ptTempFile.h"

//==============================================================================

ptTempFile::ptTempFile(const QString &ASrcFileName) {
  int i = 0;
  do {
    FFileName = QString("%1/photivo-tmp-%2.pts").arg(QDir::tempPath()).arg(i++);
  } while (QFile::exists(FFileName));

  if (QFile::exists(ASrcFileName))
    QFile::copy(ASrcFileName, FFileName);
}

//==============================================================================

ptTempFile::~ptTempFile() {
  if (QFile::exists(FFileName))
    QFile::remove(FFileName);
}

//==============================================================================

