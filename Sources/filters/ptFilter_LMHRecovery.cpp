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

#include "ptFilter_LMHRecovery.h"
#include "ui_ptFilter_LMHRecovery.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptSettings.h"

//==============================================================================

const QString CLMHRecoveryRgbId = "LMHRecoveryRgb";
const QString CLMHRecoveryLabId = "LMHRecoveryLab";

const QString CMaskType1      = "MaskType1";
const QString CStrength1      = "Strength1";
const QString CLowerLimit1    = "LowerLimit1";
const QString CUpperLimit1    = "UpperLimit1";
const QString CSoftness1      = "Softness1";

const QString CMaskType2      = "MaskType2";
const QString CStrength2      = "Strength2";
const QString CLowerLimit2    = "LowerLimit2";
const QString CUpperLimit2    = "UpperLimit2";
const QString CSoftness2      = "Softness2";

//==============================================================================

ptFilter_LMHRecovery::ptFilter_LMHRecovery(const QString &AFilterName, TColorSpace AColorSpace)
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  FFilterName = AFilterName;
  FColorSpace = AColorSpace;
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_LMHRecovery::createLMHRecoveryRgb() {
  auto hInstance         = new ptFilter_LMHRecovery(CLMHRecoveryRgbId, TColorSpace::Rgb);
  hInstance->FCaption    = tr("Low/mid/highlight recovery");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_LMHRecovery::createLMHRecoveryLab() {
  auto hInstance         = new ptFilter_LMHRecovery(CLMHRecoveryLabId, TColorSpace::Lab);
  hInstance->FCaption    = tr("Low/mid/highlight recovery");
  return hInstance;
}
//==============================================================================

void ptFilter_LMHRecovery::doDefineControls() {
  QList<ptCfgItem::TComboEntry> hMaskEntries;
  hMaskEntries.append({tr("Disabled"),    ptMaskType_None,       "disabled"});
  hMaskEntries.append({tr("Shadows"),     ptMaskType_Shadows,    "shadows"});
  hMaskEntries.append({tr("Midtones"),    ptMaskType_Midtones,   "midtones"});
  hMaskEntries.append({tr("Highlights"),  ptMaskType_Highlights, "highlights"});
  hMaskEntries.append({tr("All values"),  ptMaskType_All,        "allvalues"});

  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaskType1,               ptCfgItem::Combo,         ptMaskType_None, hMaskEntries,                                true, true, tr("Mask type"), tr("")})
    << ptCfgItem({CStrength1,               ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.1,        2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CLowerLimit1,             ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.002,      3,        true, true, tr("Lower limit"), tr("")})
    << ptCfgItem({CUpperLimit1,             ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.002,      3,        true, true, tr("Upper limit"), tr("")})
    << ptCfgItem({CSoftness1,               ptCfgItem::Slider,        1.0,       -2.0,          2.0,          0.1,        1,        true, true, tr("Softness"), tr("")})
    << ptCfgItem({CMaskType2,               ptCfgItem::Combo,         ptMaskType_None, hMaskEntries,                                true, true, tr("Mask type"), tr("")})
    << ptCfgItem({CStrength2,               ptCfgItem::Slider,        0.0,       -3.0,          3.0,          0.1,        2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CLowerLimit2,             ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.002,      3,        true, true, tr("Lower limit"), tr("")})
    << ptCfgItem({CUpperLimit2,             ptCfgItem::Slider,        1.0,        0.0,          1.0,          0.002,      3,        true, true, tr("Upper limit"), tr("")})
    << ptCfgItem({CSoftness2,               ptCfgItem::Slider,        1.0,       -2.0,          2.0,          0.1,        1,        true, true, tr("Softness"), tr("")})
  );
}

//==============================================================================

bool ptFilter_LMHRecovery::doCheckHasActiveCfg() {
  return (FConfig.value(CMaskType1).toInt() != ptMaskType_None) ||
         (FConfig.value(CMaskType2).toInt() != ptMaskType_None);
}

//==============================================================================

void ptFilter_LMHRecovery::doRunFilter(ptImage *AImage) const {
  auto hInputFactor = Settings->GetDouble("InputPowerFactor");

  auto hMaskType1   = FConfig.value(CMaskType1).toInt();
  auto hStrength1   = FConfig.value(CStrength1).toDouble();
  auto hLowerLimit1 = FConfig.value(CLowerLimit1).toDouble();
  auto hUpperLimit1 = FConfig.value(CUpperLimit1).toDouble();
  auto hSoftness1   = FConfig.value(CSoftness1).toDouble();
  hLowerLimit1 = qMin(hLowerLimit1, hUpperLimit1-0.01);
  hUpperLimit1 = qMax(hUpperLimit1, hLowerLimit1+0.01);

  auto hMaskType2   = FConfig.value(CMaskType2).toInt();
  auto hStrength2   = FConfig.value(CStrength2).toDouble();
  auto hLowerLimit2 = FConfig.value(CLowerLimit2).toDouble();
  auto hUpperLimit2 = FConfig.value(CUpperLimit2).toDouble();
  auto hSoftness2   = FConfig.value(CSoftness2).toDouble();
  hLowerLimit2 = qMin(hLowerLimit2, hUpperLimit2-0.02);
  hUpperLimit2 = qMax(hUpperLimit2, hLowerLimit2+0.02);

  if (FColorSpace == TColorSpace::Rgb) {
    hLowerLimit1 = pow(hLowerLimit1, hInputFactor);
    hUpperLimit1 = pow(hUpperLimit1, hInputFactor);
    hLowerLimit2 = pow(hLowerLimit2, hInputFactor);
    hUpperLimit2 = pow(hUpperLimit2, hInputFactor);
    AImage->toRGB();
  } else if (FColorSpace == TColorSpace::Lab) {
    AImage->toLab();
  }

  if ((hMaskType1 != ptMaskType_None) && (hStrength1 != 0.0))
    AImage->LMHRecovery(hMaskType1, hStrength1, hLowerLimit1, hUpperLimit1, hSoftness1);
  if ((hMaskType2 != ptMaskType_None) && (hStrength2 != 0.0))
    AImage->LMHRecovery(hMaskType2, hStrength2, hLowerLimit2, hUpperLimit2, hSoftness2);
}

//==============================================================================


QWidget *ptFilter_LMHRecovery::doCreateGui() {
  auto hGuiBody = new QWidget;
  Ui_LMHRecoveryForm hForm;

  hForm.setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  return hGuiBody;
}

//==============================================================================

RegisterHelper LMHRecoveryRgbRegister(&ptFilter_LMHRecovery::createLMHRecoveryRgb, CLMHRecoveryRgbId);
RegisterHelper LMHRecoveryLabRegister(&ptFilter_LMHRecovery::createLMHRecoveryLab, CLMHRecoveryLabId);

//==============================================================================
