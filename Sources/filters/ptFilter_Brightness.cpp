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

#include "ptFilter_Brightness.h"
#include "../ptImage.h"

//==============================================================================

const QString CBrightnessId = "Brightness";

const QString CCatchWhite   = "CatchWhite";
const QString CCatchBlack   = "CatchBlack";
const QString CGain         = "Gain";

//==============================================================================

ptFilter_Brightness::ptFilter_Brightness()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Brightness::createBrightness() {
  auto hInstance         = new ptFilter_Brightness;
  hInstance->FFilterName = CBrightnessId;
  hInstance->FCaption    = tr("Brightness");
  return hInstance;
}

//==============================================================================

void ptFilter_Brightness::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CCatchWhite,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Catch white"), tr("Darken the bright parts")})
    << ptCfgItem({CCatchBlack,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Catch black"), tr("Brighten the dark parts")})
    << ptCfgItem({CGain,                   ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Gain"), tr("Exposure gain")})
  );
}

//==============================================================================

bool ptFilter_Brightness::doCheckHasActiveCfg() {
  return (FConfig.value(CCatchWhite).toFloat() != 0.0f) ||
         (FConfig.value(CCatchBlack).toFloat() != 0.0f) ||
         (FConfig.value(CGain).toFloat() != 0.0f);
}

//==============================================================================

void ptFilter_Brightness::doRunFilter(ptImage *AImage) {
  AImage->toRGB();

  float hWhite = FConfig.value(CCatchWhite).toFloat();
  float hBlack = FConfig.value(CCatchBlack).toFloat();
  float hGain  = FConfig.value(CGain).toFloat();

  // catch white/black curve
  TAnchorList hAnchors;
  if (hBlack > 0.0f) {
    hAnchors.push_back(TAnchor(0.0, 0.02*hBlack));
  } else {
    hAnchors.push_back(TAnchor(-0.02*hBlack, 0.0));
  }

  for (int i = 3; i < 12; ++i) {
    hAnchors.push_back(TAnchor((double) i/20.0, (double) i/20.0));
  }

  if (hWhite > 0.0f) {
    hAnchors.push_back(TAnchor(1.0, 1.0-0.40*hWhite));
  } else {
    hAnchors.push_back(TAnchor(1.0+0.40*hWhite, 1.0));
  }

  std::unique_ptr<ptCurve> hCurve = make_unique<ptCurve>(hAnchors);
  AImage->ApplyCurve(hCurve.get(), ChMask_RGB);

  // gain curve
  hCurve->setFromAnchors({TAnchor(0.0,           0.0),
                          TAnchor(0.5-0.2*hGain, 0.5+0.2*hGain),
                          TAnchor(1.0,           1.0)} );
  AImage->ApplyCurve(hCurve.get(), ChMask_RGB);
}

//==============================================================================

RegisterHelper BrightnessRegister(&ptFilter_Brightness::createBrightness, CBrightnessId);

//==============================================================================
