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

#include "ptFilter_ColorEnhancement.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CColorEnhancementId = "ColorEnhancement";

const QString CShadows      = "Shadows";
const QString CHighlights   = "Highlights";

//==============================================================================

ptFilter_ColorEnhancement::ptFilter_ColorEnhancement()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ColorEnhancement::createColorEnhancement() {
  auto hInstance         = new ptFilter_ColorEnhancement;
  hInstance->FFilterName = CColorEnhancementId;
  hInstance->FCaption    = tr("Color enhancement");
  return hInstance;
}

//==============================================================================

void ptFilter_ColorEnhancement::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()
    //            Id                       Type                      Default     Min           Max           Step       Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CShadows,                ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.1,       1,        true, true, tr("Enhance shadows"), tr("")})
    << ptCfgItem({CHighlights,             ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.1,       1,        true, true, tr("Enhance highlights"), tr("")})
  ;
}

//==============================================================================

bool ptFilter_ColorEnhancement::doCheckHasActiveCfg() {
  return (FConfig->getValue(CShadows).toFloat() != 0.0f) &&
         (FConfig->getValue(CHighlights).toFloat() != 0.0f);
}

//==============================================================================

void ptFilter_ColorEnhancement::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  AImage->ColorEnhance(FConfig->getValue(CShadows).toFloat(),
                       FConfig->getValue(CHighlights).toFloat() );
}

//==============================================================================

RegisterHelper ColorEnhancementRegister(&ptFilter_ColorEnhancement::createColorEnhancement, CColorEnhancementId);

//==============================================================================
