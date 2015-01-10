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

#include "ptFilter_TextureContrast.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CTextureContrastRgbId = "TextureContrastRgb";
const QString CTextureContrastLabId = "TextureContrastLab";

const QString CStrength    = "Strength";
const QString CThreshold   = "Threshold";
const QString CSoftness    = "Softness";
const QString COpacity     = "Opacity";
const QString CEdgeControl = "EdgeControl";
const QString CMasking     = "Masking";

//------------------------------------------------------------------------------

ptFilter_TextureContrast::ptFilter_TextureContrast():
  ptFilterBase()
{
  internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_TextureContrast::createTextureContrastRgb() {
  auto hInstance         = new ptFilter_TextureContrast;
  hInstance->FFilterName = CTextureContrastRgbId;
  hInstance->FCaption    = tr("Texture contrast");
  return hInstance;
}

// -----------------------------------------------------------------------------

ptFilterBase *ptFilter_TextureContrast::createTextureContrastLab() {
  auto hInstance         = new ptFilter_TextureContrast;
  hInstance->FFilterName = CTextureContrastLabId;
  hInstance->FCaption    = tr("Texture contrast");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_TextureContrast::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.0,      -10.0,         40.0,          1.0,        1,        true, true, tr("Strength"),  tr("")})
    << ptCfgItem({CThreshold,              ptCfgItem::Slider,        20.0,       0.0,         50.0,          4.0,        1,        true, true, tr("Scale"),     tr("")})
    << ptCfgItem({CSoftness,               ptCfgItem::Slider,        0.14,       0.0,          1.0,          0.01,       2,        true, true, tr("Threshold"), tr("")})
    << ptCfgItem({COpacity,                ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        1,        true, true, tr("Opacity"),   tr("")})
    << ptCfgItem({CEdgeControl,            ptCfgItem::Slider,        0.0,        0.0,         10.0,          0.1,        1,        true, true, tr("Denoise"),   tr("")})
    << ptCfgItem({CMasking,                ptCfgItem::Slider,        100,        0,          100,           10,          0,        true, true, tr("Masking"),   tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_TextureContrast::doCheckHasActiveCfg() {
  return
      (FConfig.value(CStrength).toDouble() != 0.0) &&
      (FConfig.value(COpacity).toDouble() != 0.0);
}

//------------------------------------------------------------------------------

void ptFilter_TextureContrast::doRunFilter(ptImage *AImage) const {
  if (FFilterName == CTextureContrastRgbId) {
    AImage->toRGB();
  } else {
    AImage->toLab();
  }
  AImage->TextureContrast(
      FConfig.value(CThreshold).toDouble() * TheProcessor->m_ScaleFactor,
      FConfig.value(CSoftness).toDouble(),
      FConfig.value(CStrength).toDouble(),
      FConfig.value(COpacity).toDouble(),
      FConfig.value(CEdgeControl).toDouble(),
      FConfig.value(CMasking).toDouble() * TheProcessor->m_ScaleFactor);
}

//------------------------------------------------------------------------------

RegisterHelper TextureContrastRgbRegister(&ptFilter_TextureContrast::createTextureContrastRgb, CTextureContrastRgbId);
RegisterHelper TextureContrastLabRegister(&ptFilter_TextureContrast::createTextureContrastLab, CTextureContrastLabId);

//------------------------------------------------------------------------------
