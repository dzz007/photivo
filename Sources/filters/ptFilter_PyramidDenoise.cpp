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

#include "ptFilter_PyramidDenoise.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CPyramidDenoiseId = "PyramidDenoise";

const QString CLumaStrength   = "LumaStrength";
const QString CChromaStrength = "ChromaStrength";
const QString CGamma          = "Gamma";
const QString CLevels         = "Levels";

//------------------------------------------------------------------------------

ptFilter_PyramidDenoise::ptFilter_PyramidDenoise():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_PyramidDenoise::createPyramidDenoise() {
  auto hInstance         = new ptFilter_PyramidDenoise;
  hInstance->FFilterName = CPyramidDenoiseId;
  hInstance->FCaption    = tr("Pyramid denoising");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_PyramidDenoise::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CLumaStrength,           ptCfgItem::Slider,        0,          0,            150,          5,          0,        true, true, tr("Lightness strength"), tr("affects L channel")})
    << ptCfgItem({CChromaStrength,         ptCfgItem::Slider,        0,          0,            150,          5,          0,        true, true, tr("Color strength"), tr("affects a and b channels")})
    << ptCfgItem({CGamma,                  ptCfgItem::Slider,        2.0,        1.0,          4.0,          0.1,        1,        true, true, tr("Gamma"), tr("")})
    << ptCfgItem({CLevels,                 ptCfgItem::SpinEdit,      0,          3,            7,            1,          0,        true, true, tr("Levels"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_PyramidDenoise::doCheckHasActiveCfg() {
  return
      (FConfig.value(CLumaStrength).toInt() != 0) ||
      (FConfig.value(CChromaStrength).toInt() != 0);
}

//------------------------------------------------------------------------------

void ptFilter_PyramidDenoise::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->dirpyrLab_denoise(
      static_cast<int>(FConfig.value(
          CLumaStrength).toDouble() /
          pow(3.0, (log(TheProcessor->m_ScaleFactor) / log(0.5)))),
      FConfig.value(CChromaStrength).toInt(),
      FConfig.value(CGamma).toDouble() / 3.0,
      FConfig.value(CLevels).toInt());
}

//------------------------------------------------------------------------------

RegisterHelper PyramidDenoiseRegister(&ptFilter_PyramidDenoise::createPyramidDenoise, CPyramidDenoiseId);

//------------------------------------------------------------------------------
