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

#include "ptFilter_GradualOverlay.h"
#include "ui_ptFilter_GradualOverlay.h"
#include "ptCfgItem.h"
#include "ptFilterUids.h"
#include "../ptConstants.h"
#include "../ptImage.h"
#include "../ptSettings.h"
#include <QColor>

//------------------------------------------------------------------------------

const QString CGradualOverlayId = "GradualOverlay";

const QString CMode       = "Mode";
const QString CColor      = "Color";
const QString CStrength   = "Strength";
const QString CAngle      = "Angle";
const QString CLowerLevel = "LowerLevel";
const QString CUpperLevel = "UpperLevel";
const QString CSoftness   = "Softness";

//------------------------------------------------------------------------------

ptFilter_GradualOverlay::ptFilter_GradualOverlay():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";

  // internalInit() cannot be done here because controls’ values depend on
  // the concrete filter instance. We need the unique name which is not set yet.
  // Init is performed by doAfterInit() instead.
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_GradualOverlay::createGradualOverlay() {
  auto instance         = new ptFilter_GradualOverlay;
  instance->FFilterName = CGradualOverlayId;
  instance->FCaption    = tr("Gradual overlay");
  return instance;
}

// -----------------------------------------------------------------------------

void ptFilter_GradualOverlay::doAfterInit() {
  this->internalInit();
}

//------------------------------------------------------------------------------

void ptFilter_GradualOverlay::doDefineControls() {
  const ptCfgItem::TComboEntryList modes({
    {tr("Disabled"),     static_cast<int>(TOverlayMode::Disabled),    "Disabled"},
    {tr("Softlight"),    static_cast<int>(TOverlayMode::Softlight),   "Softlight"},
    {tr("Multiply"),     static_cast<int>(TOverlayMode::Multiply),    "Multiply"},
    {tr("Screen"),       static_cast<int>(TOverlayMode::Screen),      "Screen"},
    {tr("Gamma dark"),   static_cast<int>(TOverlayMode::GammaDark),   "GammaDark"},
    {tr("Gamma bright"), static_cast<int>(TOverlayMode::GammaBright), "GammaBright"},
    {tr("Color burn"),   static_cast<int>(TOverlayMode::ColorBurn),   "ColorBurn"},
    {tr("Color dodge"),  static_cast<int>(TOverlayMode::ColorDodge),  "ColorDodge"},
    {tr("Normal"),       static_cast<int>(TOverlayMode::Normal),      "Normal"},
    {tr("Replace"),      static_cast<int>(TOverlayMode::Replace),     "Replace"},
    {tr("Show Mask"),    static_cast<int>(TOverlayMode::ShowMask),    "ShowMask"},
  });

  QColor defaultColor(0, 0, 0);
  if (this->uniqueName() == Fuid::GradualOverlay2_EyeCandy) {
    defaultColor.setRgb(255, 200, 0);
  }

  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id            Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,        ptCfgItem::Combo,         static_cast<int>(TOverlayMode::Disabled), modes,              true, true, tr("Mode"), tr("overlay mode")})
    << ptCfgItem({CColor,       ptCfgItem::ColorSelectButton, defaultColor,                                             true, true, tr("overlay color")})
    << ptCfgItem({CStrength,    ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CAngle,       ptCfgItem::Slider,        0,         -180,          180,          5,          0,        true, true, tr("Angle"), tr("")})
    << ptCfgItem({CLowerLevel,  ptCfgItem::Slider,        0.5,        0.0,          3.0,          0.1,        2,        true, true, tr("Lower level"), tr("")})
    << ptCfgItem({CUpperLevel,  ptCfgItem::Slider,        1.0,        0.0,          3.0,          0.1,        2,        true, true, tr("Upper level"), tr("")})
    << ptCfgItem({CSoftness,    ptCfgItem::Slider,        0.15,       0.0,          1.0,          0.1,        2,        true, true, tr("Softness"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_GradualOverlay::doCheckHasActiveCfg() {
  return
      (static_cast<TOverlayMode>(FConfig.value(CMode).toInt()) != TOverlayMode::Disabled) &&
      !qFuzzyIsNull(FConfig.value(CStrength).toDouble());
}

//------------------------------------------------------------------------------

namespace pt {
  namespace detail {
    inline uint16_t adjustedPow(int ABase, double AExponent) {
      return static_cast<uint16_t>(0xffff * pow(ABase/255.0, AExponent));
    }
  }
}

// -----------------------------------------------------------------------------

void ptFilter_GradualOverlay::doRunFilter(ptImage* AImage) {
  const double inputPowerFactor = Settings->GetDouble("InputPowerFactor");
  const QColor color = FConfig.value(CColor).value<QColor>();

  AImage->toRGB();
  AImage->GradualOverlay(
      pt::detail::adjustedPow(color.red(), inputPowerFactor),
      pt::detail::adjustedPow(color.green(), inputPowerFactor),
      pt::detail::adjustedPow(color.blue(), inputPowerFactor),
      static_cast<TOverlayMode>(FConfig.value(CMode).toInt()),
      FConfig.value(CStrength).toDouble(),
      FConfig.value(CAngle).toDouble(),
      FConfig.value(CLowerLevel).toDouble(),
      FConfig.value(CUpperLevel).toDouble(),
      FConfig.value(CSoftness).toDouble() );
}

//------------------------------------------------------------------------------

QWidget* ptFilter_GradualOverlay::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_GradualOverlayForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper GradualOverlayRegister(&ptFilter_GradualOverlay::createGradualOverlay, CGradualOverlayId);

//------------------------------------------------------------------------------
