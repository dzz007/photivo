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

#include "ptFilter_SoftglowOrton.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CSoftglowOrtonId = "SoftglowOrton";

const QString CMode       = "Mode";
const QString CStrength   = "Strength";
const QString CRadius     = "Radius";
const QString CContrast   = "Contrast";
const QString CSaturation = "Saturation";

//------------------------------------------------------------------------------

ptFilter_SoftglowOrton::ptFilter_SoftglowOrton():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_SoftglowOrton::createSoftglowOrton() {
  auto instance         = new ptFilter_SoftglowOrton;
  instance->FFilterName = CSoftglowOrtonId;
  instance->FCaption    = tr("Softglow/Orton");
  return instance;
}

//------------------------------------------------------------------------------

void ptFilter_SoftglowOrton::doDefineControls() {
  const ptCfgItem::TComboEntryList sgModes({
    {tr("Disabled"),        static_cast<int>(TSoftglowMode::Disabled),       "Disabled"},
    {tr("Lighten"),         static_cast<int>(TSoftglowMode::Lighten),        "Lighten"},
    {tr("Screen"),          static_cast<int>(TSoftglowMode::Screen),         "Screen"},
    {tr("Softlight"),       static_cast<int>(TSoftglowMode::Softlight),      "Softlight"},
    {tr("Normal"),          static_cast<int>(TSoftglowMode::Normal),         "Normal"},
    {tr("Orton screen"),    static_cast<int>(TSoftglowMode::OrtonScreen),    "OrtonScreen"},
    {tr("Orton softlight"), static_cast<int>(TSoftglowMode::OrtonSoftlight), "OrtonSoftlight"},
  });

  FConfig.initStores(TCfgItemList()                                   //--- Combo: list of entries               ---//
    //            Id            Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,        ptCfgItem::Combo,         static_cast<int>(TSoftglowMode::Disabled), sgModes,           true, true, tr("Mode"), tr("")})
    << ptCfgItem({CStrength,    ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        1,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CRadius,      ptCfgItem::Slider,       10.0,        0.0,         30.0,          1.0,        1,        true, true, tr("Radius"), tr("")})
    << ptCfgItem({CContrast,    ptCfgItem::Slider,        5.0,        0.0,         20.0,          0.5,        1,        true, true, tr("Contrast"), tr("")})
    << ptCfgItem({CSaturation,  ptCfgItem::Slider,        -50,       -100,         100,           5,          0,        true, true, tr("Saturation"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_SoftglowOrton::doCheckHasActiveCfg() {
  return
      (static_cast<TSoftglowMode>(FConfig.value(CMode).toInt()) != TSoftglowMode::Disabled) &&
      !qFuzzyIsNull(FConfig.value(CStrength).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_SoftglowOrton::doRunFilter(ptImage *AImage) {
  AImage->toRGB();
  AImage->Softglow(
      static_cast<TSoftglowMode>(FConfig.value(CMode).toInt()),
      FConfig.value(CRadius).toDouble() * TheProcessor->m_ScaleFactor,
      FConfig.value(CStrength).toDouble(),
      ChMask_RGB,
      FConfig.value(CContrast).toDouble(),
      FConfig.value(CSaturation).toDouble() );
}

//------------------------------------------------------------------------------

RegisterHelper SoftglowOrtonRegister(&ptFilter_SoftglowOrton::createSoftglowOrton, CSoftglowOrtonId);

//------------------------------------------------------------------------------
