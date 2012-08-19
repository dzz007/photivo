/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Dominic Lyons <domlyons@googlemail.com>
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
** along with Photivo. If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

// Qt includes
#include <QString>

// local includes
#include "ptSettings.h"
#include "ptXmpIDMap.h"

// public: /////////////////////////////////////////////////////////////////////

//static//
ptXmpIDMap& ptXmpIDMap::getMap()
{
  static ptXmpIDMap map;
  return map;
}

//==============================================================================

void ptXmpIDMap::setValue(const QString &key, const QString &value)
{
  QMutexLocker locker(&mutex);//RAII: becomes unlocked on locker destruction
  settings.setValue(key, QVariant(value));
  settings.sync();
}

//==============================================================================

QString ptXmpIDMap::value(const QString &key) const
{
  QMutexLocker locker(&mutex);
  return settings.value(key).toString();
}

// private: ////////////////////////////////////////////////////////////////////

ptXmpIDMap::ptXmpIDMap()
  : settings(settingsFilePath(), QSettings::IniFormat)
{
  settings.setIniCodec("UTF-8");
}

//==============================================================================

//static//
QString ptXmpIDMap::settingsFilePath()
{
  const QString filename = "xmpmmid.ini";
  const QString userDir  = Settings->GetString("UserDirectory");

  return QFileInfo(userDir).dir().absoluteFilePath(filename);
}

//==============================================================================
