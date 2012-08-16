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

#include "ptFilter_Wiener.h"
#include "ui_ptFilter_Wiener.h"
#include "ptCfgItem.h"
#include "../ptWiener.h"
#include "../ptImage.h"

//==============================================================================

const QString CWienerId   = "Wiener";
const QString CEnabled    = "Enabled";
const QString COnlyEdges  = "OnlyEdges";
const QString CStrength   = "Strength";
const QString CGaussian   = "Gaussian";
const QString CBox        = "Box";
const QString CLensBlur   = "LensBlur";

//==============================================================================

ptFilter_Wiener::ptFilter_Wiener()
: ptFilterBase()
{
  FIsSlow = true;
  internalInit();
}

//==============================================================================

/* static */
ptFilterBase *ptFilter_Wiener::CreateWiener() {
  ptFilter_Wiener *hInstance = new ptFilter_Wiener;
  hInstance->FFilterName = CWienerId;
  hInstance->FCaption    = tr("Wiener sharpen");
  return hInstance;
}

//==============================================================================

void ptFilter_Wiener::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CEnabled,                ptCfgItem::Check,         0,                                                            true, true, tr("Enable"), tr("Switch filter on and off")})
    << ptCfgItem({COnlyEdges,              ptCfgItem::Check,         1,                                                            true, true, tr("Only edges"), tr("Sharpen only edges")})
    << ptCfgItem({CStrength,               ptCfgItem::Slider,        0.2,        0.0,          1.0,          0.05,       2,        true, true, tr("Strength") ,tr("")})
    << ptCfgItem({CGaussian,               ptCfgItem::Slider,        0.6,        0.0,          5.0,          0.05,       2,        true, true, tr("Gaussian") ,tr("")})
    << ptCfgItem({CBox,                    ptCfgItem::Slider,        0.0,        0.0,          5.0,          0.05,       2,        true, true, tr("Box"), tr("")})
    << ptCfgItem({CLensBlur,               ptCfgItem::Slider,        0.0,        0.0,          5.0,          0.05,       2,        true, true, tr("Lens blur") ,tr("")})
  ;
}

//==============================================================================

QWidget *ptFilter_Wiener::doCreateGui() {
  auto hGuiBody = new QWidget;
  Ui_WienerForm hForm;

  hForm.setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  return hGuiBody;
}

//==============================================================================

bool ptFilter_Wiener::doCheckHasActiveCfg() {
  return FConfig->getValue(CEnabled).toBool();
}

//==============================================================================

void ptFilter_Wiener::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  ptWienerFilterChannel(AImage,
                        FConfig->getValue(CGaussian).toDouble(),
                        FConfig->getValue(CBox).toDouble(),
                        FConfig->getValue(CLensBlur).toDouble(),
                        FConfig->getValue(CStrength).toDouble(),
                        FConfig->getValue(COnlyEdges).toBool());
}

//==============================================================================

RegisterHelper WienerRegister(&ptFilter_Wiener::CreateWiener, CWienerId);

//==============================================================================
