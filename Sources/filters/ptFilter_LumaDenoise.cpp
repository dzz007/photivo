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

#include "ptFilter_LumaDenoise.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CLumaDenoiseId = "LumaDenoise";

const QString COpacity       = "Opacity";
const QString CEdgeThreshold = "EdgeThreshold";
const QString CLScale        = "LScale";
const QString CLStrength     = "LStrength";

//------------------------------------------------------------------------------

ptFilter_LumaDenoise::ptFilter_LumaDenoise():
  ptFilterBase()
{
  FIsSlow = true;
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_LumaDenoise::createLumaDenoise() {
  auto hInstance         = new ptFilter_LumaDenoise;
  hInstance->FFilterName = CLumaDenoiseId;
  hInstance->FCaption    = tr("Luminance denoising");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_LumaDenoise::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COpacity,                ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.1,        2,        true, true, tr("Opacity"),        tr("")})
    << ptCfgItem({CEdgeThreshold,          ptCfgItem::Slider,       50,          0,           50,           10,          0,        true, true, tr("Edge Threshold"), tr("")})
    << ptCfgItem({CLScale,                 ptCfgItem::Slider,        8.0,        4.0,         50.0,          4.0,        1,        true, true, tr("L scale"),        tr("")})
    << ptCfgItem({CLStrength,              ptCfgItem::Slider,        0.3,        0.0,          3.0,          0.02,       2,        true, true, tr("L strength"),     tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_LumaDenoise::doCheckHasActiveCfg() {
  return !qFuzzyIsNull(FConfig.value(COpacity).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_LumaDenoise::doRunFilter(ptImage *AImage) {
  AImage->toLab();
  AImage->BilateralDenoise(
      FConfig.value(CLScale).toDouble() * TheProcessor->m_ScaleFactor,
      FConfig.value(CLStrength).toDouble() / 10.0,
      FConfig.value(COpacity).toDouble(),
      FConfig.value(CEdgeThreshold).toDouble() * TheProcessor->m_ScaleFactor);
}

//------------------------------------------------------------------------------

RegisterHelper LumaDenoiseRegister(&ptFilter_LumaDenoise::createLumaDenoise, CLumaDenoiseId);

//------------------------------------------------------------------------------
