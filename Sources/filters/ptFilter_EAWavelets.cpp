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

#include "ptFilter_EAWavelets.h"
#include "ui_ptFilter_EAWavelets.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

//------------------------------------------------------------------------------

const QString CEAWaveletsId = "EAWavelets";

const QString CMaster = "Master";
const QString CLevel1 = "Level1";
const QString CLevel2 = "Level2";
const QString CLevel3 = "Level3";
const QString CLevel4 = "Level4";
const QString CLevel5 = "Level5";
const QString CLevel6 = "Level6";

//------------------------------------------------------------------------------

ptFilter_EAWavelets::ptFilter_EAWavelets():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_EAWavelets::createEAWavelets() {
  auto hInstance         = new ptFilter_EAWavelets;
  hInstance->FFilterName = CEAWaveletsId;
  hInstance->FCaption    = tr("Edge avoiding wavelets");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_EAWavelets::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaster,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Master"),  tr("Quick setup of all levels")})
    << ptCfgItem({CLevel1,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Level 1"), tr("")})
    << ptCfgItem({CLevel2,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Level 2"), tr("")})
    << ptCfgItem({CLevel3,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Level 3"), tr("")})
    << ptCfgItem({CLevel4,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Level 4"), tr("")})
    << ptCfgItem({CLevel5,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Level 5"), tr("")})
    << ptCfgItem({CLevel6,                 ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, true, tr("Level 6"), tr("")})
  );
}

// -----------------------------------------------------------------------------

void ptFilter_EAWavelets::onMasterValueChanged(QString, QVariant ANewValue) {
  QVariant zero         = 0.0;
  double   masterVal    = ANewValue.toDouble();
  double   absMasterVal = fabs(masterVal);

  FConfig.setValue(CMaster, ANewValue);

  if (qFuzzyIsNull(masterVal)) {
    FConfig.setValue(CLevel1, zero);
    FConfig.setValue(CLevel2, zero);
    FConfig.setValue(CLevel3, zero);
    FConfig.setValue(CLevel4, zero);
    FConfig.setValue(CLevel5, zero);
    FConfig.setValue(CLevel6, zero);
  } else if (absMasterVal <= 0.1) {
    FConfig.setValue(CLevel1, masterVal);
    FConfig.setValue(CLevel2, zero);
    FConfig.setValue(CLevel3, zero);
    FConfig.setValue(CLevel4, zero);
    FConfig.setValue(CLevel5, zero);
    FConfig.setValue(CLevel6, zero);
  } else if (absMasterVal <= 0.2) {
    FConfig.setValue(CLevel1, masterVal);
    FConfig.setValue(CLevel2, masterVal / 2.0);
    FConfig.setValue(CLevel3, zero);
    FConfig.setValue(CLevel4, zero);
    FConfig.setValue(CLevel5, zero);
    FConfig.setValue(CLevel6, zero);
  } else if (absMasterVal <= 0.3) {
    FConfig.setValue(CLevel1, masterVal);
    FConfig.setValue(CLevel2, masterVal / 3.0 * 2.0);
    FConfig.setValue(CLevel3, masterVal / 3.0);
    FConfig.setValue(CLevel4, zero);
    FConfig.setValue(CLevel5, zero);
    FConfig.setValue(CLevel6, zero);
  } else if (absMasterVal <= 0.4) {
    FConfig.setValue(CLevel1, masterVal);
    FConfig.setValue(CLevel2, masterVal / 4.0 * 3.0);
    FConfig.setValue(CLevel3, masterVal / 4.0 * 2.0);
    FConfig.setValue(CLevel4, masterVal / 4.0);
    FConfig.setValue(CLevel5, zero);
    FConfig.setValue(CLevel6, zero);
  } else if (absMasterVal <= 0.5) {
    FConfig.setValue(CLevel1, masterVal);
    FConfig.setValue(CLevel2, masterVal / 5.0 * 4.0);
    FConfig.setValue(CLevel3, masterVal / 5.0 * 3.0);
    FConfig.setValue(CLevel4, masterVal / 5.0 * 2.0);
    FConfig.setValue(CLevel5, masterVal / 5.0);
    FConfig.setValue(CLevel6, zero);
  } else {
    FConfig.setValue(CLevel1, masterVal);
    FConfig.setValue(CLevel2, masterVal / 6.0 * 5.0);
    FConfig.setValue(CLevel3, masterVal / 6.0 * 4.0);
    FConfig.setValue(CLevel4, masterVal / 6.0 * 3.0);
    FConfig.setValue(CLevel5, masterVal / 6.0 * 2.0);
    FConfig.setValue(CLevel6, masterVal / 6.0);
  }

  this->updateGui();
}

// -----------------------------------------------------------------------------

void ptFilter_EAWavelets::onLevelValueChanged(QString AId, QVariant ANewValue) {
  FConfig.setValue(AId, ANewValue);
//  FConfig.setValue(CMaster, 0.0);
  this->updateGui();
}

//------------------------------------------------------------------------------

bool ptFilter_EAWavelets::doCheckHasActiveCfg() {
  return
      !qFuzzyIsNull(FConfig.value(CLevel1).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CLevel2).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CLevel3).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CLevel4).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CLevel5).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CLevel6).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_EAWavelets::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->EAWChannel(
      static_cast<int>(logf(TheProcessor->m_ScaleFactor)/logf(0.5f)),
      FConfig.value(CLevel1).toDouble(),
      FConfig.value(CLevel2).toDouble(),
      FConfig.value(CLevel3).toDouble(),
      FConfig.value(CLevel4).toDouble(),
      FConfig.value(CLevel5).toDouble(),
      FConfig.value(CLevel6).toDouble());
}

//------------------------------------------------------------------------------

QWidget *ptFilter_EAWavelets::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_EAWaveletsForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);
  connect(form.Master, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onMasterValueChanged(QString,QVariant)));
  connect(form.Level1, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onLevelValueChanged(QString,QVariant)));
  connect(form.Level2, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onLevelValueChanged(QString,QVariant)));
  connect(form.Level3, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onLevelValueChanged(QString,QVariant)));
  connect(form.Level4, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onLevelValueChanged(QString,QVariant)));
  connect(form.Level5, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onLevelValueChanged(QString,QVariant)));
  connect(form.Level6, SIGNAL(valueChanged(QString,QVariant)),
                       SLOT(onLevelValueChanged(QString,QVariant)));

  return guiBody;
}

// -----------------------------------------------------------------------------

RegisterHelper EAWaveletsRegister(&ptFilter_EAWavelets::createEAWavelets, CEAWaveletsId);

//------------------------------------------------------------------------------
