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

#include "ptFilter_GradualBlur.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CGradualBlurId = "GradualBlur";

const QString CMode       = "Mode";
const QString CShape      = "Shape";
const QString CRadius     = "Radius";
const QString CLowerLevel = "LowerLevel";
const QString CUpperLevel = "UpperLevel";
const QString CSoftness   = "Softness";
const QString CAngle      = "Angle";
const QString CRoundness  = "Roundness";
const QString CCenterX    = "CenterX";
const QString CCenterY    = "CenterY";

//------------------------------------------------------------------------------

ptFilter_GradualBlur::ptFilter_GradualBlur():
  ptFilterBase()
{
  FIsSlow = true;
  FHelpUri = "http://photivo.org/manual/tabs/eyecandy#gradual_blur";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_GradualBlur::createGradualBlur() {
  auto instance         = new ptFilter_GradualBlur;
  instance->FFilterName = CGradualBlurId;
  instance->FCaption    = tr("Gradual blur");
  return instance;
}

//------------------------------------------------------------------------------

void ptFilter_GradualBlur::doDefineControls() {
  const ptCfgItem::TComboEntryList modes({
    {tr("Linear"),        static_cast<int>(TGradualBlurMode::Linear),       "Linear"},
    {tr("Vignette"),      static_cast<int>(TGradualBlurMode::Vignette),     "Vignette"},
    {tr("Linear mask"),   static_cast<int>(TGradualBlurMode::LinearMask),   "LinearMask"},
    {tr("Vignette mask"), static_cast<int>(TGradualBlurMode::VignetteMask), "VignetteMask"},
  });

  FConfig.initStores(TCfgItemList()                                   //--- Combo: list of entries               ---//
    //            Id            Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,        ptCfgItem::Combo,         static_cast<int>(TGradualBlurMode::Linear), modes,            false, true, tr("Mode"), tr("Mode")})
    << ptCfgItem({CShape,       ptCfgItem::Combo,         static_cast<int>(TVignetteShape::Circle),
                                                          pt::ComboEntries::VignetteShapes,                             true,  true, tr("Shape"), tr("shape of the vignette")})
    << ptCfgItem({CRadius,      ptCfgItem::Slider,        0,          0,            200,          5,          0,        true,  true, tr("Radius"), tr("")})
    << ptCfgItem({CLowerLevel,  ptCfgItem::Slider,        0.3,        0.0,          10.0,         0.1,        2,        true,  true, tr("Lower level"), tr("")})
    << ptCfgItem({CUpperLevel,  ptCfgItem::Slider,        1.5,        0.0,          10.0,         0.1,        2,        true,  true, tr("Upper level"), tr("")})
    << ptCfgItem({CSoftness,    ptCfgItem::Slider,        0.01,       0.0,          1.0,          0.001,      3,        true,  true, tr("Softness"), tr("")})
    << ptCfgItem({CAngle,       ptCfgItem::Slider,        0,         -180,          180,          5,          0,        true,  true, tr("Angle"), tr("")})
    << ptCfgItem({CRoundness,   ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.1,        2,        true,  true, tr("Roundness"), tr("")})
    << ptCfgItem({CCenterX,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true,  true, tr("Horizontal center"), tr("")})
    << ptCfgItem({CCenterY,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true,  true, tr("vertical center"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_GradualBlur::doCheckHasActiveCfg() {
  return !qFuzzyIsNull(FConfig.value(CRadius).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_GradualBlur::doRunFilter(ptImage* AImage) const {
  AImage->toRGB();
  AImage->GradualBlur(
      static_cast<TGradualBlurMode>(FConfig.value(CMode).toInt()),
      FConfig.value(CRadius).toDouble() * TheProcessor->m_ScaleFactor,
      FConfig.value(CLowerLevel).toDouble(),
      FConfig.value(CUpperLevel).toDouble(),
      FConfig.value(CSoftness).toDouble(),
      FConfig.value(CAngle).toDouble(),
      static_cast<TVignetteShape>(FConfig.value(CShape).toInt()),
      FConfig.value(CRoundness).toDouble(),
      FConfig.value(CCenterX).toDouble(),
        FConfig.value(CCenterY).toDouble() );
}

//------------------------------------------------------------------------------

QWidget* ptFilter_GradualBlur::doCreateGui() {
  auto guiBody = new QWidget;

  FForm.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  connect(FForm.Mode,
          SIGNAL(valueChanged(QString,QVariant)),
          SLOT(onModeValueChanged(QString,QVariant)));

  return guiBody;
}

// -----------------------------------------------------------------------------

void ptFilter_GradualBlur::doUpdateGui() {
  auto mode = static_cast<TGradualBlurMode>(FConfig.value(CMode).toInt());
  bool isVignette =
      (mode == TGradualBlurMode::Vignette) ||
      (mode == TGradualBlurMode::VignetteMask);

  FForm.Shape->setVisible(isVignette);
  FForm.Angle->setVisible(!isVignette);
  FForm.Roundness->setVisible(isVignette);
  FForm.CenterX->setVisible(isVignette);
  FForm.CenterY->setVisible(isVignette);
}

// -----------------------------------------------------------------------------
// Manage visibility of some sliders. Depends on mode.
void ptFilter_GradualBlur::onModeValueChanged(QString AId, QVariant ANewValue) {
  FConfig.setValue(AId, ANewValue);
  this->doUpdateGui();
}

//------------------------------------------------------------------------------

RegisterHelper GradualBlurRegister(&ptFilter_GradualBlur::createGradualBlur, CGradualBlurId);

//------------------------------------------------------------------------------
