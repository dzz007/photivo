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

#include "ptFilter_ColorBoost.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CColorBoostId = "ColorBoost";

const QString CStrengthA    = "StrengthA";
const QString CStrengthB    = "StrengthB";

//==============================================================================

ptFilter_ColorBoost::ptFilter_ColorBoost()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ColorBoost::createColorBoost() {
  auto hInstance         = new ptFilter_ColorBoost;
  hInstance->FFilterName = CColorBoostId;
  hInstance->FCaption    = tr("Color boost");
  return hInstance;
}

//==============================================================================

void ptFilter_ColorBoost::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CStrengthA,              ptCfgItem::Slider,        1.0,        0.0,          3.0,          0.1 ,       1,        true, true, tr("Strength in a"), tr("")})
    << ptCfgItem({CStrengthB,              ptCfgItem::Slider,        1.0,        0.0,          3.0,          0.1,        1,        true, true, tr("Strength in b"), tr("")})
  );
}

//==============================================================================

bool ptFilter_ColorBoost::doCheckHasActiveCfg() {
  return (FConfig.value(CStrengthA).toFloat() != 1.0f) ||
         (FConfig.value(CStrengthB).toFloat() != 1.0f);
}

//==============================================================================

void ptFilter_ColorBoost::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->ColorBoost(FConfig.value(CStrengthA).toFloat(),
                     FConfig.value(CStrengthB).toFloat());
}

//==============================================================================

RegisterHelper ColorBoostRegister(&ptFilter_ColorBoost::createColorBoost, CColorBoostId);

//==============================================================================
