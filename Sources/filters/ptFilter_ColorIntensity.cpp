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

#include "ptFilter_ColorIntensity.h"
#include "ui_ptFilter_ColorIntensity.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CColorIntensityId = "ColorIntensity";

const QString CVibrance     = "Vibrance";
const QString CRed          = "Red";
const QString CGreen        = "Green";
const QString CBlue         = "Blue";

//==============================================================================

ptFilter_ColorIntensity::ptFilter_ColorIntensity()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ColorIntensity::createColorIntensity() {
  auto hInstance         = new ptFilter_ColorIntensity;
  hInstance->FFilterName = CColorIntensityId;
  hInstance->FCaption    = tr("Color Intensity");
  return hInstance;
}

//==============================================================================

void ptFilter_ColorIntensity::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default    Min           Max           Step     Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CVibrance,               ptCfgItem::Slider,        0,        -100,          100,          5,       0,        true, true, tr("Vibrance"), tr("")})
    << ptCfgItem({CRed,                    ptCfgItem::Slider,        0,        -100,          100,          5,       0,        true, true, tr("Red"), tr("")})
    << ptCfgItem({CGreen,                  ptCfgItem::Slider,        0,        -100,          100,          5,       0,        true, true, tr("Green"), tr("")})
    << ptCfgItem({CBlue,                   ptCfgItem::Slider,        0,        -100,          100,          5,       0,        true, true, tr("Blue"), tr("")})
  );
}

//==============================================================================

QWidget *ptFilter_ColorIntensity::doCreateGui() {
  auto hGuiBody = new QWidget;
  Ui_ColorIntensityForm hForm;

  hForm.setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  return hGuiBody;
}

//==============================================================================

bool ptFilter_ColorIntensity::doCheckHasActiveCfg() {
  return (FConfig.value(CVibrance).toInt() != 0) ||
         (FConfig.value(CRed).toInt() != 0) ||
         (FConfig.value(CGreen).toInt() != 0) ||
         (FConfig.value(CBlue).toInt() != 0);
}

//==============================================================================

void ptFilter_ColorIntensity::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  AImage->ColorIntensity(FConfig.value(CVibrance).toInt(),
                         FConfig.value(CRed).toInt(),
                         FConfig.value(CGreen).toInt(),
                         FConfig.value(CBlue).toInt());
}

//==============================================================================

RegisterHelper ColorIntensityRegister(&ptFilter_ColorIntensity::createColorIntensity, CColorIntensityId);

//==============================================================================
