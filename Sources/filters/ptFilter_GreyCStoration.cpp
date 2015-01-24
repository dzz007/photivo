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

#include "ptFilter_GreyCStoration.h"
#include "ui_ptFilter_GreyCStoration.h"
#include "ptCfgItem.h"
#include "ptGuiOptions.h"
#include "../ptImage.h"
#include "ptCimg.h"

#include "../ptSettings.h"

// TODO: Needed for access to m_ReportProgress. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CGreyCStorationId = "GreyCStoration";

const QString CMode = "Mode";
const QString CMaskType = "MaskType";
const QString CFastGauss = "FastGauss";
const QString CIterations = "Iterations";
const QString COpacity = "Opacity";
const QString CAmplitude = "Amplitude";
const QString CSharpness = "Sharpness";
const QString CAnisotropy = "Anisotropy";
const QString CGradientSmooth = "GradientSmooth";
const QString CTensorSmooth = "TensorSmooth";
const QString CSpacialPrec = "SpacialPrec";
const QString CAngularPrec = "AngularPrec";
const QString CValuePrec = "ValuePrec";
const QString CInterpolation = "Interpolation";

//------------------------------------------------------------------------------

ptFilter_GreyCStoration::ptFilter_GreyCStoration():
  ptFilterBase()
{
  FIsSlow = true;
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_GreyCStoration::createGreyCStoration() {
  auto hInstance         = new ptFilter_GreyCStoration;
  hInstance->FFilterName = CGreyCStorationId;
  hInstance->FCaption    = tr("GreyCStoration on L");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_GreyCStoration::doDefineControls() {
  const ptCfgItem::TComboEntryList denoiseMask ({
    {tr("All Values"), static_cast<int>(TGreyCDenoiseMask::All),      "All"},
    {tr("Shadows 1"),  static_cast<int>(TGreyCDenoiseMask::Shadows1), "Shadows1"},
    {tr("Shadows 2"),  static_cast<int>(TGreyCDenoiseMask::Shadows2), "Shadows2"},
    {tr("Shadows 3"),  static_cast<int>(TGreyCDenoiseMask::Shadows3), "Shadows3"},
    {tr("Shadows 4"),  static_cast<int>(TGreyCDenoiseMask::Shadows4), "Shadows4"},
    {tr("Shadows 5"),  static_cast<int>(TGreyCDenoiseMask::Shadows5), "Shadows5"}
  });

  const ptCfgItem::TComboEntryList interpol({
    {tr("Nearest neighbour"), static_cast<int>(TGreyCInterpol::NearestNeighbour), "NearestNeighbour"},
    {tr("Linear"),            static_cast<int>(TGreyCInterpol::Linear),           "Linear"},
    {tr("Runge-Kutta"),       static_cast<int>(TGreyCInterpol::RungeKutta),       "RungeKutta"}
  });

  FConfig.initStores(TCfgItemList()                                      //--- Combo: list of entries               ---//
                                                                         //--- Check: not available                 ---//
    //            Id               Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,           ptCfgItem::Combo,         static_cast<int>(TMaskedFilterModes::Disabled),
                                                             pt::ComboEntries::MaskedFilterModes,                          true, true, tr("Mode"), tr("")})
    << ptCfgItem({CMaskType,       ptCfgItem::Combo,         static_cast<int>(TGreyCDenoiseMask::Shadows3), denoiseMask,   true, true, tr("Denoise mask"), tr("")})
    << ptCfgItem({CFastGauss,      ptCfgItem::Check,         true,                                                         true, true, tr("Fast Gaussian approximation"), tr("")})
    << ptCfgItem({CIterations,     ptCfgItem::SpinEdit,      1,          1,           10,            1,          0,        true, true, tr("Iterations"),  tr("")})
    << ptCfgItem({COpacity,        ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.1,        1,        true, true, tr("Opacity"),     tr("")})
    << ptCfgItem({CAmplitude,      ptCfgItem::Slider,       45.0,        0.0,        500.0,          5.0,        1,        true, true, tr("Amplitude"),   tr("")})
    << ptCfgItem({CSharpness,      ptCfgItem::Slider,        0.6,        0.0,          2.0,          0.05,       2,        true, true, tr("Sharpness"),   tr("")})
    << ptCfgItem({CAnisotropy,     ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.05,       2,        true, true, tr("Anisotropy"),  tr("")})
    << ptCfgItem({CGradientSmooth, ptCfgItem::Slider,        1.0,        0.0,         20.0,          0.1,        1,        true, true, tr("Gradient smoothness"), tr("alpha standard deviation of the gradient blur")})
    << ptCfgItem({CTensorSmooth,   ptCfgItem::Slider,        1.0,        0.0,         20.0,          0.1,        1,        true, true, tr("Tensor smoothness"),   tr("sigma standard deviation of the gradient blur")})
    << ptCfgItem({CSpacialPrec,    ptCfgItem::Slider,        0.8,        0.0,          2.0,          0.01,       2,        true, true, tr("Spacial precision"),   tr("dl spatial discretization")})
    << ptCfgItem({CAngularPrec,    ptCfgItem::Slider,       30,          0,          180,            1,          0,        true, true, tr("Angular precision"),   tr("da angular discretization")})
    << ptCfgItem({CValuePrec,      ptCfgItem::Slider,        2.0,        0.0,          5.0,          0.1,        1,        true, true, tr("Value precision"),     tr("precision of the diffusion process")})
    << ptCfgItem({CInterpolation,  ptCfgItem::Combo,        static_cast<int>(TGreyCInterpol::NearestNeighbour), interpol,  true, true, tr("Interpolation"),       tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_GreyCStoration::doCheckHasActiveCfg() {
  auto mode = static_cast<TMaskedFilterModes>(FConfig.value(CMode).toInt());

  return
      (mode >= TMaskedFilterModes::AlwaysOn) ||
      ((mode == TMaskedFilterModes::FinalRun) && Settings->GetInt("FullOutput"));
}

//------------------------------------------------------------------------------

void ptFilter_GreyCStoration::doRunFilter(ptImage *AImage) const {
  AImage->toLab();

  if (static_cast<TMaskedFilterModes>(FConfig.value(CMode).toInt()) == TMaskedFilterModes::ShowMask) {
    ptCimgEdgeTensors(
        AImage,
        FConfig.value(CSharpness).toDouble(),
        FConfig.value(CAnisotropy).toDouble(),
        FConfig.value(CGradientSmooth).toDouble(),
        FConfig.value(CTensorSmooth).toDouble(),
        0.0,
        static_cast<TGreyCDenoiseMask>(FConfig.value(CMaskType).toInt()));
  } else {
    ptGreycStorationLab(
          AImage,
          TheProcessor->m_ReportProgress,
          FConfig.value(CIterations).toInt(),
          FConfig.value(CAmplitude).toDouble(),
          FConfig.value(CSharpness).toDouble(),
          FConfig.value(CAnisotropy).toDouble(),
          FConfig.value(CGradientSmooth).toDouble(),
          FConfig.value(CTensorSmooth).toDouble(),
          FConfig.value(CSpacialPrec).toDouble(),
          FConfig.value(CAngularPrec).toInt(),
          FConfig.value(CValuePrec).toDouble(),
          static_cast<TGreyCInterpol>(FConfig.value(CInterpolation).toInt()),
          FConfig.value(CFastGauss).toBool(),
          static_cast<TGreyCDenoiseMask>(FConfig.value(CMaskType).toInt()),
          FConfig.value(COpacity).toDouble());
  }
}

//------------------------------------------------------------------------------

QWidget *ptFilter_GreyCStoration::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_GreyCStorationForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper GreyCStorationRegister(&ptFilter_GreyCStoration::createGreyCStoration, CGreyCStorationId);

//------------------------------------------------------------------------------
