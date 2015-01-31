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

#include "ptFilter_WaveletDenoise.h"
#include "ui_ptFilter_WaveletDenoise.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CWaveletDenoiseId = "WaveletDenoise";

const QString CLStrength      = "LStrength";
const QString CLSoftness      = "LSoftness";
const QString CSharpness      = "Sharpness";
const QString CAnisotropy     = "Anisotropy";
const QString CGradientSmooth = "GradientSmooth";
const QString CTensorSmooth   = "TensorSmooth";
const QString CAStrength      = "AStrength";
const QString CASoftness      = "ASoftness";
const QString CBStrength      = "BStrength";
const QString CBSoftness      = "BSoftness";

//------------------------------------------------------------------------------

ptFilter_WaveletDenoise::ptFilter_WaveletDenoise():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_WaveletDenoise::createWaveletDenoise() {
  auto hInstance         = new ptFilter_WaveletDenoise;
  hInstance->FFilterName = CWaveletDenoiseId;
  hInstance->FCaption    = tr("Wavelet denoising");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_WaveletDenoise::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                Type                  Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CLStrength,       ptCfgItem::Slider,    0.0,        0.0,         10.0,          0.0,        1,        true, true, tr("L strength"),          tr("")})
    << ptCfgItem({CLSoftness,       ptCfgItem::Slider,    0.2,        0.0,          1.0,          0.01,       2,        true, true, tr("L softness"),          tr("")})
    << ptCfgItem({CSharpness,       ptCfgItem::Slider,    0.0,        0.0,          2.0,          0.1,        1,        true, true, tr("Sharpness"),           tr("")})
    << ptCfgItem({CAnisotropy,      ptCfgItem::Slider,    0.2,        0.0,          1.0,          0.01,       2,        true, true, tr("Anisotropy"),          tr("")})
    << ptCfgItem({CGradientSmooth,  ptCfgItem::Slider,    0.5,        0.0,          5.0,          0.1,        1,        true, true, tr("Gradient smoothness"), tr("")})
    << ptCfgItem({CTensorSmooth,    ptCfgItem::Slider,    1.0,        0.0,          5.0,          0.1,        1,        true, true, tr("Tensor smoothness"),   tr("")})
    << ptCfgItem({CAStrength,       ptCfgItem::Slider,    0.0,        0.0,         10.0,          0.1,        1,        true, true, tr("A strength"),          tr("")})
    << ptCfgItem({CASoftness,       ptCfgItem::Slider,    0.2,        0.0,          1.0,          0.01,       2,        true, true, tr("A softness"),          tr("")})
    << ptCfgItem({CBStrength,       ptCfgItem::Slider,    0.0,        0.0,         10.0,          0.1,        1,        true, true, tr("B strength"),          tr("")})
    << ptCfgItem({CBSoftness,       ptCfgItem::Slider,    0.2,        0.0,          1.0,          0.01,       2,        true, true, tr("B softness"),          tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_WaveletDenoise::doCheckHasActiveCfg() {
  return
      !qFuzzyIsNull(FConfig.value(CLStrength).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CAStrength).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CBStrength).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_WaveletDenoise::doRunFilter(ptImage *AImage) {
  AImage->toLab();

  const double LStrength = FConfig.value(CLStrength).toDouble();
  const double AStrength = FConfig.value(CAStrength).toDouble();
  const double BStrength = FConfig.value(CBStrength).toDouble();
  const double strengthDiv = (log(TheProcessor->m_ScaleFactor)/log(0.5)) + 1.0;

  if (!qFuzzyIsNull(LStrength)) {
    AImage->WaveletDenoise(
          ChMask_L,
          LStrength / strengthDiv,
          FConfig.value(CLSoftness).toDouble(),
          true,
          FConfig.value(CSharpness).toDouble(),
          FConfig.value(CAnisotropy).toDouble(),
          FConfig.value(CGradientSmooth).toDouble(),
          FConfig.value(CTensorSmooth).toDouble());
  }

  if (!qFuzzyIsNull(AStrength)) {
    AImage->WaveletDenoise(
        ChMask_a,
        AStrength /strengthDiv,
        FConfig.value(CASoftness).toDouble());
  }

  if (!qFuzzyIsNull(BStrength)) {
    AImage->WaveletDenoise(
        ChMask_a,
        BStrength /strengthDiv,
        FConfig.value(CBSoftness).toDouble());
  }
}

//------------------------------------------------------------------------------

QWidget *ptFilter_WaveletDenoise::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_WaveletDenoiseForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper WaveletDenoiseRegister(&ptFilter_WaveletDenoise::createWaveletDenoise, CWaveletDenoiseId);

//------------------------------------------------------------------------------
