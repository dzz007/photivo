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
#include "ptFilterUids.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptInfo.h"

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
  // internalInit() cannot be done here because controls’ values depend on
  // the concrete filter instance. We need the unique name which is not set yet.
  // Init is performed by doAfterInit() instead.
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

// -----------------------------------------------------------------------------

void ptFilter_TextureContrast::doAfterInit() {
  this->internalInit();
}

//------------------------------------------------------------------------------

void ptFilter_TextureContrast::doDefineControls() {
  QString strengthCaption  {tr("Strength")};
  QString scaleCaption     {tr("Scale")};
  QString thresholdCaption {tr("Threshold")};
  QString opacityCaption   {tr("Opacity")};
  QString denoiseCaption   {tr("Denoise")};
  QString maskingCaption   {tr("Masking")};

  if (this->uniqueName() == Fuid::TextureContrast_RGB) {
    FConfig.initStores(TCfgItemList()
      //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
      << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.0,      -10.0,         40.0,          1.0,        1,        true, true, strengthCaption,  tr("")})
      << ptCfgItem({CThreshold,              ptCfgItem::Slider,        20.0,       0.0,         50.0,          4.0,        1,        true, true, scaleCaption,     tr("")})
      << ptCfgItem({CSoftness,               ptCfgItem::Slider,        0.14,       0.0,          1.0,          0.01,       2,        true, true, thresholdCaption, tr("")})
      << ptCfgItem({COpacity,                ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        1,        true, true, opacityCaption,   tr("")})
      << ptCfgItem({CEdgeControl,            ptCfgItem::Slider,        0.0,        0.0,         10.0,          0.1,        1,        true, true, denoiseCaption,   tr("")})
      << ptCfgItem({CMasking,                ptCfgItem::Slider,        100,        0,          100,           10,          0,        true, true, maskingCaption,   tr("")})
    );
  } else if (this->uniqueName() == Fuid::TextureContrast1_LabCC) {
    FConfig.initStores(TCfgItemList()
      //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
      << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.0,      -10.0,         40.0,          1.0,        1,        true, true, strengthCaption,  tr("")})
      << ptCfgItem({CThreshold,              ptCfgItem::Slider,        20.0,       0.0,         50.0,          4.0,        1,        true, true, scaleCaption,     tr("")})
      << ptCfgItem({CSoftness,               ptCfgItem::Slider,        0.14,       0.0,          1.0,          0.01,       2,        true, true, thresholdCaption, tr("")})
      << ptCfgItem({COpacity,                ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.1,        1,        true, true, opacityCaption,   tr("")})
      << ptCfgItem({CEdgeControl,            ptCfgItem::Slider,        0.0,        0.0,         10.0,          0.1,        1,        true, true, denoiseCaption,   tr("")})
      << ptCfgItem({CMasking,                ptCfgItem::Slider,        100,        0,          100,           10,          0,        true, true, maskingCaption,   tr("")})
    );
  } else if (this->uniqueName() == Fuid::TextureContrast2_LabCC) {
    FConfig.initStores(TCfgItemList()
      //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
      << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.0,      -10.0,         40.0,          1.0,        1,        true, true, strengthCaption,  tr("")})
      << ptCfgItem({CThreshold,              ptCfgItem::Slider,       100.0,       0.0,        400.0,          4.0,        1,        true, true, scaleCaption,     tr("")})
      << ptCfgItem({CSoftness,               ptCfgItem::Slider,        0.14,       0.0,          1.0,          0.01,       2,        true, true, thresholdCaption, tr("")})
      << ptCfgItem({COpacity,                ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.1,        1,        true, true, opacityCaption,   tr("")})
      << ptCfgItem({CEdgeControl,            ptCfgItem::Slider,        0.0,        0.0,         10.0,          0.1,        1,        true, true, denoiseCaption,   tr("")})
      << ptCfgItem({CMasking,                ptCfgItem::Slider,        100,        0,          100,           10,          0,        true, true, maskingCaption,   tr("")})
    );
  } else {
    GInfo->Raise("Unexpected unique name: " + this->uniqueName(), AT);
  }
}

//------------------------------------------------------------------------------

bool ptFilter_TextureContrast::doCheckHasActiveCfg() {
  return
      (FConfig.value(CStrength).toDouble() != 0.0) &&
      (FConfig.value(COpacity).toDouble() != 0.0);
}

//------------------------------------------------------------------------------

void ptFilter_TextureContrast::doRunFilter(ptImage *AImage) {
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
