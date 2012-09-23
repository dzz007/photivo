/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptFilter_Exposure.h"
#include "ui_ptFilter_Exposure.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CExposureId = "Exposure";

const QString CExposure      = "Exposure";
const QString CClipMode      = "ClipMode";
const QString CUfrawEV       = "UfrawEV";
const QString CAutoExpose    = "AutoExpose";
const QString CWhiteFraction = "WhiteFraction";
const QString CWhiteLevel    = "WhiteLevel";

//==============================================================================

ptFilter_Exposure::ptFilter_Exposure()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Exposure::createExposure() {
  auto hInstance         = new ptFilter_Exposure;
  hInstance->FFilterName = CExposureId;
  hInstance->FCaption    = tr("Exposure");
  return hInstance;
}

//==============================================================================

void ptFilter_Exposure::doDefineControls() {
  QList<ptCfgItem::TComboEntry> hClipModes;
  hClipModes.append({tr("No clip"),    NoExposureClip,    "none"});
  hClipModes.append({tr("Ratio"),      RatioExposureClip, "ratio"});
  hClipModes.append({tr("Film curve"), CurveExposureClip, "filmcurve"});

  FCfgItems = QList<ptCfgItem>()                                            //--- Combo: list of entries               ---//
                                                                            //--- Check: not available                 ---//
    //            Id                  Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CExposure,          ptCfgItem::Slider,        0.0,       -5.0,          5.0,          0.1,        2,        true, true, tr("EV"), tr("Exposure correction in f-stops")})
    << ptCfgItem({CClipMode,          ptCfgItem::Combo,         CurveExposureClip, hClipModes,                                true, true, tr("Name in GUI"),   tr("Tooltip")})
    << ptCfgItem({CUfrawEV,           ptCfgItem::Check,         0,                                                            true, true, tr("Exposure like UFRaw"), tr("Adds 0.67 to the EV value")})
    << ptCfgItem({CAutoExpose,        ptCfgItem::Check,         0,                                                            true, true, tr("Automatic exposure"), tr("")})
    << ptCfgItem({CWhiteFraction,     ptCfgItem::Slider,        10,         1,            50,           1,          0,        true, true, tr("% white"), tr("Percentage of white aimed at")})
    << ptCfgItem({CWhiteLevel,        ptCfgItem::Slider,        90,         50,           99,           1,          0,        true, true, tr("White level"), tr("Brightness of the “white”")})
  ;
}

//==============================================================================

bool ptFilter_Exposure::doCheckHasActiveCfg() {
  return FConfig->getValue(CClipMode).toFloat() != 0.0f;
}

//==============================================================================

void ptFilter_Exposure::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
//  AImage->Exposure(FConfig->getValue(CExposure).toBool(),
//                     FConfig->getValue(CClipMode).toFloat(),
//                     FConfig->getValue(CUfrawEV).toInt() );
}

//==============================================================================

QWidget *ptFilter_Exposure::doCreateGui() {
  auto hGuiBody = new QWidget;
  Ui_ExposureForm hForm;

  hForm.setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  return hGuiBody;
}

//==============================================================================

RegisterHelper ExposureRegister(&ptFilter_Exposure::createExposure, CExposureId);

//==============================================================================
