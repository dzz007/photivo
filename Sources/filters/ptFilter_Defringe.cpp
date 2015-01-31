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

#include "ptFilter_Defringe.h"
#include "ui_ptFilter_Defringe.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CDefringeId = "Defringe";

const QString CRadius      = "Radius";
const QString CThreshold   = "Threshold";
const QString CMaskTuning  = "MaskTuning";
const QString CColorRed    = "ColorRed";
const QString CColorYellow = "ColorYellow";
const QString CColorGreen  = "ColorGreen";
const QString CColorCyan   = "ColorCyan";
const QString CColorBlue   = "ColorBlue";
const QString CColorPurple = "ColorPurple";

//------------------------------------------------------------------------------

ptFilter_Defringe::ptFilter_Defringe():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_Defringe::createDefringe() {
  auto hInstance         = new ptFilter_Defringe;
  hInstance->FFilterName = CDefringeId;
  hInstance->FCaption    = tr("Defringe");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_Defringe::doDefineControls() {
  FConfig.initStores(TCfgItemList()
                                                                       //--- Check: not available                 ---//
    //            Id             Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CRadius,         ptCfgItem::Slider,        0.0,        0.0,         10.0,          0.5,        1,        true, true, tr("Radius"),       tr("")})
    << ptCfgItem({CThreshold,      ptCfgItem::Slider,        25,         0,          100,            5,          0,        true, true, tr("Threshold"),    tr("")})
    << ptCfgItem({CMaskTuning,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Masks tuning"), tr("fine tunes the color masks")})
    << ptCfgItem({CColorRed,       ptCfgItem::Check,         false,                                                        true, true, tr("Red"), tr("")})
    << ptCfgItem({CColorYellow,    ptCfgItem::Check,         false,                                                        true, true, tr("Yellow"), tr("")})
    << ptCfgItem({CColorGreen,     ptCfgItem::Check,         true,                                                         true, true, tr("Green"), tr("")})
    << ptCfgItem({CColorCyan,      ptCfgItem::Check,         false,                                                        true, true, tr("Cyan"), tr("")})
    << ptCfgItem({CColorBlue,      ptCfgItem::Check,         true,                                                         true, true, tr("Blue"), tr("")})
    << ptCfgItem({CColorPurple,    ptCfgItem::Check,         true,                                                         true, true, tr("Purple"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_Defringe::doCheckHasActiveCfg() {
  return !qFuzzyIsNull(FConfig.value(CRadius).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_Defringe::doRunFilter(ptImage *AImage) {
  AImage->toLab();
  AImage->DeFringe(
      FConfig.value(CRadius).toDouble(),
      FConfig.value(CThreshold).toInt(),
      FConfig.value(CColorRed).toInt() +
          (FConfig.value(CColorYellow).toInt() << 1) +
          (FConfig.value(CColorGreen).toInt() << 2) +
          (FConfig.value(CColorCyan).toInt() << 3) +
          (FConfig.value(CColorBlue).toInt() << 4) +
          (FConfig.value(CColorPurple).toInt() << 5),
      FConfig.value(CMaskTuning).toDouble());
}

//------------------------------------------------------------------------------

QWidget *ptFilter_Defringe::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_DefringeForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper DefringeRegister(&ptFilter_Defringe::createDefringe, CDefringeId);

//------------------------------------------------------------------------------
