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

#include "ptFilter_CrossProcessing.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CCrossProcessingId = "CrossProcessing";

const QString CMode        = "Mode";
const QString CMainColor   = "MainColor";
const QString CSecondColor = "SecondColor";

//------------------------------------------------------------------------------

ptFilter_CrossProcessing::ptFilter_CrossProcessing():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_CrossProcessing::createCrossProcessing() {
  auto instance         = new ptFilter_CrossProcessing;
  instance->FFilterName = CCrossProcessingId;
  instance->FCaption    = tr("Cross processing");
  return instance;
}

//------------------------------------------------------------------------------

void ptFilter_CrossProcessing::doDefineControls() {
  const ptCfgItem::TComboEntryList modes({
    {tr("Disabled"),       static_cast<int>(TCrossProcessMode::Disabled),    "Disabled"},
    {tr("Green - yellow"), static_cast<int>(TCrossProcessMode::GreenYellow), "GreenYellow"},
    {tr("Green - cyan"),   static_cast<int>(TCrossProcessMode::GreenCyan),   "GreenCyan"},
    {tr("Red - yellow"),   static_cast<int>(TCrossProcessMode::RedYellow),   "RedYellow"},
    {tr("Red - magenta"),  static_cast<int>(TCrossProcessMode::RedMagenta),  "RedMagenta"},
    {tr("Blue - cyan"),    static_cast<int>(TCrossProcessMode::BlueCyan),    "BlueCyan"},
    {tr("Blue - magenta"), static_cast<int>(TCrossProcessMode::BlueMagenta), "BlueMagenta"},
  });

  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id             Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,         ptCfgItem::Combo,         static_cast<int>(TCrossProcessMode::Disabled), modes,         true, true, tr("Colors"), tr("")})
    << ptCfgItem({CMainColor,    ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        2,        true, true, tr("Main color"), tr("intensity of the main color")})
    << ptCfgItem({CSecondColor,  ptCfgItem::Slider,        0.4,        0.0,          1.0,          0.1,        2,        true, true, tr("Secondary color"), tr("intensity of the secondary color")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_CrossProcessing::doCheckHasActiveCfg() {
  return static_cast<TCrossProcessMode>(FConfig.value(CMode).toInt()) != TCrossProcessMode::Disabled;
}

//------------------------------------------------------------------------------

void ptFilter_CrossProcessing::doRunFilter(ptImage* AImage) {
  AImage->toRGB();
  AImage->Crossprocess(
      static_cast<TCrossProcessMode>(FConfig.value(CMode).toInt()),
      FConfig.value(CMainColor).toDouble(),
      FConfig.value(CSecondColor).toDouble() );
}

//------------------------------------------------------------------------------

RegisterHelper CrossProcessingRegister(&ptFilter_CrossProcessing::createCrossProcessing, CCrossProcessingId);

//------------------------------------------------------------------------------
