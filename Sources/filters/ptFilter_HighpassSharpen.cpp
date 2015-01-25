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

#include "ptFilter_HighpassSharpen.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CHighpassSharpenId = "HighpassSharpen";

const QString CMode     = "Mode";
const QString CStrength = "Strength";
const QString CRadius   = "Radius";
const QString CDenoise  = "Denoise";

//------------------------------------------------------------------------------

ptFilter_HighpassSharpen::ptFilter_HighpassSharpen():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_HighpassSharpen::createHighpassSharpen() {
  auto hInstance         = new ptFilter_HighpassSharpen;
  hInstance->FFilterName = CHighpassSharpenId;
  hInstance->FCaption    = tr("Highpass sharpen");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_HighpassSharpen::doDefineControls() {
  FConfig.initStores(TCfgItemList()                           //--- Combo: list of entries               ---//
    //            Id          Type                Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,      ptCfgItem::Combo,   static_cast<int>(TFilterMode::Disabled), pt::ComboEntries::FilterModes, true, true, tr("Mode"),     tr("")})
    << ptCfgItem({CStrength,  ptCfgItem::Slider,  4.0,        0.0,          20.0,         0.5,        1,        true, true, tr("Strength"),           tr("")})
    << ptCfgItem({CRadius,    ptCfgItem::Slider,  2.0,        0.0,          10.0,         0.5,        1,        true, true, tr("Radius"),             tr("")})
    << ptCfgItem({CDenoise,   ptCfgItem::Slider,  0.2,        0.0,          10.0,         0.1,        1,        true, true, tr("Denoising strength"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_HighpassSharpen::doCheckHasActiveCfg() {
  return pt::isActiveFilterMode(FConfig.value(CMode));
}

//------------------------------------------------------------------------------

void ptFilter_HighpassSharpen::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->Highpass(
      FConfig.value(CRadius).toDouble() * TheProcessor->m_ScaleFactor,
      FConfig.value(CStrength).toDouble(),
      -0.3,
      FConfig.value(CDenoise).toDouble()/((log(TheProcessor->m_ScaleFactor)/log(0.5))+1.0));
}

//------------------------------------------------------------------------------

RegisterHelper HighpassSharpenRegister(&ptFilter_HighpassSharpen::createHighpassSharpen, CHighpassSharpenId);

//------------------------------------------------------------------------------
