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

#ifndef PTXMPIDMAP_H
#define PTXMPIDMAP_H

// Qt includes
#include <QString>
#include <QMutex>
#include <QSettings>

// local includes
#include "ptSettings.h"

/*! Thread-safe singelton wrapper around QSettings */
class ptXmpIDMap
{

public: ////////////////////////////////////////////////////////////////////////

  /*! **************************************************************************
   * Get singleton intance
   */
  static ptXmpIDMap &getMap()
  {
    static ptXmpIDMap map;
    return map;
  }

  /*! **************************************************************************
   * Set value by key. If this key already exists the value gets replaced,
   *  otherwise the key-value-pair gets added.
   */
  void setValue(const QString &key, const QString &value)
  {
    QMutexLocker locker(&mutex);
    settings.setValue(key, QVariant(value));
    settings.sync();
  }

  /*!  *************************************************************************
   * Get value by key
   */
  QString value(const QString &key) const
  {
    QMutexLocker locker(&mutex);
    return settings.value(key).toString();
  }

private: ///////////////////////////////////////////////////////////////////////

  QSettings      settings;
  QDateTime      lastModified;
  mutable QMutex mutex;

  /*! **************************************************************************
   * Initialize settings file path (<code>%UserDirectory%/xmpmmid.ini</code>)
   * and format (INI-Format, UTF-8)
   */
  ptXmpIDMap()
    : settings(settingsFilePath(), QSettings::IniFormat)
  {
    settings.setIniCodec("UTF-8");
  }

  /*! **************************************************************************
   * Helper to make initialization of <code>settings</code> in ctor less ugly
   */
  static QString settingsFilePath()
  {
    const QString filename = "xmpmmid.ini";
    const QString userDir  = Settings->GetString("UserDirectory");

    return QFileInfo(userDir).dir().absoluteFilePath(filename);
  }
};

#endif // PTXMPIDMAP_H
