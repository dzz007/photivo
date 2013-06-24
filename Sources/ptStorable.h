/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
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
#ifndef PTSTORABLE_H
#define PTSTORABLE_H

#include "ptUtils.h"
#include <QVariant>

typedef QMap<QString, QVariant> TConfigStore;

class ptStorable {
public:
  ptStorable();
  virtual ~ptStorable();

  TConfigStore storeConfig(const QString &APrefix) const;
  void         loadConfig(const TConfigStore &AConfig, const QString &APrefix);

protected:
  virtual TConfigStore doStoreConfig(const QString &APrefix) const = 0;
  virtual void         doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) = 0;
};

#endif // PTSTORABLE_H
