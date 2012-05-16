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

#include "ptFilter_SpotTuning.h"
#include "ptTuningSpot.h"
#include "ptCfgItem.h"
#include <ptImage.h>
#include <ptCurve.h>

//==============================================================================

const QString CSpotTuningId       = "SpotTuning";
const QString CSpotListId         = "SpotList";
const QString CSpotLumaCurveWinId = "CurveWin";
// All other id constants are included from ptTuningSpot.h

//==============================================================================

ptFilter_SpotTuning::ptFilter_SpotTuning()
: ptFilterBase(),
  FSpots(ptImageSpotList(&this->createSpot))
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_SpotTuning::CreateSpotTuning() {
  auto hInstance         = new ptFilter_SpotTuning;
  hInstance->FFilterName = CSpotTuningId;
  hInstance->FCaption    = tr("Spot tuning");
  return hInstance;
}

//==============================================================================

void ptFilter_SpotTuning::doDefineControls() {
  TAnchorList hNullAnchors = { TAnchor(0.0, 0.0), TAnchor(0.4, 0.6), TAnchor(1.0, 1.0) };
  FCfgItems = QList<ptCfgItem>()                                         //--- Combo: list of entries               ---//
                                                                         //--- Check: not available                 ---//
    //            Id               Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CSpotHasMaxRadiusId,   ptCfgItem::Check,         0,                                                            false, false, tr("Use maximum radius"), tr("")})
    << ptCfgItem({CSpotMaxRadiusId,      ptCfgItem::Slider,        500,        1,            7000,         10,         0,        false, false, tr("Maximum radius"), tr("Pixels outside this radius will never be included in the mask.")})
    << ptCfgItem({CSpotChromaWeightId,   ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        2,        false, false, tr("Brightness/color ratio"), tr("Defines how brightness and color affect the threshold.\n0.0: ignore color, 1.0: ignore brightness, 0.5: equal weight for both")})
    << ptCfgItem({CSpotThresholdId,      ptCfgItem::Slider,        0.25,       0.0,          1.0,          0.05,       3,        false, false, tr("Threshold"), tr("Maximum amount a pixel may differ from the spot's source pixel to get included in the mask.")})
    << ptCfgItem({CSpotSaturationId,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, false, tr("Saturation"), tr("")})
    << ptCfgItem({CSpotIsAdaptiveSatId,  ptCfgItem::Check,         0,                                                            false, false, tr("Adaptive saturation"), tr("Prevent clipping when adjusting saturation")})
    << ptCfgItem({CSpotColorShiftId,     ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.001,      3,        false, false, tr("Color shift"), tr("")})
  ;

  FConfig->insertComplexStore(CSpotListId, &FSpots);
}

//==============================================================================

QWidget *ptFilter_SpotTuning::doCreateGui() {
  auto hGuiBody = new QWidget;
  FGui = make_unique<Ui::Form>();

  FGui->setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  FGui->SpotList->init(&FSpots);
  FGui->CurveWin->setCaption(tr("Luminance curve"));

  return hGuiBody;
}

//==============================================================================

bool ptFilter_SpotTuning::doCheckHasActiveCfg() {
  return FSpots.hasEnabledSpots();
}

//==============================================================================

void ptFilter_SpotTuning::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  for (auto hSpot: *FSpots) {
    if (hSpot->isEnabled()) {
      ptTuningSpot *hTSpot = static_cast<ptTuningSpot*>(hSpot);
      AImage->MaskedColorAdjust(hTSpot->x(),
                                hTSpot->y(),
                                hTSpot->getValue(CSpotThresholdId).toFloat(),
                                hTSpot->getValue(CSpotChromaWeightId).toFloat(),
                                hTSpot->getValue(CSpotMaxRadiusId).toInt(),
                                hTSpot->getValue(CSpotHasMaxRadiusId).toBool(),
                                hTSpot->curve(),
                                hTSpot->getValue(CSpotIsAdaptiveSatId).toBool(),
                                hTSpot->getValue(CSpotSaturationId).toFloat(),
                                hTSpot->getValue(CSpotColorShiftId).toFloat());
    }
  }
}

//==============================================================================

ptImageSpot *ptFilter_SpotTuning::createSpot() {
  return new ptTuningSpot(&FCfgItems);
}

//==============================================================================

RegisterHelper SpotTuningRegister(&ptFilter_SpotTuning::CreateSpotTuning, CSpotTuningId);

//==============================================================================
