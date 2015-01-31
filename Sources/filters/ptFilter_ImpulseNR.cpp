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

#include "ptFilter_ImpulseNR.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CImpulseNRId = "ImpulseNR";

const QString CThresholdL  = "ThresholdL";
const QString CThresholdAB = "ThresholdAB";

//------------------------------------------------------------------------------

ptFilter_ImpulseNR::ptFilter_ImpulseNR():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_ImpulseNR::createImpulseNR() {
  auto hInstance         = new ptFilter_ImpulseNR;
  hInstance->FFilterName = CImpulseNRId;
  hInstance->FCaption    = tr("Impulse noise reduction");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_ImpulseNR::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CThresholdL,             ptCfgItem::Slider,        0.0,        0.0,          5.0,          0.2,        1,        true, true, tr("Lightness threshold"), tr("")})
    << ptCfgItem({CThresholdAB,            ptCfgItem::Slider,        0.0,        0.0,          5.0,          0.2,        1,        true, true, tr("Color threshold"),     tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_ImpulseNR::doCheckHasActiveCfg() {
  return
      (FConfig.value(CThresholdL).toDouble() != 0.0f) ||
      (FConfig.value(CThresholdAB).toDouble() != 0.0f);
}

//------------------------------------------------------------------------------

void ptFilter_ImpulseNR::doRunFilter(ptImage *AImage) {
  AImage->toLab();
  AImage->DenoiseImpulse(
      FConfig.value(CThresholdL).toDouble(),
      FConfig.value(CThresholdAB).toDouble());
}

//------------------------------------------------------------------------------

RegisterHelper ImpulseNRRegister(&ptFilter_ImpulseNR::createImpulseNR, CImpulseNRId);

//------------------------------------------------------------------------------
