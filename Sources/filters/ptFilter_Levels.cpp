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

#include "ptFilter_Levels.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptSettings.h"

//==============================================================================

const QString CLevelsRgbId = "LevelsRgb";
const QString CLevelsLabId = "LevelsLab";

const QString CBlackpoint   = "Blackpoint";
const QString CWhitepoint   = "Whitepoint";

//==============================================================================

ptFilter_Levels::ptFilter_Levels(const QString &AFilterName, TColorSpace AColorSpace)
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  FFilterName = AFilterName;
  FColorSpace = AColorSpace;
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Levels::createLevelsRgb() {
  auto hInstance         = new ptFilter_Levels(CLevelsRgbId, TColorSpace::Rgb);
  hInstance->FCaption    = tr("Levels");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_Levels::createLevelsLab() {
  auto hInstance         = new ptFilter_Levels(CLevelsLabId, TColorSpace::Lab);
  hInstance->FCaption    = tr("Levels");
  return hInstance;
}

//==============================================================================

void ptFilter_Levels::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CBlackpoint,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.002,      3,        true, true, tr("Blackpoint"), tr("")})
    << ptCfgItem({CWhitepoint,             ptCfgItem::Slider,        1.0,        0.0,          2.0,          0.002,      3,        true, true, tr("Whitepoint"), tr("")})
  ;
}

//==============================================================================

bool ptFilter_Levels::doCheckHasActiveCfg() {
  return (FConfig->getValue(CBlackpoint).toFloat() != 0.0f) &&
         (FConfig->getValue(CWhitepoint).toFloat() != 1.0f);
}

//==============================================================================

void ptFilter_Levels::doRunFilter(ptImage *AImage) const {
  if (FColorSpace == TColorSpace::Rgb) {
    float hBlackP = FConfig->getValue(CBlackpoint).toFloat();
    float hWhiteP = FConfig->getValue(CWhitepoint).toFloat();

    hBlackP = (hBlackP > 0.0f) ? pow(  hBlackP, Settings->GetDouble("InputPowerFactor")) :
                                 -pow(-hBlackP, Settings->GetDouble("InputPowerFactor"));
    hWhiteP = pow(hWhiteP, Settings->GetDouble("InputPowerFactor"));

    AImage->toRGB();
    AImage->Levels(hBlackP, hWhiteP);


  } else if (FColorSpace == TColorSpace::Lab) {
    AImage->toLab();
    AImage->Levels(FConfig->getValue(CBlackpoint).toFloat(),
                   FConfig->getValue(CWhitepoint).toFloat() );
  }
}

//==============================================================================

RegisterHelper LevelsRgbRegister(&ptFilter_Levels::createLevelsRgb, CLevelsRgbId);
RegisterHelper LevelsLabRegister(&ptFilter_Levels::createLevelsLab, CLevelsLabId);

//==============================================================================
