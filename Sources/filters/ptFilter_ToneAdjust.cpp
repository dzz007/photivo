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

#include "ptFilter_ToneAdjust.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"
#include "../ptConstants.h"

//==============================================================================

const QString CToneAdjustId = "ToneAdjust";

const QString CStrength     = "Strength";
const QString CMaskMode     = "MaskMode";
const QString CSaturation   = "Saturation";
const QString CHue          = "Hue";
const QString CLowerLimit   = "LowerLimit";
const QString CUpperLimit   = "UpperLimit";
const QString CSoftness     = "Softness";

//==============================================================================

ptFilter_ToneAdjust::ptFilter_ToneAdjust()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ToneAdjust::CreateToneAdjust() {
  auto hInstance         = new ptFilter_ToneAdjust;
  hInstance->FFilterName = CToneAdjustId;
  hInstance->FCaption    = tr("Tone adjustment");
  return hInstance;
}

//==============================================================================

void ptFilter_ToneAdjust::doDefineControls() {
  ptCfgItem::TComboEntryList hMaskModes = pt::ComboEntries::MaskTypes;
  hMaskModes.removeFirst(); // Remove "disabled" entry. Filter activation state is determined by strength.

  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.05,       2,        true, true, tr("Strength"),    tr("")})
    << ptCfgItem({CMaskMode,               ptCfgItem::Combo,         static_cast<int>(TMaskType::Shadows), hMaskModes,             true, true, tr("Mask mode"),   tr("")})
    << ptCfgItem({CSaturation,             ptCfgItem::Slider,        1.0,        0.0,          4.0,          0.1,        2,        true, true, tr("Saturation"),  tr("")})
    << ptCfgItem({CHue,                    ptCfgItem::HueSlider,     60,         0,            360,          10,         0,        true, true, tr("Hue"),         tr("")})
    << ptCfgItem({CLowerLimit,             ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.05,       3,        true, true, tr("Lower limit"), tr("")})
    << ptCfgItem({CUpperLimit,             ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.05,       3,        true, true, tr("Upper limit"), tr("")})
    << ptCfgItem({CSoftness,               ptCfgItem::Slider,        0.0,       -2.0,          2.0,          0.1,        1,        true, true, tr("Softness"),    tr("")})
  );
}

//==============================================================================

bool ptFilter_ToneAdjust::doCheckHasActiveCfg() {
  return FConfig.value(CStrength).toFloat() != 0.0f;
}

//==============================================================================

void ptFilter_ToneAdjust::doRunFilter(ptImage *AImage) {
  AImage->toLab();
  AImage->LABTone(FConfig.value(CStrength).toDouble(),
                  FConfig.value(CHue).toDouble(),
                  FConfig.value(CSaturation).toDouble(),
                  static_cast<TMaskType>(FConfig.value(CMaskMode).toInt()),
                  true, // manual mask
                  FConfig.value(CLowerLimit).toDouble(),
                  FConfig.value(CUpperLimit).toDouble(),
                  FConfig.value(CSoftness).toDouble() );
}

//==============================================================================

RegisterHelper ToneAdjustRegister(&ptFilter_ToneAdjust::CreateToneAdjust, CToneAdjustId);

//==============================================================================
