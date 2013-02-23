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

#include "ptFilter_Highlights.h"
#include "ptCfgItem.h"
#include <ptImage.h>

//==============================================================================

const QString CHighlightsId = "Highlights";

const QString CHighlightsR  = "HighlightsR";
const QString CHighlightsG  = "HighlightsG";
const QString CHighlightsB  = "HighlightsB";

//==============================================================================

ptFilter_Highlights::ptFilter_Highlights()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Highlights::CreateHighlights() {
  auto hInstance         = new ptFilter_Highlights;
  hInstance->FFilterName = CHighlightsId;
  hInstance->FCaption    = tr("Highlights");
  return hInstance;
}

//==============================================================================

void ptFilter_Highlights::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()                                                 //--- Combo: list of entries               ---//
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CHighlightsR,            ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Reg highlights"),    tr("Adjusts brightness of highlights in R channel")})
    << ptCfgItem({CHighlightsG,            ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Green highlights"),  tr("Adjusts brightness of highlights in G channel")})
    << ptCfgItem({CHighlightsB,            ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Blue highlights"),   tr("Adjusts brightness of highlights in B channel")})
  ;
}

//==============================================================================

bool ptFilter_Highlights::doCheckHasActiveCfg() {
  return (FConfig->getValue(CHighlightsR).toFloat() != 0.0f) ||
         (FConfig->getValue(CHighlightsG).toFloat() != 0.0f) ||
         (FConfig->getValue(CHighlightsB).toFloat() != 0.0f);
}

//==============================================================================

void ptFilter_Highlights::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  AImage->Highlights(FConfig->getValue(CHighlightsR).toFloat(),
                     FConfig->getValue(CHighlightsG).toFloat(),
                     FConfig->getValue(CHighlightsB).toFloat());
}

//==============================================================================

RegisterHelper HighlightsRegister(&ptFilter_Highlights::CreateHighlights, CHighlightsId);

//==============================================================================
