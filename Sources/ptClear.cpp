////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <QtCore>
#include <QtGui>

#ifdef Q_OS_WIN32
  // Get %appdata% via WinAPI call
  #include "qt_windows.h"
  #include "qlibrary.h"
  #ifndef CSIDL_APPDATA
    #define CSIDL_APPDATA 0x001a
  #endif
#endif

QApplication* TheApplication;

int main(int Argc, char *Argv[]) {

  //QApplication TheApplication(Argc,Argv);
  TheApplication = new QApplication(Argc,Argv);

  // User home folder, where Photivo stores its ini and all Presets, Curves etc
  // %appdata%\Photivo on Windows, ~/.photivo on Linux
  #ifdef Q_OS_WIN32
    // Get %appdata% via WinAPI call
    QString AppDataFolder;
    QLibrary library(QLatin1String("shell32"));
    QT_WA(
      {
        typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPTSTR, int, BOOL);
        GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathW");
        if (SHGetSpecialFolderPath) {
          TCHAR path[MAX_PATH];
          SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE);
          AppDataFolder = QString::fromUtf16((ushort*)path);
        }
      },
      {
        typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, char*, int, BOOL);
        GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathA");
        if (SHGetSpecialFolderPath) {
          char path[MAX_PATH];
          SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE);
          AppDataFolder = QString::fromLocal8Bit(path);
        }
      }
    );

    // WinAPI returns path with native separators "\". We need to change this to "/" for Qt.
    AppDataFolder.replace(QString("\\"), QString("/"));
    // Keeping the leading "/" separate here is important or mkdir will fail.
    QString Folder = "Photivo/";
  #else
    QString Folder = ".photivo/";
    QString AppDataFolder = QDir::homePath();
  #endif

  QString UserDirectory = AppDataFolder + "/" + Folder;
  QString SettingsFileName = UserDirectory + "photivo.ini";

  if (QMessageBox::warning(0,
        QObject::tr("Are you sure?"),
        QObject::tr("I'm going to clear all your settings for Photivo.\nProceed?"),
        QMessageBox::Ok,QMessageBox::Cancel)==QMessageBox::Ok){
    if(QFile::remove(SettingsFileName))
      QMessageBox::information(0,"Done","Settings are removed.");
    else
      QMessageBox::information(0,"Done","Nothing to remove.");
  }

  return 0;
}


