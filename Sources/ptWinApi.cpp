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

#include "ptWinApi.h"

#include <qt_windows.h>
#include <QLibrary>
#include <QDir>

//==============================================================================

#ifndef CSIDL_APPDATA
#  define CSIDL_APPDATA 0x001a
#endif
#ifndef ATTACH_PARENT_PROCESS
#  define ATTACH_PARENT_PROCESS ((DWORD)-1)
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

// On Windows you can either always or never get a console window. I.e. you either get an annoying
// additional window or no console output even when Photivo was started from an existing console.
// The following takes care of that problem by trying to attach to the parent process’s console.
// On success we have a console window to output to. If not no additional window appears.
void WinApi::AttachToParentConsole() {
  QLibrary library(QLatin1String("kernel32"));
  typedef BOOL (WINAPI*AttachConsolePtr)(DWORD);
  AttachConsolePtr AttachConsole = (AttachConsolePtr)library.resolve("AttachConsole");

  if (AttachConsole) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
      // Attaching succeeded. Reopen output streams to be able to write to the parent’s console.
      freopen("CONOUT$", "wb", stdout);
      freopen("CONOUT$", "wb", stderr);
      // Done. printf, cout, cerr now use the attached console.
    }
  }
}

//==============================================================================

QStringList WinApi::DrivesListPretty() {
  QFileInfoList list = QDir::drives();
  QStringList result;
#if (QT_VERSION >= 0x40700)
  result.reserve(list.count());
#endif
  for (int i = 0; i < list.count(); i++) {
    result.append(WinApi::VolumeNamePretty(list.at(i).filePath().left(2)));
  }
  return result;
}

//==============================================================================

QString WinApi::VolumeName(QString drive) {
  drive = drive.left(2) + "\\";

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
  vName = vName.trimmed();
  return vName;
}

//==============================================================================

/*! Return volume name and drive letter formatted as "Name (D:)" */
QString WinApi::VolumeNamePretty(QString drive) {
  drive = drive.left(2).toUpper();
  return QString("%1 (%2)").arg(VolumeName(drive)).arg(drive).trimmed();
}

//==============================================================================
