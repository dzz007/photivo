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

#include "ptFilterConfig.h"
#include "../ptUtils_Storage.h"
#include "../ptInfo.h"

#include <QSettings>

//------------------------------------------------------------------------------
// Strictly for debugging! Dumps all key/value pairs to stdout.
/*
#include <iostream>
void DumpConfig(QHash<QString, QVariant>& ADataStore) {
  std::cout << std::endl << "Number of keys: " << ADataStore.size() << std::endl;
  for (auto hItem = ADataStore.constBegin(); hItem != ADataStore.constEnd(); hItem++) {
    std::cout << hItem.key().toAscii().data()
              << QString().fill('_', 30-hItem.key().length()).toAscii().data()
              << hItem.value().type()
              << "    "
              << hItem.value().toString().toAscii().data()
              << std::endl;
  }
}
*/
//------------------------------------------------------------------------------
/*! Creates a new ptFilterConfig object. */
ptFilterConfig::ptFilterConfig() {
  /* nothing to do */
}

//------------------------------------------------------------------------------
/*! Destroys a ptFilterConfig object. */
ptFilterConfig::~ptFilterConfig() {
  // nothing to do here
}

//------------------------------------------------------------------------------
/*! Returns a non-modifiable reference to the current list of config items. */
const TCfgItemList &ptFilterConfig::items() const {
  return FItems;
}

//------------------------------------------------------------------------------
/*!
  Removes all entries from the default and custom stores. The ptFilterConfig object is
  guaranteed to be completely empty after calling this method.
*/
void ptFilterConfig::clear() {
  FDefaultStore.clear();
  FCustomStore.clear();
}

//------------------------------------------------------------------------------
/*!
  Exports the object’s config data from the default and custom stores into a *QSettings* structure.
  *APreset* must be set to the appropriate group before calling this method.
*/
void ptFilterConfig::exportPreset(QSettings *APreset) const {
  // default store: export simple key/value list of config data to APreset
  for (const ptCfgItem &hCfgItem: FItems) {
    if ((hCfgItem.Type < ptCfgItem::CFirstCustomType) && (hCfgItem.Storable))
      APreset->setValue(hCfgItem.Id, makeStorageFriendly(FDefaultStore.value(hCfgItem.Id)));
  }

  // custom store: Query the contained ptStorable objects for their key/value list of
  // config data and export it to the APreset as well
  auto hCStoreEnd = FCustomStore.constEnd();
  for (auto hCStoreIter = FCustomStore.constBegin(); hCStoreIter != hCStoreEnd; ++hCStoreIter) {
    // get config from an object in the custom store
    auto hConfigData = hCStoreIter.value()->storeConfig(hCStoreIter.key());

    // write the individual key/value pairs from the config data to the preset file
    auto hCfgEnd = hConfigData.constEnd();
    for (auto hCfgIter = hConfigData.constBegin(); hCfgIter != hCfgEnd; ++hCfgIter) {
      APreset->setValue(hCfgIter.key(), makeStorageFriendly(hCfgIter.value()));
    }
  }
}

//------------------------------------------------------------------------------
/*!
  Imports config data from a *QSettings* structure into the default and custom stores.
  *APreset* must be set to the appropriate group before calling this method.
*/
void ptFilterConfig::importPreset(QSettings *APreset) {
  // Import data for the default key/value store
  // Because QVariant validation is necessary we iterate over FItems instead of FDefaultStore.
  for (const ptCfgItem &hCfgItem: FItems) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType) {
      auto hValue = APreset->value(hCfgItem.Id); // returns invalid QVariant if not present in preset
      if (hValue.isValid())
        FDefaultStore[hCfgItem.Id] = hCfgItem.validate(hValue);
    }
  }

  // For all registered ptStorable objects create an appropriate key/value map from the data in
  // APreset and load it into the respective object.
  auto hCStoreEnd = FCustomStore.end();
  for (auto hCStoreIter = FCustomStore.begin(); hCStoreIter != hCStoreEnd; ++hCStoreIter) {
    TConfigStore hData = presetToMap(APreset, hCStoreIter.key());
    hCStoreIter.value()->loadConfig(hData, trailingSlash(APreset->group())+hCStoreIter.key());
  }
}

//------------------------------------------------------------------------------
/*! Returns *true* if both the default store and custom store are empty. */
bool ptFilterConfig::isEmpty() const {
  return FDefaultStore.isEmpty() && FCustomStore.isEmpty();
}

//------------------------------------------------------------------------------
/*!
  Initializes the data stores with the data provided in *ACfgItemList*. All existing
  entries in both data stores are discarded first.

  The object keeps a copy of *ACfgItemList* that can be accessed via items().
 */
void ptFilterConfig::initStores(const TCfgItemList &ACfgItemList) {
  this->clear();
  FItems = ACfgItemList;

  for (const ptCfgItem& hCfgItem: FItems) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType)
      FDefaultStore.insert(hCfgItem.Id, hCfgItem.Default);
    else
      FCustomStore.insert(hCfgItem.Id, hCfgItem.AssocObject);
  }
}

//------------------------------------------------------------------------------
/*! Resets all entries in both data stores to their default values. */
void ptFilterConfig::loadDefaults() {
  for (const ptCfgItem& hCfgItem: FItems) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType)
      FDefaultStore[hCfgItem.Id] = hCfgItem.Default;
    else
      FCustomStore[hCfgItem.Id]->loadConfig(hCfgItem.Default.toMap(), "");
  }
}

//------------------------------------------------------------------------------
/*! Returns the value for the config item *AKey*. */
QVariant ptFilterConfig::value(const QString &AKey) const {
  if (!FDefaultStore.contains(AKey)) {
    GInfo->Raise(QString("Key \"%1\" not found in FDataStore.").arg(AKey), AT);
  }

  return FDefaultStore.value(AKey);
}

//------------------------------------------------------------------------------
/*! Updates the config item *AKey* with *AValue*. */
void ptFilterConfig::setValue(const QString &AKey, const QVariant &AValue) {
  if (!FDefaultStore.contains(AKey))
    GInfo->Raise(QString("Key \"%1\" not found in FDataStore.").arg(AKey), AT);

  FDefaultStore[AKey] = AValue;
}

//------------------------------------------------------------------------------
bool ptFilterConfig::containsObject(const QString &AId) const {
  return FCustomStore.contains(AId);
}

//------------------------------------------------------------------------------
ptStorable *ptFilterConfig::object(const QString &AId) {
  if (!FCustomStore.contains(AId))
    GInfo->Raise("Custom store item \"" + AId + "\" not found.");

  return FCustomStore.value(AId);
}

