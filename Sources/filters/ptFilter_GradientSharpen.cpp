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

#include "ptFilter_GradientSharpen.h"
#include "ui_ptFilter_GradientSharpen.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CGradientSharpenId = "GradientSharpen";

const QString CPasses        = "Passes";
const QString CStrength      = "Strength";
const QString CMicrocontrast = "Microcontrast";
const QString CHaloControl   = "HaloControl";
const QString CWeight        = "Weight";
const QString CCleanup       = "Cleanup";

//------------------------------------------------------------------------------

ptFilter_GradientSharpen::ptFilter_GradientSharpen():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_GradientSharpen::createGradientSharpen() {
  auto hInstance         = new ptFilter_GradientSharpen;
  hInstance->FFilterName = CGradientSharpenId;
  hInstance->FCaption    = tr("Gradien sharpen");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_GradientSharpen::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CPasses,                 ptCfgItem::SpinEdit,      0,          0,           10,            1,          0,        true, true, tr("Passes"), tr("")})
    << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.4,        0.0,          1.0,          0.05,       2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CMicrocontrast,          ptCfgItem::Slider,        0.0,       -0.1,          0.5,          0.05,       2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CHaloControl,            ptCfgItem::Slider,       40,          1,          200,           10,          0,        true, true, tr("Halo control"), tr("")})
    << ptCfgItem({CWeight,                 ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.05,       2,        true, true, tr("Weight"), tr("")})
    << ptCfgItem({CCleanup,                ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.1,        2,        true, true, tr("Hotpixel reduction"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_GradientSharpen::doCheckHasActiveCfg() {
  return
      (FConfig.value(CPasses).toInt() > 0) ||
      !qFuzzyIsNull(FConfig.value(CMicrocontrast).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CCleanup).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_GradientSharpen::doRunFilter(ptImage *AImage) {
  AImage->toLab();

  int gradientPasses = FConfig.value(CPasses).toInt();
  if (gradientPasses > 0) {
    AImage->GradientSharpen(gradientPasses, FConfig.value(CStrength).toDouble());
  }

  double mcStrength = FConfig.value(CMicrocontrast).toDouble();
  if (!qFuzzyIsNull(mcStrength)) {
    AImage->MLMicroContrast(
        mcStrength,
        FConfig.value(CHaloControl).toDouble(),
        FConfig.value(CWeight).toDouble());
  }

  double cleanup = FConfig.value(CCleanup).toDouble();
  if (!qFuzzyIsNull(cleanup)) {
    AImage->HotpixelReduction(cleanup);
  }
}

//------------------------------------------------------------------------------

QWidget *ptFilter_GradientSharpen::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_GradientSharpenForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper GradientSharpenRegister(&ptFilter_GradientSharpen::createGradientSharpen, CGradientSharpenId);

//------------------------------------------------------------------------------
