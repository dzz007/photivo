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

#include "ptFilter_ColorTone.h"
#include "ui_ptFilter_ColorTone.h"
#include "ptCfgItem.h"
#include "ptFilterUids.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"
#include "../ptSettings.h"
#include <QColor>

//------------------------------------------------------------------------------

const QString CColorToneId = "ColorTone";

const QString CMaskType   = "MaskType";
const QString CColor      = "Color";
const QString CStrength   = "Strength";
const QString CLowerLimit = "LowerLimit";
const QString CUpperLimit = "UpperLimit";
const QString CSoftness   = "Softness";

//------------------------------------------------------------------------------

ptFilter_ColorTone::ptFilter_ColorTone():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";

  // internalInit() cannot be done here because controls’ values depend on
  // the concrete filter instance. We need the unique name which is not set yet.
  // Init is performed by doAfterInit() instead.
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_ColorTone::createColorTone() {
  auto instance         = new ptFilter_ColorTone;
  instance->FFilterName = CColorToneId;
  instance->FCaption    = tr("Color toning");
  return instance;
}

// -----------------------------------------------------------------------------

void ptFilter_ColorTone::doAfterInit() {
  this->internalInit();
}

//------------------------------------------------------------------------------

void ptFilter_ColorTone::doDefineControls() {
  ptCfgItem::TComboEntryList maskTypes = pt::ComboEntries::MaskTypes;
  maskTypes.append({QObject::tr("Midtones: screen"),       static_cast<int>(TMaskType::Screen),      "Screen"});
  maskTypes.append({QObject::tr("Midtones: multiply"),     static_cast<int>(TMaskType::Multiply),    "Multiply"});
  maskTypes.append({QObject::tr("Midtones: gamma bright"), static_cast<int>(TMaskType::GammaBright), "GammaBright"});
  maskTypes.append({QObject::tr("Midtones: gamma dark"),   static_cast<int>(TMaskType::GammaDark),   "GammaDark"});

  QColor defaultColor(0, 200, 255); // cyan
  if (this->uniqueName() == Fuid::ColorTone2_EyeCandy) {
    defaultColor.setRgb(255, 200, 0); // orange
  }

  FConfig.initStores(TCfgItemList()                                   //--- Combo: list of entries               ---//
    //            Id            Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaskType,    ptCfgItem::Combo,         static_cast<int>(TMaskType::Disabled), maskTypes,             true, true, tr("Mask type"), tr("Mask type")})
    << ptCfgItem(ptCfgItem::TColorSelectButton{CColor, ptCfgItem::ColorSelectButton, defaultColor,                      true, true, tr("toning color")})
    << ptCfgItem({CStrength,    ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CLowerLimit,  ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.1,        2,        true, true, tr("Lower limit"), tr("")})
    << ptCfgItem({CUpperLimit,  ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.1,        2,        true, true, tr("Upper limit"), tr("")})
    << ptCfgItem({CSoftness,    ptCfgItem::Slider,        0.0,       -2.0,          2.0,          0.1,        1,        true, true, tr("Softness"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_ColorTone::doCheckHasActiveCfg() {
  return pt::isActiveMaskType(FConfig.value(CMaskType));
}

//------------------------------------------------------------------------------

void ptFilter_ColorTone::doRunFilter(ptImage* AImage) {
  AImage->toRGB();
  const auto toneColor = FConfig.value(CColor).value<QColor>();
  AImage->Tone(
      toneColor.red(),
      toneColor.green(),
      toneColor.blue(),
      FConfig.value(CStrength).toDouble(),
      static_cast<TMaskType>(FConfig.value(CMaskType).toInt()),
      FConfig.value(CLowerLimit).toDouble(),
      FConfig.value(CUpperLimit).toDouble(),
      FConfig.value(CSoftness).toDouble(),
      Settings->GetDouble("InputPowerFactor") );
}

//------------------------------------------------------------------------------

QWidget* ptFilter_ColorTone::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_ColorToneForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper ColorToneRegister(&ptFilter_ColorTone::createColorTone, CColorToneId);

//------------------------------------------------------------------------------
