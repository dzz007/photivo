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

#include "ptFilter_FilmGrain.h"
#include "ui_ptFilter_FilmGrain.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CFilmGrainId = "FilmGrain";

const QString CMaskType1   = "MaskType1";
const QString CGrainType1  = "GrainType1";
const QString CStrength1   = "Strength1";
const QString CRadius1     = "Radius1";
const QString COpacity1    = "Opacity1";
const QString CLowerLimit1 = "LowerLimit1";
const QString CUpperLimit1 = "UpperLimit1";

const QString CMaskType2   = "MaskType2";
const QString CGrainType2  = "GrainType2";
const QString CStrength2   = "Strength2";
const QString CRadius2     = "Radius2";
const QString COpacity2    = "Opacity2";
const QString CLowerLimit2 = "LowerLimit2";
const QString CUpperLimit2 = "UpperLimit2";

//------------------------------------------------------------------------------

ptFilter_FilmGrain::ptFilter_FilmGrain():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_FilmGrain::createFilmGrain() {
  auto hInstance         = new ptFilter_FilmGrain;
  hInstance->FFilterName = CFilmGrainId;
  hInstance->FCaption    = tr("Film grain simulation");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_FilmGrain::doDefineControls() {
  const ptCfgItem::TComboEntryList grainTypes({
    {tr("Soft gaussian"),       static_cast<int>(TGrainType::SoftGaussian),   "SoftGaussian"},
    {tr("Soft uniform"),        static_cast<int>(TGrainType::SoftUniform),    "SoftUniform"},
    {tr("Soft salt & pepper"),  static_cast<int>(TGrainType::SoftSaltPepper), "SoftSaltPepper"},
    {tr("Hard gaussian"),       static_cast<int>(TGrainType::HardGaussian),   "HardGaussian"},
    {tr("Hard uniform"),        static_cast<int>(TGrainType::HardUniform),    "HardUniform"},
    {tr("Hard salt & pepper"),  static_cast<int>(TGrainType::HardSaltPepper), "HardSaltPepper"},
  });

  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id             Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaskType1,    ptCfgItem::Combo,   static_cast<int>(TMaskType::Disabled), pt::ComboEntries::MaskTypes, true, true, tr("Mask type"),   tr("Mask type")})
    << ptCfgItem({CGrainType1,   ptCfgItem::Combo,   static_cast<int>(TGrainType::SoftGaussian), grainTypes,             true, true, tr("Noise type"),  tr("Noise type")})
    << ptCfgItem({CStrength1,    ptCfgItem::Slider,        0.3,        0.0,          1.0,          0.1,        2,        true, true, tr("Strength"),    tr("")})
    << ptCfgItem({CRadius1,      ptCfgItem::Slider,        1.0,        0.0,         10.0,          1.0,        1,        true, true, tr("Radius"),      tr("")})
    << ptCfgItem({COpacity1,     ptCfgItem::Slider,        0.3,        0.0,          1.0,          0.1,        2,        true, true, tr("Opacity"),     tr("")})
    << ptCfgItem({CLowerLimit1,  ptCfgItem::Slider,        0.1,        0.0,          1.0,          0.002,      3,        true, true, tr("Lower limit"), tr("")})
    << ptCfgItem({CUpperLimit1,  ptCfgItem::Slider,        0.4,        0.0,          1.0,          0.002,      3,        true, true, tr("Upper limit"), tr("")})

    << ptCfgItem({CMaskType2,    ptCfgItem::Combo,   static_cast<int>(TMaskType::Disabled), pt::ComboEntries::MaskTypes, true, true, tr("Mask type"),   tr("Mask type")})
    << ptCfgItem({CGrainType2,   ptCfgItem::Combo,   static_cast<int>(TGrainType::SoftGaussian), grainTypes,             true, true, tr("Noise type"),  tr("Noise type")})
    << ptCfgItem({CStrength2,    ptCfgItem::Slider,        0.3,        0.0,          1.0,          0.1,        2,        true, true, tr("Strength"),    tr("")})
    << ptCfgItem({CRadius2,      ptCfgItem::Slider,        2.0,        0.0,         10.0,          1.0,        1,        true, true, tr("Radius"),      tr("")})
    << ptCfgItem({COpacity2,     ptCfgItem::Slider,        0.2,        0.0,          1.0,          0.1,        2,        true, true, tr("Opacity"),     tr("")})
    << ptCfgItem({CLowerLimit2,  ptCfgItem::Slider,        0.1,        0.0,          1.0,          0.002,      3,        true, true, tr("Lower limit"), tr("")})
    << ptCfgItem({CUpperLimit2,  ptCfgItem::Slider,        0.4,        0.0,          1.0,          0.002,      3,        true, true, tr("Upper limit"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_FilmGrain::doCheckHasActiveCfg() {
  return
      pt::isActiveMaskType(FConfig.value(CMaskType1)) ||
      pt::isActiveMaskType(FConfig.value(CMaskType2));
}

//------------------------------------------------------------------------------

void ptFilter_FilmGrain::doRunFilter(ptImage *AImage) {
  AImage->toLab();
  int scaleFactor = static_cast<int>(logf(TheProcessor->m_ScaleFactor)/logf(0.5f));

  if (pt::isActiveMaskType(FConfig.value(CMaskType1))) {
    AImage->Grain(
          FConfig.value(CStrength1).toDouble(),
          static_cast<TGrainType>(FConfig.value(CGrainType1).toInt()),
          FConfig.value(CRadius1).toDouble(),
          FConfig.value(COpacity1).toDouble(),
          static_cast<TMaskType>(FConfig.value(CMaskType1).toInt()),
          FConfig.value(CLowerLimit1).toDouble(),
          FConfig.value(CUpperLimit1).toDouble(),
          scaleFactor);
  }

  if (pt::isActiveMaskType(FConfig.value(CMaskType2))) {
    AImage->Grain(
          FConfig.value(CStrength2).toDouble(),
          static_cast<TGrainType>(FConfig.value(CGrainType2).toInt()),
          FConfig.value(CRadius2).toDouble(),
          FConfig.value(COpacity2).toDouble(),
          static_cast<TMaskType>(FConfig.value(CMaskType2).toInt()),
          FConfig.value(CLowerLimit2).toDouble(),
          FConfig.value(CUpperLimit2).toDouble(),
          scaleFactor);
  }
}

//------------------------------------------------------------------------------

QWidget *ptFilter_FilmGrain::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_FilmGrainForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper FilmGrainRegister(&ptFilter_FilmGrain::createFilmGrain, CFilmGrainId);

//------------------------------------------------------------------------------
