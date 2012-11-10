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
  this->FDataStore    = AOther.FDataStore;
  this->FStoreIds     = AOther.FStoreIds;
  this->FCustomStores = AOther.FCustomStores;
}

//==============================================================================

void ptFilterConfig::init(const TConfigStore &AInitData) {
  FDataStore = AInitData;
}

//==============================================================================

void ptFilterConfig::update(const TConfigStore &AInitData) {
  // QMap::unite() is unsuitable to update an existing map with new data
  // because it creates duplicate keys. We have to use QMap::insert() manually.
  for (auto hItem = AInitData.constBegin(); hItem != AInitData.constEnd(); hItem++) {
    if (FDataStore.contains(hItem.key())) {
      FDataStore.insert(hItem.key(), hItem.value());
    }
  }
}

//==============================================================================

QVariant ptFilterConfig::getValue(const QString &AKey) const {
  if (!FDataStore.contains(AKey)) {
    GInfo->Raise(QString("Key \"%1\" not found in FDataStore.").arg(AKey), AT);
  }

  return FDataStore.value(AKey);
}

//==============================================================================

void ptFilterConfig::setValue(const QString &AKey, const QVariant &AValue) {
  if (!FDataStore.contains(AKey))
    GInfo->Raise(QString("Key \"%1\" not found in FDataStore.").arg(AKey), AT);

  FDataStore.insert(AKey, AValue);
}

//==============================================================================

TConfigStore *ptFilterConfig::newStore(const QString &AId, const TConfigStore ADefaults) {
  if (FStoreIds.indexOf(AId) != -1)
    GInfo->Raise("Id \"" + AId + "\" already defined. Must be unique!", AT);

  FStoreIds.append(AId);
  FCustomStores.append(ADefaults);
  return &FCustomStores.last();
}

//==============================================================================

TConfigStore *ptFilterConfig::getStore(const QString &AId) {
  int hIdx = FStoreIds.indexOf(AId);

  if (hIdx == -1)
    return nullptr;
  else
    return &FCustomStores[hIdx];
}

//==============================================================================

void ptFilterConfig::clearCustomStores() {
  FStoreIds.clear();
  FCustomStores.clear();
}

//==============================================================================
