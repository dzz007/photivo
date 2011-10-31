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

#include <qt_windows.h>
#include <QLibrary>
#include <QDir>

#include "ptWinApi.h"

//==============================================================================

#ifndef CSIDL_APPDATA
  #define CSIDL_APPDATA 0x001a
#endif

//==============================================================================

QString WinApi::AppdataFolder() {
  QString result;
  QLibrary library(QLatin1String("shell32"));
  typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPTSTR, int, BOOL);
  GetSpecialFolderPath SHGetSpecialFolderPath =
      (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathW");

  if (SHGetSpecialFolderPath) {
    TCHAR path[MAX_PATH];
    SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE);
    result = QString::fromUtf16((ushort*)path);
  }

  // WinAPI returns path with native separators "\". We need to change this to "/" for Qt.
  return QDir::fromNativeSeparators(result);
}

//==============================================================================

QString WinApi::VolumeName(QString& drive) {
  drive = drive.toUpper() + "\\";

  WCHAR szVolumeName[256] ;
  WCHAR szFileSystemName[256];
  DWORD dwSerialNumber = 0;
  DWORD dwMaxFileNameLength=256;
  DWORD dwFileSystemFlags=0;

  bool ret = GetVolumeInformation((WCHAR*)drive.utf16(),
                                  szVolumeName,
                                  256,
                                  &dwSerialNumber,
                                  &dwMaxFileNameLength,
                                  &dwFileSystemFlags,
                                  szFileSystemName,
                                  256);
  if(!ret) {
    return QString("");
  }

  QString vName = QString::fromUtf16((const ushort*)szVolumeName);
  vName.trimmed();
  return vName;
}

//==============================================================================
