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

#include "ptFilter_Saturation.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CSaturationId = "Saturation";
const QString CStrength     = "Strength";

//==============================================================================

ptFilter_Saturation::ptFilter_Saturation()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Saturation::createSaturation() {
  auto hInstance         = new ptFilter_Saturation;
  hInstance->FFilterName = CSaturationId;
  hInstance->FCaption    = tr("Saturation adjustment");
  return hInstance;
}

//==============================================================================

void ptFilter_Saturation::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.0,      -10.0,         10.0,          0.5,        1,        true, true, tr("Strength"), tr("")})
  ;
}

//==============================================================================

bool ptFilter_Saturation::doCheckHasActiveCfg() {
  return FConfig.value(CStrength).toFloat() != 0.0f;
}

//==============================================================================

void ptFilter_Saturation::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->SigmoidalContrast(FConfig.value(CStrength).toFloat(), 0.5, ChMask_a|ChMask_b);
}

//==============================================================================

RegisterHelper SaturationRegister(&ptFilter_Saturation::createSaturation, CSaturationId);

//==============================================================================
