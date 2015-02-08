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

#include "ptFilter_ReinhardBrighten.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CReinhardBrightenId = "ReinhardBrighten";

const QString CEnabled     = "Enabled";
const QString CBrightness  = "Brightness";
const QString CChroma      = "Chroma";
const QString CLightTweak  = "LightTweak";

//==============================================================================

ptFilter_ReinhardBrighten::ptFilter_ReinhardBrighten()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ReinhardBrighten::createReinhardBrighten() {
  auto hInstance         = new ptFilter_ReinhardBrighten;
  hInstance->FFilterName = CReinhardBrightenId;
  hInstance->FCaption    = tr("Reinhard brighten");
  return hInstance;
}

//==============================================================================

void ptFilter_ReinhardBrighten::doDefineControls() {
  FConfig.initStores(TCfgItemList()
                                                                                 //--- Check: not available                 ---//
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CEnabled,                ptCfgItem::Check,         false,                                                        true, true, tr("Enable"), tr("")})
    << ptCfgItem({CBrightness,             ptCfgItem::Slider,        -10,        -90,          10,              2,       0,        true, true, tr("Brightness"), tr("")})
    << ptCfgItem({CChroma,                 ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.10,       2,        true, true, tr("Chrominance"), tr("")})
    << ptCfgItem({CLightTweak,             ptCfgItem::Slider,        0.2,        0.0,          1.0,          0.05,       2,        true, true, tr("Lightness tweak"), tr("")})
  );
}

//==============================================================================

bool ptFilter_ReinhardBrighten::doCheckHasActiveCfg() {
  return FConfig.value(CEnabled).toBool();
}

//==============================================================================

void ptFilter_ReinhardBrighten::doRunFilter(ptImage *AImage) {
  AImage->toRGB();
  AImage->Reinhard05(FConfig.value(CBrightness).toFloat(),
                     FConfig.value(CChroma).toFloat(),
                     FConfig.value(CLightTweak).toFloat() );
}

//==============================================================================

RegisterHelper ReinhardBrightenRegister(&ptFilter_ReinhardBrighten::createReinhardBrighten, CReinhardBrightenId);

//==============================================================================
