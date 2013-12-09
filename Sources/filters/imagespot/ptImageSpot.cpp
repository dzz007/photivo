/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptImageSpot.h"
#include "../../ptSettings.h"

//==============================================================================

ptImageSpot::ptImageSpot()
: FIsEnabled(true)
{}

//==============================================================================

ptImageSpot::~ptImageSpot()
{}

//==============================================================================

QPoint ptImageSpot::pos() const {
  return QPoint(FPos.x() >> Settings->GetInt("Scaled"),
                FPos.y() >> Settings->GetInt("Scaled") );
}

//==============================================================================

void ptImageSpot::setPos(const QPoint &APos) {
  FPos.setX(APos.x() << Settings->GetInt("Scaled"));
  FPos.setY(APos.y() << Settings->GetInt("Scaled"));
}

//==============================================================================

int ptImageSpot::x() const {
  return FPos.x() >> Settings->GetInt("Scaled");
}

//==============================================================================

int ptImageSpot::y() const {
  return FPos.y() >> Settings->GetInt("Scaled");
}

//==============================================================================

QVariant ptImageSpot::value(const QString &AKey) const {
  QVariant hResult = doValue(AKey);

  if (hResult.isValid())              return hResult;
  else if (FDataStore.contains(AKey)) return FDataStore.value(AKey);
  else if (AKey == CSpotIsEnabledId)  return FIsEnabled;
  else if (AKey == CSpotNameId)       return FName;
  else if (AKey == CSpotPosId)        return this->pos();
  else {
    GInfo->Raise("Invalid key: " + AKey, AT);
    return QVariant();
  }
}

//==============================================================================

void ptImageSpot::setValue(const QString &AKey, const QVariant AValue) {
  if (doSetValue(AKey, AValue))       return;
  else if (FDataStore.contains(AKey)) FDataStore.insert(AKey, AValue);
  else if (AKey == CSpotIsEnabledId)  FIsEnabled = AValue.toBool();
  else if (AKey == CSpotNameId)       FName = AValue.toString();
  else if (AKey == CSpotPosId)        this->setPos(AValue.toPoint());
  else
    GInfo->Raise("Invalid key: " + AKey, AT);
}

//==============================================================================

TConfigStore ptImageSpot::doStoreConfig(const QString &APrefix) const {
  auto hConfig = this->dodoStoreConfig(APrefix);

  hConfig.insert(APrefix+CSpotIsEnabledId, FIsEnabled);
  hConfig.insert(APrefix+CSpotNameId, FName);
  hConfig.insert(APrefix+CSpotPosId+"/X", this->x());
  hConfig.insert(APrefix+CSpotPosId+"/Y", this->y());

  return hConfig;
}

//==============================================================================

void ptImageSpot::doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) {
  this->setEnabled(AConfig.value(APrefix+CSpotIsEnabledId, true).toBool());
  this->setName(AConfig.value(APrefix+CSpotNameId, "").toString());
  this->setPos(QPoint(AConfig.value(APrefix+CSpotPosId+"/X").toInt(),
                      AConfig.value(APrefix+CSpotPosId+"/Y").toInt()));

  this->dodoLoadConfig(AConfig, APrefix);
}

//==============================================================================

