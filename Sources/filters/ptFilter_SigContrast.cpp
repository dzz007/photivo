/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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

#include "ptFilter_SigContrast.h"
#include "ptCfgItem.h"
#include "ptFilterConfig.h"
#include "../ptImage.h"
#include "../ptInfo.h"

const QString CSigContrastLabId = "SigContrastLab";
const QString CSigContrastRgbId = "SigContrastRgb";

const QString CStrengthId  = "Strength";
const QString CThresholdId = "Threshold";

//==============================================================================

ptFilter_SigContrast::ptFilter_SigContrast(const QString &AFilterName,
                                           const TColorSpace AColorSpace,
                                           const QString &AGuiCaption)
: ptFilterBase()
{
  FFilterName = AFilterName;
  FColorSpace = AColorSpace;
  FCaption    = AGuiCaption;

  // Needs to be done in the derived constructor! Base ctor cannot call virtual methods.
  internalInit();
}

//==============================================================================

void ptFilter_SigContrast::doDefineControls() {
  // Lab and RGB versions have slightly different default values
  auto ThreshDef = (FColorSpace == TColorSpace::Lab) ? 0.50 : 0.30;

  /* NOTE: No floats for now. The old custom widgets enforce double and it’s too much of
     a hassle to change. */
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CStrengthId,             ptCfgItem::Slider,        0.0,        -20.0,        20.0,         0.5,        1,  true,  true,  tr("Strength"), tr("")})
    << ptCfgItem({CThresholdId,            ptCfgItem::Slider,        ThreshDef,  0.05,         0.95,         0.05,       2,  true,  true,  tr("Threshold"), tr("")})
  );
}

//==============================================================================

bool ptFilter_SigContrast::doCheckHasActiveCfg() {
  // The filter is off when the contrast slider sits at 0.0. Position of other
  // controls does not matter.
  return (FConfig.value(CStrengthId).toFloat() != 0.0f);
}

//==============================================================================

void ptFilter_SigContrast::doRunFilter(ptImage *AImage) const {
  if (FColorSpace == TColorSpace::Rgb) {
    if (!(AImage->m_ColorSpace == ptSpace_Profiled))
      AImage->toRGB();
    AImage->SigmoidalContrast(FConfig.value(CStrengthId).toFloat(),
                              FConfig.value(CThresholdId).toFloat(),
                              ChMask_RGB);
  } else if (FColorSpace == TColorSpace::Lab) {
    AImage->toLab();
    AImage->SigmoidalContrast(FConfig.value(CStrengthId).toFloat(),
                              FConfig.value(CThresholdId).toFloat(),
                              ChMask_L);
  }
}

//==============================================================================

/* static */
ptFilterBase *ptFilter_SigContrast::CreateLabContrast() {
  ptFilter_SigContrast *hInstance = new ptFilter_SigContrast(CSigContrastLabId,
                                                             TColorSpace::Lab,
                                                             tr("Lightness contrast"));
  return hInstance;
}

//==============================================================================

/* static */
ptFilterBase *ptFilter_SigContrast::CreateRgbContrast() {
  ptFilter_SigContrast *hInstance = new ptFilter_SigContrast(CSigContrastRgbId,
                                                             TColorSpace::Rgb,
                                                             tr("Sigmoidal contrast"));
  return hInstance;
}

//==============================================================================

RegisterHelper LabContrastRegister(&ptFilter_SigContrast::CreateLabContrast, CSigContrastLabId);
RegisterHelper RGBContrastRegister(&ptFilter_SigContrast::CreateRgbContrast, CSigContrastRgbId);

//==============================================================================
