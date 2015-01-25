/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2015 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptFilter_ColorDenoise.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CColorDenoiseId = "ColorDenoise";

const QString CAStrength = "AStrength";
const QString CAScale    = "AScale";
const QString CBStrength = "BStrength";
const QString CBScale    = "BScale";

//------------------------------------------------------------------------------

ptFilter_ColorDenoise::ptFilter_ColorDenoise():
  ptFilterBase()
{
  FIsSlow = true;
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_ColorDenoise::createColorDenoise() {
  auto hInstance         = new ptFilter_ColorDenoise;
  hInstance->FFilterName = CColorDenoiseId;
  hInstance->FCaption    = tr("Color denoising");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_ColorDenoise::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CAStrength,              ptCfgItem::Slider,        0.0,        0.0,          3.0,          0.02,       2,        true, true, tr("A channel strength"), tr("")})
    << ptCfgItem({CAScale,                 ptCfgItem::Slider,        8.0,        4.0,         50.0,          4.0,        1,        true, true, tr("A channel scale"),    tr("")})
    << ptCfgItem({CBStrength,              ptCfgItem::Slider,        0.0,        0.0,          3.0,          0.02,       2,        true, true, tr("B channel strength"), tr("")})
    << ptCfgItem({CBScale,                 ptCfgItem::Slider,        8.0,        4.0,         50.0,          4.0,        1,        true, true, tr("B channel scale"),    tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_ColorDenoise::doCheckHasActiveCfg() {
  return
      !qFuzzyIsNull(FConfig.value(CAStrength).toFloat()) ||
      !qFuzzyIsNull(FConfig.value(CBStrength).toFloat());
}

//------------------------------------------------------------------------------

void ptFilter_ColorDenoise::doRunFilter(ptImage *AImage) const {
  AImage->toLab();

  double AStrength = FConfig.value(CAStrength).toFloat();

  if (!qFuzzyIsNull(AStrength)) {
    AImage->fastBilateralChannel(
        FConfig.value(CAScale).toFloat() * TheProcessor->m_ScaleFactor,
        AStrength / 10.0f,
        2,
        ChMask_a);
  }

  double BStrength = FConfig.value(CBStrength).toFloat();

  if (!qFuzzyIsNull(BStrength)) {
    AImage->fastBilateralChannel(
        FConfig.value(CBScale).toFloat() * TheProcessor->m_ScaleFactor,
        BStrength,
        2,
        ChMask_b);
  }
}

//------------------------------------------------------------------------------

RegisterHelper ColorDenoiseRegister(&ptFilter_ColorDenoise::createColorDenoise, CColorDenoiseId);

//------------------------------------------------------------------------------
