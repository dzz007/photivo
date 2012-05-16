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

#ifndef PTINFO_H
#define PTINFO_H

#include <QTime>
#include <QString>

#include "ptDefines.h"

//==============================================================================
// forward
class GetGlobalInfo;

//==============================================================================

/*! \class ptInfo
  Collects all messages send to the user, also errors.
  The singleton instance of the info system is accessible via the global
  variable \c GInfo.
  - We need to make sure, that it covers all of the program run time.
  - We should expect problems, once we work with more threads.
  */
class ptInfo
{
  friend class GetGlobalInfo;

public:
  /*! Raise an exception when \c ACondition is \c false. */
  void Assert(const bool ACondition, const char* AMsg, const char *ALocation = "");
  void Assert(const bool ACondition, const QString AMsg, const char *ALocation = "");

  /*! Raise an Error.*/
  void Raise(const char* AMsg, const char *ALocation = "");
  void Raise(const QString AMsg, const char *ALocation = "");

  /*! Just a warning to the user.*/
  void Warning(const char* AMsg, const char *ALocation = "");
  void Warning(const QString AMsg, const char *ALocation = "");

  /*! (Re)Start a global timer.*/
  void StartTimer(const char* AMsg = "");

  /*! Output timer value.*/
  void LogTimerInterval(const char* AMsg = "");

  /*! Stop global timer.*/
  void StopTimer(const char* AMsg = "");

private:
  ptInfo();
  ~ptInfo();

  /*! Public access to the singleton instance.*/
  static ptInfo* GetInstance();

  /*! Method to shut the info system down.*/
  static void ShutDownInfoSystem();

  /*! Singleton instance.*/
  static ptInfo* FInstance;

  /*! A method to route the logs.*/
  void Log(const char* AMsg = "", const short AMode = 0);
  void Log(const QString &AMsg = QString(), const short AMode = 0);

  QTime *FTimerComplete;
  QTime *FTimerDiff;
};

//==============================================================================

/*! Global variable to the info system.*/
extern ptInfo* GInfo;

//==============================================================================

#endif // PTINFO_H
