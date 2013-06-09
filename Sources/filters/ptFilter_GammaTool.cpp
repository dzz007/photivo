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

#include "ptFilter_GammaTool.h"
#include "ptCfgItem.h"
#include <ptImage.h>

//==============================================================================

const QString CGammaToolId = "GammaTool";

const QString CGamma      = "Gamma";
const QString CLinearity  = "Linearity";

//==============================================================================

ptFilter_GammaTool::ptFilter_GammaTool()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_GammaTool::CreateGammaTool() {
  auto hInstance         = new ptFilter_GammaTool;
  hInstance->FFilterName = CGammaToolId;
  hInstance->FCaption    = tr("Gamma adjustment");
  return hInstance;
}

//==============================================================================

void ptFilter_GammaTool::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CGamma,                 ptCfgItem::Slider,        1.0,        0.1,          2.0,          0.01,       2,        true, true, tr("Gamma"),    tr("")})
    << ptCfgItem({CLinearity,             ptCfgItem::Slider,        0.0,        0.0,          0.99,         0.01,       2,        true, true, tr("Linearity"),  tr("")})
  );
}

//==============================================================================

bool ptFilter_GammaTool::doCheckHasActiveCfg() {
  return FConfig.value(CGamma).toFloat() != 1.0f;
}

//==============================================================================

void ptFilter_GammaTool::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  AImage->GammaTool(FConfig.value(CGamma).toFloat(),
                    FConfig.value(CLinearity).toFloat());
}

//==============================================================================

RegisterHelper GammaToolRegister(&ptFilter_GammaTool::CreateGammaTool, CGammaToolId);

//==============================================================================
