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

#include "ptFilter_Drc.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CDrcId = "Drc";

const QString CStrength   = "Strength";   // DRC "beta"
const QString CBias       = "Bias";       // DRC "alpha"
const QString CColorAdapt = "ColorAdapt";

//==============================================================================

ptFilter_Drc::ptFilter_Drc()
: ptFilterBase()
{
  FIsSlow = true;
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Drc::createDrc() {
  auto hInstance         = new ptFilter_Drc;
  hInstance->FFilterName = CDrcId;
  hInstance->FCaption    = tr("Dynamic range compression");
  return hInstance;
}

//==============================================================================

void ptFilter_Drc::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()
    //            Id                   Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CStrength,           ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.05,       2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CBias,               ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.01,       2,        true, true, tr("Bias"), tr("")})
    << ptCfgItem({CColorAdapt,         ptCfgItem::Slider,        0.25,       0.0,          1.0,          0.05,       2,        true, true, tr("Color adaption"), tr("")})
  ;
}

//==============================================================================

bool ptFilter_Drc::doCheckHasActiveCfg() {
  return FConfig.value(CStrength).toFloat() != 1.0f;
}

//==============================================================================

void ptFilter_Drc::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->DRC(FConfig.value(CBias).toFloat(),
              FConfig.value(CStrength).toFloat(),
              FConfig.value(CColorAdapt).toFloat() );
}

//==============================================================================

RegisterHelper DrcRegister(&ptFilter_Drc::createDrc, CDrcId);

//==============================================================================
