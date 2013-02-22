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

#include <QMap>
#include <QVariant>
#include <QStringList>

#include "ptStorable.h"

//forwards
class QSettings;


class ptFilterConfig {
public:
  ptFilterConfig();
  ptFilterConfig(const ptFilterConfig &AOther);
  ~ptFilterConfig();

  /*! \name Management of the default data store. *//*! @{*/
  void            init(const TConfigStore &AInitData);
  void            update(const TConfigStore &AInitData);
  QVariant        getValue(const QString &AKey) const;
  void            setValue(const QString &AKey, const QVariant &AValue);
  /*! @}*/

  /*! \name Management of additional custom data stores. *//*! @{*/
  TConfigStore       *newSimpleStore(const QString &AId, const TConfigStore ADefaults = TConfigStore());
  TConfigStore       *getSimpleStore(const QString &AId);
  const QStringList   simpleStoreIds() const { return FSimpleStoreIds; }
  void                clearSimpleStores();
  /*! @}*/

  /*! \name Management of complex custom stores that implement the ptStorable interface.
      ptFilterConfig does *not* take ownership of the stores.
  *//*! @{*/
  void                insertStore(const QString &AId, ptStorable *AStore);
  ptStorable         *getStore(const QString &AId);
  const QStringList   storeIds() const { return FStoreIds; }
  /*! @}*/


private:
  TConfigStore         FDefaultStore;
  QList<TConfigStore>  FSimpleStores;
  QStringList          FSimpleStoreIds;
  QList<ptStorable*>   FStores;
  QStringList          FStoreIds;

};

#endif // PTFILTERCONFIG_H
