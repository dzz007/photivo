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

#include "ptFilter_LocalContrastStretch.h"
#include "ptFilterUids.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptInfo.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CLocalContrastStretchId = "LocalContrastStretch";

const QString CRadius  = "Radius";
const QString CFeather = "Feather";
const QString COpacity = "Opacity";
const QString CMasking = "Masking";

//------------------------------------------------------------------------------

ptFilter_LocalContrastStretch::ptFilter_LocalContrastStretch():
  ptFilterBase()
{
  FIsSlow = true;
  // internalInit() cannot be done here because controls’ values depend on
  // the concrete filter instance. We need the unique name which is not set yet.
  // Init is performed by doAfterInit() instead.
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_LocalContrastStretch::createLocalContrastStretch() {
  auto hInstance         = new ptFilter_LocalContrastStretch;
  hInstance->FFilterName = CLocalContrastStretchId;
  hInstance->FCaption    = tr("Local contrast stretch");
  return hInstance;
}

// -----------------------------------------------------------------------------

void ptFilter_LocalContrastStretch::doAfterInit() {
  this->internalInit();
}

//------------------------------------------------------------------------------

void ptFilter_LocalContrastStretch::doDefineControls() {
  // Values for Fuid::LocalContrastStretch1_LabCC
  QVariant radDefault {400};
  QVariant radStep    {20};

  if (this->uniqueName() == Fuid::LocalContrastStretch2_LabCC) {
    radDefault = 800;
    radStep    = 40;
  } else if (this->uniqueName() != Fuid::LocalContrastStretch1_LabCC) {
    GInfo->Raise("Unexpected unique name: " + this->uniqueName(), AT);
  }

  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CRadius,                 ptCfgItem::Slider,        radDefault, 0,         1000,            radStep,    0,        true, true, tr("Radius"),  tr("")})
    << ptCfgItem({CFeather,                ptCfgItem::Slider,       -0.3,       -1.0,          1.0,          0.1,        2,        true, true, tr("Feather"), tr("")})
    << ptCfgItem({COpacity,                ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.1,        2,        true, true, tr("Opacity"), tr("")})
    << ptCfgItem({CMasking,                ptCfgItem::Slider,        1.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Masking"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_LocalContrastStretch::doCheckHasActiveCfg() {
  return FConfig.value(COpacity).toDouble() != 0.0f;
}

//------------------------------------------------------------------------------

void ptFilter_LocalContrastStretch::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->Localcontrast(
      FConfig.value(CRadius).toDouble() * TheProcessor->m_ScaleFactor,
      FConfig.value(COpacity).toDouble(),
      FConfig.value(CMasking).toDouble(),
      FConfig.value(CFeather).toDouble(),
      1);
  // "1" is the "method". There are "1" and "2". No idea what that means. Why is this
  // argument an integer in the first place? Should have been an enum or enum class.
}

//------------------------------------------------------------------------------

RegisterHelper LocalContrastStretchRegister(&ptFilter_LocalContrastStretch::createLocalContrastStretch, CLocalContrastStretchId);

//------------------------------------------------------------------------------
