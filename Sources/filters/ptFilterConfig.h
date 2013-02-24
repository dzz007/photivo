/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012-2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILTERCONFIG_H
#define PTFILTERCONFIG_H

#include "../ptStorable.h"
#include <QMap>
#include <QVariant>
#include <QStringList>

class QSettings;

//------------------------------------------------------------------------------
struct TFlaggedVariant {
  QVariant  Value;
  bool      Storable;
};

typedef QMap<QString, TFlaggedVariant> TFlaggedConfigStore;

//------------------------------------------------------------------------------
class ptFilterConfig {
public:
  ptFilterConfig();
  ptFilterConfig(const ptFilterConfig &AOther);
  ~ptFilterConfig();

  void exportPreset(QSettings *APreset) const;
  void importPreset(QSettings *APreset);
  bool isEmpty() const;

  /*! \name Management of the default data store. *//*! @{*/
  void      init(const TFlaggedConfigStore &AInitData);
//  void      update(const TConfigStore &AInitData);
  QVariant  value(const QString &AKey) const;
  void      setValue(const QString &AKey, const QVariant &AValue);
  /*! @}*/

  /*! \name Management of the custom store *//*! @{*/
  bool        containsObject(const QString &AId) const;
  void        insertObject(const QString &AId, ptStorable *AObject);
  ptStorable *object(const QString &AId);
  /*! @}*/

private:
  typedef QMap<QString, ptStorable*> TCustomStore;

  TFlaggedConfigStore FDefaultStore;
  TCustomStore        FCustomStore;
};

#endif // PTFILTERCONFIG_H
