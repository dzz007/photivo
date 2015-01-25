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

#include "ptFilter_LumaSatAdjust.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptInfo.h"

//==============================================================================

const QString CLumaAdjustId = "LumaAdjust";
const QString CSatAdjustId  = "SatAdjust";

const QString CAdjustRed        = "AdjustRed";
const QString CAdjustOrange     = "AdjustOrange";
const QString CAdjustYellow     = "AdjustYellow";
const QString CAdjustLightGreen = "AdjustLightGreen";
const QString CAdjustDarkGreen  = "AdjustDarkGreen";
const QString CAdjustCyan       = "AdjustCyan";
const QString CAdjustBlue       = "AdjustBlue";
const QString CAdjustMagenta    = "AdjustMagenta";

//==============================================================================

ptFilter_LumaSatAdjust::ptFilter_LumaSatAdjust()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_LumaSatAdjust::createLumaAdjust() {
  auto hInstance         = new ptFilter_LumaSatAdjust;
  hInstance->FMode       = LumaMode;
  hInstance->FFilterName = CLumaAdjustId;
  hInstance->FCaption    = tr("Luminance adjustment");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_LumaSatAdjust::createSatAdjust() {
  auto hInstance         = new ptFilter_LumaSatAdjust;
  hInstance->FMode       = SatMode;
  hInstance->FFilterName = CSatAdjustId;
  hInstance->FCaption    = tr("Saturation adjustment");
  return hInstance;
}

//==============================================================================

void ptFilter_LumaSatAdjust::doDefineControls() {
  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
      << ptCfgItem({CAdjustRed,              ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Red"),    tr("")})
      << ptCfgItem({CAdjustOrange,           ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Orange"),    tr("")})
      << ptCfgItem({CAdjustYellow,           ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Yellow"),    tr("")})
      << ptCfgItem({CAdjustLightGreen,       ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Light green"),    tr("")})
      << ptCfgItem({CAdjustDarkGreen,        ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Dark green"),    tr("")})
      << ptCfgItem({CAdjustCyan,             ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Cyan"),    tr("")})
      << ptCfgItem({CAdjustBlue,             ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Blue"),    tr("")})
      << ptCfgItem({CAdjustMagenta,          ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.05,       2,        true, true, tr("Magenta"),    tr("")})
  );
}

//==============================================================================

bool ptFilter_LumaSatAdjust::doCheckHasActiveCfg() {
  return (FConfig.value(CAdjustRed).toDouble() != 0.0) ||
         (FConfig.value(CAdjustOrange).toDouble() != 0.0) ||
         (FConfig.value(CAdjustYellow).toDouble() != 0.0) ||
         (FConfig.value(CAdjustLightGreen).toDouble() != 0.0) ||
         (FConfig.value(CAdjustDarkGreen).toDouble() != 0.0) ||
         (FConfig.value(CAdjustCyan).toDouble() != 0.0) ||
         (FConfig.value(CAdjustBlue).toDouble() != 0.0) ||
         (FConfig.value(CAdjustMagenta).toDouble() != 0.0);
}

//==============================================================================

void ptFilter_LumaSatAdjust::doRunFilter(ptImage *AImage) const {
  AImage->toLab();

  switch (FMode) {
    case LumaMode:
      AImage->LumaAdjust(FConfig.value(CAdjustRed).toDouble(),
                         FConfig.value(CAdjustOrange).toDouble(),
                         FConfig.value(CAdjustYellow).toDouble(),
                         FConfig.value(CAdjustLightGreen).toDouble(),
                         FConfig.value(CAdjustDarkGreen).toDouble(),
                         FConfig.value(CAdjustCyan).toDouble(),
                         FConfig.value(CAdjustBlue).toDouble(),
                         FConfig.value(CAdjustMagenta).toDouble());
      break;

    case SatMode:
      AImage->SatAdjust(FConfig.value(CAdjustRed).toDouble(),
                        FConfig.value(CAdjustOrange).toDouble(),
                        FConfig.value(CAdjustYellow).toDouble(),
                        FConfig.value(CAdjustLightGreen).toDouble(),
                        FConfig.value(CAdjustDarkGreen).toDouble(),
                        FConfig.value(CAdjustCyan).toDouble(),
                        FConfig.value(CAdjustBlue).toDouble(),
                        FConfig.value(CAdjustMagenta).toDouble());
      break;

    default:
      GInfo->Raise("Unknown filter mode.", AT);
  }
}

//==============================================================================

RegisterHelper LumaSatAdjustRegister(&ptFilter_LumaSatAdjust::createLumaAdjust, CLumaAdjustId);
RegisterHelper SatAdjustRegister(&ptFilter_LumaSatAdjust::createSatAdjust, CSatAdjustId);

//==============================================================================
