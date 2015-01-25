/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012-2013 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptTuningSpot.h"
#include "../ptCfgItem.h"
#include "../../ptSettings.h"

//------------------------------------------------------------------------------
ptTuningSpot::ptTuningSpot(const QList<ptCfgItem>* ADefaults, const ptCurve& ANullCurve)
: ptImageSpot(),
  FDefaults(ADefaults)
{
  for (ptCfgItem hCfgItem: *FDefaults) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType)
        FDataStore.insert(hCfgItem.Id, hCfgItem.Default);
  }

  FCurve = std::make_shared<ptCurve>();
  FCurve->set(ANullCurve);
}

//------------------------------------------------------------------------------
ptTuningSpot::~ptTuningSpot() {
/*
  Resources managed by Qt parent or other objects. Do not delete manually.
    FDefaults
*/
}

//------------------------------------------------------------------------------
/*!
  Returns a pointer to the spot’s ptCurve object. The pointer is only valid as
  long as the spot lives.
*/
ptCurve* ptTuningSpot::curvePtr() {
  return FCurve.get();
}

//------------------------------------------------------------------------------
/*! Returns the spot’s curve. */
std::shared_ptr<ptCurve> ptTuningSpot::curve() {
  return FCurve;
}

//------------------------------------------------------------------------------
TConfigStore ptTuningSpot::dodoStoreConfig(const QString &APrefix) const {
  TConfigStore hConfig;

  for (auto iter = FDataStore.begin(); iter != FDataStore.end(); ++iter) {
    hConfig.insert(APrefix+iter.key(), iter.value());
  }

  hConfig.unite(FCurve->storeConfig(APrefix+CSpotLumaCurveId));
  return hConfig;
}

//------------------------------------------------------------------------------
void ptTuningSpot::dodoLoadConfig(const TConfigStore &AConfig, const QString &APrefix) {
  FCurve->loadConfig(AConfig, APrefix+CSpotLumaCurveId);

  for (ptCfgItem hCfgItem: *FDefaults) {
    if(hCfgItem.Type < ptCfgItem::CFirstCustomType) {
      FDataStore.insert(hCfgItem.Id,
                        hCfgItem.validate(AConfig.value(APrefix+hCfgItem.Id, hCfgItem.Default)));
    }
  }
}

//------------------------------------------------------------------------------
QVariant ptTuningSpot::doValue(const QString &AKey) const {
  if (AKey == CSpotMaxRadiusId) {
    return FDataStore.value(CSpotMaxRadiusId).toInt() >> Settings->GetInt("Scaled");

  } else if (AKey == CSpotLumaCurveId) {
    QVariant hCurveCfg = FCurve->storeConfig("");
    return hCurveCfg;

  } else {
    return QVariant();
  }
}

//------------------------------------------------------------------------------
bool ptTuningSpot::doSetValue(const QString &AKey, const QVariant AValue) {
  if (AKey == CSpotMaxRadiusId) {
    FDataStore.insert(AKey, AValue.toInt() << Settings->GetInt("Scaled"));
    return true;

  } else if (AKey == CSpotLumaCurveId) {
    FCurve->loadConfig(AValue.toMap(), "");
    return true;

  } else {
    return false;
  }
}

//------------------------------------------------------------------------------
