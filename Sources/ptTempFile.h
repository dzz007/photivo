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
/*!
  \class ptTempFile
  Helper class to manage a temporary file.
  Because QTemporaryFile does not quite do what we need.
*/

#ifndef PTTEMPFILE_H
#define PTTEMPFILE_H

#include <QString>

//==============================================================================

class ptTempFile {
public:
  /*! Constructs a ptTempFile instance.
      \param ASrcFileName
        Optional source file. If a source file is given and exists it is copied to the
        temporary location. Otherwise an empty temp file is created.
   */
  explicit ptTempFile(const QString &ASrcFileName = "");

  /*! Destructs a ptTempFile instance and deletes the temp file from disk. */
  ~ptTempFile();

  /*! Returns the full absolute path to the temp file. */
  QString fileName() { return FFileName; }


private:
  QString FFileName;

};

#endif // PTTEMPFILE_H
