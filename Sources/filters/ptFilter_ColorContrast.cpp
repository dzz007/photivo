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

#include "ptFilter_ColorContrast.h"
#include "ptCfgItem.h"
#include <ptImage.h>

//==============================================================================

const QString CColorContrastId = "ColorContrast";

const QString COpacity      = "Opacity";
const QString CRadius       = "Radius";
const QString CStrength     = "Strength";
const QString CHaloControl  = "HaloControl";

//==============================================================================

ptFilter_ColorContrast::ptFilter_ColorContrast()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ColorContrast::CreateColorContrast() {
  auto hInstance         = new ptFilter_ColorContrast;
  hInstance->FFilterName = CColorContrastId;
  hInstance->FCaption    = tr("Color contrast");
  return hInstance;
}

//==============================================================================

void ptFilter_ColorContrast::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()                                      //--- Combo: list of entries               ---//
    //            Id             Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COpacity,      ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Opacity"),    tr("")})
    << ptCfgItem({CRadius,       ptCfgItem::Slider,        100,          0,          2000,         50,         0,        true, true, tr("Radius"),  tr("")})
    << ptCfgItem({CStrength,     ptCfgItem::Slider,        4.0,        0.0,         20.0,          1.0,        1,        true, true, tr("Strength"),   tr("")})
    << ptCfgItem({CHaloControl,  ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        1,        true, true, tr("Halo control"),   tr("")})
  ;
}

//==============================================================================

bool ptFilter_ColorContrast::doCheckHasActiveCfg() {
  return FConfig.value(COpacity).toDouble() != 0.0;
}

//==============================================================================

void ptFilter_ColorContrast::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->Colorcontrast(FConfig.value(CRadius).toInt(),
                        FConfig.value(CStrength).toDouble(),
                        FConfig.value(COpacity).toDouble(),
                        FConfig.value(CHaloControl).toDouble());
}

//==============================================================================

RegisterHelper ColorContrastRegister(&ptFilter_ColorContrast::CreateColorContrast, CColorContrastId);

//==============================================================================
