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

#include "ptConfirmRequest.h"
#include "ptMessageBox.h"
#include "ptSettings.h"
// ATZ
#include "filters/ptFilterDM.h"
#include "ptProcessor.h"
// end ATZ

extern ptSettings* Settings;
extern ptProcessor* TheProcessor;
extern short ImageCleanUp;
extern void ptRemoveFile( const QString FileName);

void CB_WritePipeButton();
void WriteCachedImage(const QString& fileName);

////////////////////////////////////////////////////////////////////////////////
//
// loadConfig()
//
////////////////////////////////////////////////////////////////////////////////
bool ptConfirmRequest::loadConfig(const ptLoadCfgMode mode, QString newFilename /*= ""*/) {
  if (Settings->GetInt("ResetSettingsConfirmation") == 0) {
    return true;
  }

  ptMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(QObject::tr("Photivo: Load configuration"));

  switch (mode) {
    case lcmGeneralCfg:
      msgBox.setText(QObject::tr("Discard current configuration and load new settings?"));
      break;

    case lcmNeutralPreset:
      newFilename = Settings->GetString("StartupSettingsFile");
#ifdef Q_OS_WIN
      newFilename.replace(QString("/"), QString("\\"));
#endif
      msgBox.setText(QObject::tr("Discard current configuration and reset to startup preset?\n") +
                     newFilename);
      break;

    case lcmStartupPreset:
      msgBox.setText(QObject::tr("Discard current configuration and reset to neutral preset?"));
      break;

    case lcmPresetFile:
      msgBox.setText(QObject::tr("Discard current configuration and load preset file?\n") +
              #ifdef Q_OS_WIN
                newFilename.replace(QString("/"), QString("\\")));
              #else
                newFilename);
              #endif
      break;

    case lcmSettingsFile:
      msgBox.setText(QObject::tr("Discard current configuration and load settings file?\n") +
               #ifdef Q_OS_WIN
                 newFilename.replace(QString("/"), QString("\\")));
               #else
                 newFilename);
               #endif
      break;

    default:
      // should never be reached
      break;
  }

  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::Yes);

  return msgBox.exec() == QMessageBox::Yes;
}


////////////////////////////////////////////////////////////////////////////////
//
// loadImage()
//
////////////////////////////////////////////////////////////////////////////////
bool ptConfirmRequest::saveImage(QString newFilename /*= ""*/) {
  QStringList InputFileNameList = Settings->GetStringList("InputFileNameList");

  if (Settings->GetInt("SaveConfirmation") == 0) {
    if (ImageCleanUp > 1) { // clean up the input file if we got just a temp file
      ptRemoveFile(InputFileNameList[0]);
      ImageCleanUp--;
    }
// ATZ: save .pts file automatically. Also save .cached jpg if needed
    else {
      QFileInfo PathInfo(InputFileNameList[0]);
      QString baseFileName = PathInfo.dir().absolutePath() + QDir::separator() + PathInfo.baseName();
      // check if we have processed image
      if (TheProcessor->m_Image_AfterDcRaw != NULL) {
        GFilterDM->WritePresetFile(baseFileName + ".pts");
        if (Settings->GetInt("SaveCachedImage") == 1) {
          // ImageMagic needs right file extension, so write to jpg, then rename
          if (QFile::exists(baseFileName + ".cached"))
            QFile::remove(baseFileName + ".cached");
          WriteCachedImage(baseFileName + "___.jpg");
          QFile::rename(baseFileName + "___.jpg", baseFileName + ".cached");
        }
      }
    }
// end ATZ
    return true;
  }

  ptMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);

  // message for save file without load new one
  if (newFilename == "") {
    msgBox.setWindowTitle(QObject::tr("Photivo: Save image"));
    msgBox.setText(QObject::tr("Do you want to save the current image?"));

  // message for save+load new file
  } else {
    msgBox.setWindowTitle(QObject::tr("Photivo: Open image"));
    msgBox.setText(QObject::tr("Before opening the image:\n") +
                 #ifdef Q_OS_WIN
                   newFilename.replace(QString("/"), QString("\\"))
                 #else
                   newFilename
                 #endif
                   + "\n\n" +
                   QObject::tr("Do you want to save the current image?"));
  }

  msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Save);
  int userChoice = msgBox.exec();

  switch (userChoice) {
    case QMessageBox::Save:
      // Save was clicked
      CB_WritePipeButton();
      if (ImageCleanUp > 1) { // clean up the input file if we got just a temp file
        ptRemoveFile(InputFileNameList[0]);
        ImageCleanUp--;
      }
      break;

    case QMessageBox::Discard:
      // Don't Save was clicked
      if (ImageCleanUp > 1) {  // clean up the input file if we got just a temp file
        ptRemoveFile(InputFileNameList[0]);
        ImageCleanUp--;
      }
      break;

    case QMessageBox::Cancel:
      // nothing to do
      break;

    default:
       // should never be reached
       break;
  }

  return userChoice == QMessageBox::Save || userChoice == QMessageBox::Discard;
}
