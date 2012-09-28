/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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
#include <ptSettings.h>
#include <filters/ptCfgItem.h>

//==============================================================================

ptTuningSpot::ptTuningSpot(const QList<ptCfgItem> *ADefaults)
: ptImageSpot(),
  FDefaults(ADefaults)
{
  int hCurveIdx = -1;
  int i = -1;
  for (ptCfgItem hCfgItem: *FDefaults) {
    ++i;
    if (hCfgItem.Type != ptCfgItem::CurveWin)
      FDataStore.insert(hCfgItem.Id, hCfgItem.Default);
    else
      hCurveIdx = i;
  }

  if (hCurveIdx > -1) {
    FCurve = FDefaults->at(hCurveIdx).Curve;
  } else {
    TAnchorList hAnchors = {TAnchor(0.0, 0.0), TAnchor(0.4, 0.6), TAnchor(1.0, 1.0)};
    FCurve = std::make_shared<ptCurve>(hAnchors,
                                       ptCurve::LumaMask,
                                       ptCurve::LumaMask,
                                       ptCurve::SplineInterpol);
  }
}

//==============================================================================

ptTuningSpot::~ptTuningSpot() {
/*
  Resources managed by Qt parent or other objects. Do not delete manually.
    FDefaults
*/
}

//==============================================================================

TConfigStore ptTuningSpot::doStoreConfig(const QString &APrefix) const {
  TConfigStore hConfig;

  for (auto iter = FDataStore.begin(); iter != FDataStore.end(); ++iter) {
    if (iter.key() == CSpotMaxRadiusId)
      hConfig.insert(APrefix+CSpotMaxRadiusId, iter.value().toInt() << Settings->GetInt("Scaled"));
    else
      hConfig.insert(APrefix+iter.key(), iter.value());
  }

  hConfig.unite(FCurve->storeConfig(APrefix+CSpotLumaCurveId+"/"));
  return hConfig;
}

//==============================================================================

void ptTuningSpot::doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) {
  FCurve->loadConfig(AConfig, APrefix+CSpotLumaCurveId+"/");

  for (ptCfgItem hCfgItem: *FDefaults) {
    FDataStore.insert(hCfgItem.Id, AConfig.value(APrefix+hCfgItem.Id, hCfgItem.Default));
  }
}

//==============================================================================

QVariant ptTuningSpot::doGetValue(const QString &AKey) const {
  if (AKey == CSpotMaxRadiusId) {
    return FDataStore.value(CSpotMaxRadiusId).toInt() >> Settings->GetInt("Scaled");

  } else {
    return QVariant();
  }
}

//==============================================================================

bool ptTuningSpot::doSetValue(const QString &AKey, const QVariant AValue) {
  if (AKey == CSpotMaxRadiusId) {
    FDataStore.insert(AKey, AValue.toInt() << Settings->GetInt("Scaled"));
    return true;

  } else {
    return false;
  }
}

//==============================================================================

