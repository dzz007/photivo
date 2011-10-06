/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2011 Bernd Schöler <brother.john@photivo.org>
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

#include <QtCore>
#include <QtGui>
#include <QDateTime>
 
#ifdef Q_OS_WIN32
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
  
  
  if (!QFile::exists(SettingsFileName)) {
    QMessageBox::information(
        0, 
        QObject::tr("Photivo - Clear settings"),
        QObject::tr("No settings file present.\nPhotivo is already at its factory defaults.")
    );
  
  
  } else if (
      QMessageBox::warning(0,
        QObject::tr("Photivo - Clear settings"),
        QObject::tr("I am about to clear all your settings for Photivo.\nProceed?"),
        QMessageBox::Yes, QMessageBox::No)
      == QMessageBox::Yes)
  {
    QString Msg = "";
    QString BakFileName = 
        UserDirectory + "photivo-" + 
        QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".bak.ini";
    
    if (QFile::rename(SettingsFileName, BakFileName)) {
      Msg = QObject::tr("Settings successfully deleted.\n\nOld settings were backed up to\n") + BakFileName;
    
    } else if (QFile::remove(SettingsFileName)) {
      Msg = QObject::tr("Settings successfully deleted.");
    
    } else {
      Msg = QObject::tr("Could not delete settings file\n") + SettingsFileName +
          QObject::tr("\n\nYou might want to try to remove it manually.");
    }  

    QMessageBox::information(0, QObject::tr("Photivo - Clear settings"), Msg);  
  }
  
  return 0;
}
