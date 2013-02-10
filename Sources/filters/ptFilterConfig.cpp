/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptFilterConfig.h"
#include "../ptInfo.h"

//==============================================================================

/* Strictly for debugging! Dumps all key/value pairs to stdout. */
//#include <iostream>
//void DumpConfig(TFilterConfig ADataStore) {
//  std::cout << std::endl << "Number of keys: " << ADataStore.size() << std::endl;
//  for (auto hItem = ADataStore.constBegin(); hItem != ADataStore.constEnd(); hItem++) {
//    std::cout << hItem.key().toAscii().data()
//              << QString().fill('_', 30-hItem.key().length()).toAscii().data()
//              << hItem.value().type()
//              << "    "
//              << hItem.value().toString().toAscii().data()
//              << std::endl;
//  }
//}

//==============================================================================

ptFilterConfig::ptFilterConfig() {
  /* nothing to do */
}

//==============================================================================

ptFilterConfig::ptFilterConfig(const ptFilterConfig &AOther) {
  this->FDefaultStore    = AOther.FDefaultStore;
  this->FSimpleStoreIds     = AOther.FSimpleStoreIds;
  this->FSimpleStores = AOther.FSimpleStores;
}

//==============================================================================

void ptFilterConfig::init(const TConfigStore &AInitData) {
  FDefaultStore = AInitData;
}

//==============================================================================

void ptFilterConfig::update(const TConfigStore &AInitData) {
  // QMap::unite() is unsuitable to update an existing map with new data
  // because it creates duplicate keys. We have to use QMap::insert() manually.
  for (auto hItem = AInitData.constBegin(); hItem != AInitData.constEnd(); hItem++) {
    if (FDefaultStore.contains(hItem.key())) {
      FDefaultStore.insert(hItem.key(), hItem.value());
    }
  }
}

//==============================================================================

QVariant ptFilterConfig::getValue(const QString &AKey) const {
  if (!FDefaultStore.contains(AKey)) {
    GInfo->Raise(QString("Key \"%1\" not found in FDataStore.").arg(AKey), AT);
  }

  return FDefaultStore.value(AKey);
}

//==============================================================================

void ptFilterConfig::setValue(const QString &AKey, const QVariant &AValue) {
  if (!FDefaultStore.contains(AKey))
    GInfo->Raise(QString("Key \"%1\" not found in FDataStore.").arg(AKey), AT);

  FDefaultStore.insert(AKey, AValue);
}

//==============================================================================

TConfigStore *ptFilterConfig::newSimpleStore(const QString &AId, const TConfigStore ADefaults) {
  if (FSimpleStoreIds.indexOf(AId) != -1)
    GInfo->Raise("Id \"" + AId + "\" already defined. Must be unique!", AT);

  FSimpleStoreIds.append(AId);
  FSimpleStores.append(ADefaults);
  return &FSimpleStores.last();
}

//==============================================================================

TConfigStore *ptFilterConfig::getSimpleStore(const QString &AId) {
  int hIdx = FSimpleStoreIds.indexOf(AId);

  if (hIdx == -1)
    return nullptr;
  else
    return &FSimpleStores[hIdx];
}

//==============================================================================

void ptFilterConfig::clearSimpleStores() {
  FSimpleStoreIds.clear();
  FSimpleStores.clear();
}

//==============================================================================

void ptFilterConfig::insertStore(const QString &AId, ptStorable *AStore) {
  if (FStoreIds.indexOf(AId) != -1)
    GInfo->Raise("Id \"" + AId + "\" already defined. Must be unique!", AT);

  FStoreIds.append(AId);
  FStores.append(AStore);
}

//==============================================================================

ptStorable *ptFilterConfig::getStore(const QString &AId) {
  int hIdx = FStoreIds.indexOf(AId);

  if (hIdx == -1)
    return nullptr;
  else
    return FStores[hIdx];
}

//==============================================================================

