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
    if (hCfgItem.Type == ptCfgItem::CurveWin)
      hCurveIdx = i;
    else
      FDataStore.insert(hCfgItem.Id, hCfgItem.Default);
  }

  if (hCurveIdx > -1) {
    FCurve = std::make_shared<ptCurve>();
    FCurve->set(*(FDefaults->at(hCurveIdx).Curve.get()));
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

TConfigStore ptTuningSpot::dodoStoreConfig(const QString &APrefix) const {
  TConfigStore hConfig;

  for (auto iter = FDataStore.begin(); iter != FDataStore.end(); ++iter) {
    hConfig.insert(APrefix+iter.key(), iter.value());
  }

  hConfig.unite(FCurve->storeConfig(APrefix+CSpotLumaCurveId));
  return hConfig;
}

//==============================================================================

void ptTuningSpot::dodoLoadConfig(const TConfigStore &AConfig, const QString &APrefix) {
  FCurve->loadConfig(AConfig, APrefix+CSpotLumaCurveId);

  for (ptCfgItem hCfgItem: *FDefaults) {
    if (hCfgItem.Id != CSpotLumaCurveId) {
      FDataStore.insert(hCfgItem.Id,
                        hCfgItem.validate(AConfig.value(APrefix+hCfgItem.Id, hCfgItem.Default)));
    }
  }
}

//==============================================================================

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

//==============================================================================

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

//==============================================================================

