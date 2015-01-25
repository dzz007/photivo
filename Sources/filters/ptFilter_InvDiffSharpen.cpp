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

#include "ptFilter_InvDiffSharpen.h"
#include "ui_ptFilter_InvDiffSharpen.h"
#include "ptCfgItem.h"
#include "../ptCimg.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CInvDiffSharpenId = "InvDiffSharpen";

const QString CIterations = "Iterations";
const QString COnlyEdges  = "OnlyEdges";
const QString CAmplitude  = "Amplitude";

//------------------------------------------------------------------------------

ptFilter_InvDiffSharpen::ptFilter_InvDiffSharpen():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_InvDiffSharpen::createInvDiffSharpen() {
  auto hInstance         = new ptFilter_InvDiffSharpen;
  hInstance->FFilterName = CInvDiffSharpenId;
  hInstance->FCaption    = tr("Inverse diffusion sharpen");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_InvDiffSharpen::doDefineControls() {
  FConfig.initStores(TCfgItemList()
                                                                       //--- Check: not available                 ---//
    //            Id             Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CIterations,   ptCfgItem::SpinEdit,      0,          0,            5,            1,          0,        true, true, tr("Iterations"), tr("")})
    << ptCfgItem({COnlyEdges,    ptCfgItem::Check,         true,                                                         true, true, tr("Only edges"), tr("")})
    << ptCfgItem({CAmplitude,    ptCfgItem::Slider,        0.2,        0.0,          2.0,          0.05,       2,        true, true, tr("Amplitude"),  tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_InvDiffSharpen::doCheckHasActiveCfg() {
  return FConfig.value(CIterations).toInt() != 0;
}

//------------------------------------------------------------------------------

void ptFilter_InvDiffSharpen::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  ptCimgSharpen(
      AImage,
      ChMask_L,
      FConfig.value(CAmplitude).toDouble(),
      FConfig.value(CIterations).toInt(),
      FConfig.value(COnlyEdges).toBool());
}

//------------------------------------------------------------------------------

QWidget *ptFilter_InvDiffSharpen::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_InvDiffSharpenForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  return guiBody;
}

//------------------------------------------------------------------------------

RegisterHelper InvDiffSharpenRegister(&ptFilter_InvDiffSharpen::createInvDiffSharpen, CInvDiffSharpenId);

//------------------------------------------------------------------------------
