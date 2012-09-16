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

#include "ptFilter_Normalization.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CNormalizationId = "Normalization";

const QString COpacity     = "Opacity";

//==============================================================================

ptFilter_Normalization::ptFilter_Normalization()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Normalization::createNormalization() {
  auto hInstance         = new ptFilter_Normalization;
  hInstance->FFilterName = CNormalizationId;
  hInstance->FCaption    = tr("Normalization");
  return hInstance;
}

//==============================================================================

void ptFilter_Normalization::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COpacity,                ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.05,       2,        true, true, tr("Opacity"), tr("")})
  ;
}

//==============================================================================

bool ptFilter_Normalization::doCheckHasActiveCfg() {
  return FConfig->getValue(COpacity).toFloat() != 0.0f;
}

//==============================================================================

void ptFilter_Normalization::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  AImage->ptGMNormalize(FConfig->getValue(COpacity).toDouble());
}

//==============================================================================

RegisterHelper NormalizationRegister(&ptFilter_Normalization::createNormalization, CNormalizationId);

//==============================================================================
