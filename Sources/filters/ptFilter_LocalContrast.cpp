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

#include "ptFilter_LocalContrast.h"
#include "ptFilterUids.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;
#include "../ptSettings.h"
#include <cmath>

//------------------------------------------------------------------------------

const QString CLocalContrastRgbId = "LocalContrastRgb";
const QString CLocalContrastLabId = "LocalContrastLab";

const QString CMaskType     = "MaskType";
const QString CRadius       = "Radius";
const QString CStrength     = "Strength";
const QString COpacity      = "Opacity";
const QString CHaloControl  = "HaloControl";
const QString CLowerLimit   = "LowerLimit";
const QString CUpperLimit   = "UpperLimit";
const QString CSoftness     = "Softness";

//------------------------------------------------------------------------------

ptFilter_LocalContrast::ptFilter_LocalContrast():
  ptFilterBase()
{
  // internalInit() cannot be done here because controls’ values depend on
  // the concrete filter instance. We need the unique name which is not set yet.
  // Init is performed by doAfterInit() instead.
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_LocalContrast::createLocalContrastRgb() {
  auto hInstance         = new ptFilter_LocalContrast;
  hInstance->FFilterName = CLocalContrastRgbId;
  hInstance->FCaption    = tr("Local contrast");
  return hInstance;
}

// -----------------------------------------------------------------------------

ptFilterBase *ptFilter_LocalContrast::createLocalContrastLab() {
  auto hInstance         = new ptFilter_LocalContrast;
  hInstance->FFilterName = CLocalContrastLabId;
  hInstance->FCaption    = tr("Local contrast");
  return hInstance;
}

// -----------------------------------------------------------------------------

void ptFilter_LocalContrast::doAfterInit() {
  this->internalInit();
}

//------------------------------------------------------------------------------

void ptFilter_LocalContrast::doDefineControls() {
  // Values for Fuid::LocalContrast1_RGB
  QVariant radDef      {100};
  QVariant radMax      {500};
  QVariant radStep     {25};
  QVariant strengthDef {8.0};

  if (this->uniqueName() == Fuid::LocalContrast2_RGB) {
    radDef      = 500;
    radMax      = 1000;
    radStep     = 50;
    strengthDef = 8.0;
  } else if (this->uniqueName() == Fuid::LocalContrast1_LabCC) {
    radDef      = 100;
    radMax      = 500;
    radStep     = 25;
    strengthDef = 4.0;
  } else if (this->uniqueName() == Fuid::LocalContrast2_LabCC) {
    radDef      = 500;
    radMax      = 1000;
    radStep     = 50;
    strengthDef = 4.0;
  } else if (this->uniqueName() != Fuid::LocalContrast1_RGB) {
    GInfo->Raise("Unexpected unique name: " + this->uniqueName(), AT);
  }

  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaskType,               ptCfgItem::Combo,   static_cast<int>(TMaskType::Disabled), pt::ComboEntries::MaskTypes, true, true, tr("Mask type"),    tr("")})
    << ptCfgItem({CRadius,                 ptCfgItem::Slider,        radDef,     0,            radMax,       radStep,    0,        true, true, tr("Radius"),       tr("")})
    << ptCfgItem({CStrength,               ptCfgItem::Slider,   strengthDef,   -10.0,         20.0,          1.0,        1,        true, true, tr("Strength"),     tr("")})
    << ptCfgItem({COpacity,                ptCfgItem::Slider,        0.2,        0.0,          1.0,          0.1,        1,        true, true, tr("Opacity"),      tr("")})
    << ptCfgItem({CHaloControl,            ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        1,        true, true, tr("Halo control"), tr("")})
    << ptCfgItem({CLowerLimit,             ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.05,       3,        true, true, tr("Lower limit"),  tr("")})
    << ptCfgItem({CUpperLimit,             ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.05,       3,        true, true, tr("Upper limit"),  tr("")})
    << ptCfgItem({CSoftness,               ptCfgItem::Slider,        0.0,       -2.0,          2.0,          0.1,        1,        true, true, tr("Softness"),     tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_LocalContrast::doCheckHasActiveCfg() {
  return static_cast<TMaskType>(FConfig.value(CMaskType).toInt()) != TMaskType::Disabled;
}

//------------------------------------------------------------------------------

void ptFilter_LocalContrast::doRunFilter(ptImage *AImage) {
  double lowerLimit {};
  double upperLimit {};

  if (FFilterName == CLocalContrastRgbId) {
    AImage->toRGB();
    lowerLimit = std::pow(
        FConfig.value(CLowerLimit).toDouble(),
        Settings->GetDouble("InputPowerFactor"));
    upperLimit = std::pow(
        FConfig.value(CUpperLimit).toDouble(),
        Settings->GetDouble("InputPowerFactor"));
  } else {
    AImage->toLab();
    lowerLimit = FConfig.value(CLowerLimit).toDouble();
    upperLimit = FConfig.value(CUpperLimit).toDouble();
  }

  AImage->Microcontrast(
      FConfig.value(CRadius).toInt() * TheProcessor->m_ScaleFactor,
      FConfig.value(CStrength).toDouble(),
      FConfig.value(COpacity).toDouble(),
      FConfig.value(CHaloControl).toDouble(),
      static_cast<TMaskType>(FConfig.value(CMaskType).toInt()),
      lowerLimit,
      upperLimit,
      FConfig.value(CSoftness).toDouble());
}

//------------------------------------------------------------------------------

RegisterHelper LocalContrastRgbRegister(&ptFilter_LocalContrast::createLocalContrastRgb, CLocalContrastRgbId);
RegisterHelper LocalContrastLabRegister(&ptFilter_LocalContrast::createLocalContrastLab, CLocalContrastLabId);

//------------------------------------------------------------------------------
