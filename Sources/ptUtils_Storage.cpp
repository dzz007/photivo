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
#include "ptUtils_Storage.h"
#include <QSettings>
#include <QStringList>

/*! Checks and possibly converts a *QVariant* to avoid problems in *QSettings* storage. */
QVariant makeStorageFriendly(const QVariant &AVariant) {
  auto hVariant = AVariant;

  // Convert bool to int because that is more robust in a preset file
  if (hVariant.type() == QVariant::Bool)
    hVariant.convert(QVariant::Int);

  return hVariant;
}

//------------------------------------------------------------------------------
/*!
  Converts the key/value pairs from group *AGroup* in *APreset* to a *TConfigStore* map and
  returns it.
  + The keys in the map are relative to *AGroup*.
  + *AGroup* may be empty.
  + The function sets *APreset*’s group to *AGroup* during its execution. If *APreset* is already
    set to the group, *AGroup* must be emtpy.
*/
TConfigStore presetToMap(QSettings *APreset, const QString &AGroup) {
  if (!AGroup.isEmpty())
    APreset->beginGroup(AGroup);

  TConfigStore hResult;
  for (auto hKey: APreset->allKeys()) {
    hResult.insert(hKey, APreset->value(hKey));
  }

  if (!AGroup.isEmpty())
    APreset->endGroup();

  return hResult;
}
