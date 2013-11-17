/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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

#include "ptInfo.h"

#include <QApplication>
#include <QMessageBox>

#include <iostream>

//==============================================================================

ptInfo* ptInfo::FInstance = nullptr;

//==============================================================================

/*! Global variable to the info system.*/
ptInfo* GInfo;

//==============================================================================

/*! \class GetGlobalInfo
  Helper to instantiate and terminate the info system.
  */
class GetGlobalInfo
{
public:
  GetGlobalInfo() {GInfo = ptInfo::GetInstance();}
  ~GetGlobalInfo() {ptInfo::ShutDownInfoSystem();}
};

GetGlobalInfo GlobalInfo;

//==============================================================================
// This method needs to do everything needed for shutting the info system down.
void ptInfo::ShutDownInfoSystem()
{
  if (FInstance) delete FInstance;
  FInstance = 0;
}

//==============================================================================

void ptInfo::Log(const char *AMsg, const short AMode)
{
  QString hTemp = QString(AMsg);
  if (!hTemp.trimmed().isEmpty()) {
    if (AMode == 0 || !qApp->activeWindow()) {
      std::cout << hTemp.toLocal8Bit().data() << std::endl;
    } else if (AMode == 1) {
      QMessageBox::information(nullptr, "Photivo log", hTemp);
    }
  }
}

//==============================================================================

void ptInfo::Log(const QString &AMsg, const short AMode)
{
  Log(AMsg.toLocal8Bit().data(), AMode);
}

//==============================================================================

void ptInfo::Raise(const char *AMsg, const char *ALocation)
{
  QString hTemp = QString(AMsg);
  if (QString(ALocation).trimmed() != "") {
    hTemp += CRLF;
    hTemp += QString(ALocation);
  }

  Log(hTemp);
  throw;
}

//==============================================================================

void ptInfo::Raise(const QString AMsg, const char *ALocation)
{
  Raise(AMsg.toLocal8Bit().data(), ALocation);
}

//==============================================================================

void ptInfo::Warning(const char *AMsg, const char *ALocation) {
  QString hTemp = QString(AMsg);
  if (QString(ALocation).trimmed() != "") {
    hTemp += CRLF;
    hTemp += QString(ALocation);
  }

  Log(hTemp);
}

//==============================================================================

void ptInfo::Warning(const QString AMsg, const char *ALocation) {
  Warning(AMsg.toLocal8Bit().data(), ALocation);
}

//==============================================================================

void ptInfo::StartTimer(const char *AMsg)
{
  FTimerComplete->start();
  FTimerDiff->start();
  Log(AMsg);
}

//==============================================================================

ptInfo *ptInfo::GetInstance()
{
  if (!FInstance) {
    FInstance = new ptInfo();
  }

  return FInstance;
}

//==============================================================================

ptInfo::ptInfo()
{
  FTimerComplete = new QTime();
  FTimerDiff     = new QTime();

  // printf("**********\nInfo system started.\n**********\n");
}

//==============================================================================

ptInfo::~ptInfo()
{
  delete FTimerComplete;
  delete FTimerDiff;

  // printf("**********\nInfo system shut down.\n**********\n");
}

//==============================================================================

void ptInfo::StopTimer(const char *AMsg)
{
  // Just for symmetry.
  // The timer is not actually running, nothing to stop.
  QString hTemp = QString(AMsg).trimmed();
  if (!hTemp.isEmpty()) {
    hTemp += ": ";
  }
  hTemp += QString::number(FTimerComplete->elapsed());

  Log(hTemp, 1);
}

//==============================================================================

void ptInfo::ShowMsg(const char *AMsg, const char *ALocation)
{
  QString hTemp = QString(AMsg);
  if (QString(ALocation).trimmed() != "") {
    hTemp += CRLF;
    hTemp += QString(ALocation);
  }

  Log(hTemp, 1);
}

//==============================================================================

void ptInfo::ShowMsg(const QString AMsg, const char *ALocation)
{
  ShowMsg(AMsg.toLocal8Bit().data(), ALocation);
}

//==============================================================================

void ptInfo::LogTimerInterval(const char *AMsg)
{
  QString hTemp = QString(AMsg).trimmed();
  if (!hTemp.isEmpty()) {
    hTemp += ": ";
  }
  hTemp += QString::number(FTimerDiff->restart());

  Log(hTemp);
}

//==============================================================================

void ptInfo::Assert(const bool ACondition, const char *AMsg, const char *ALocation) {
  if (!ACondition)
    Raise(AMsg, ALocation);
}

//==============================================================================

void ptInfo::Assert(const bool ACondition, const QString AMsg, const char *ALocation) {
  if (!ACondition)
    Raise(AMsg.toLocal8Bit().data(), ALocation);
}

//==============================================================================
