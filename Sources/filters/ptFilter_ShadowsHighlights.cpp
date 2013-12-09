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

#include "ptFilter_ShadowsHighlights.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptCurve.h"

//==============================================================================

const QString CShadowsHighlightsId = "ShadowsHighlights";

const QString CFineDetail   = "FineDetail";
const QString CCoarseDetail = "CoarseDetail";
const QString CScale        = "Scale";
const QString CCurve        = "Curve";

//==============================================================================

ptFilter_ShadowsHighlights::ptFilter_ShadowsHighlights()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ShadowsHighlights::CreateShadowsHighlights() {
  auto hInstance         = new ptFilter_ShadowsHighlights;
  hInstance->FFilterName = CShadowsHighlightsId;
  hInstance->FCaption    = tr("Shadows/Highlights");
  return hInstance;
}

//==============================================================================

void ptFilter_ShadowsHighlights::doDefineControls() {
  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
                                                                                 //--- Check: not available                 ---//
    //            Id                       Type                      Default     Min           Max           Step       Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CFineDetail,             ptCfgItem::Slider,        0.0,       -10.0,        10.0,          0.5,       1,        true, true, tr("Fine detail") ,tr("")})
    << ptCfgItem({CCoarseDetail,           ptCfgItem::Slider,        0.0,       -10.0,        10.0,          0.5,       1,        true, true, tr("Coarse detail") ,tr("")})
    << ptCfgItem({CScale,                  ptCfgItem::Slider,       10.0,        0.0,         30.0,          4.0,       1,        true, true, tr("Scale") ,tr("")})
    << ptCfgItem({CCurve,                  ptCfgItem::CurveWin,      std::make_shared<ptCurve>(ptCurve::diagonalNull(),
                                                                                               ptCurve::NoMask,
                                                                                               ptCurve::NoMask,
                                                                                               ptCurve::SplineInterpol),          tr("")})
  );
}

//==============================================================================

bool ptFilter_ShadowsHighlights::doCheckHasActiveCfg() {
  return (FConfig.value(CFineDetail).toFloat()   != 0.0f) ||
         (FConfig.value(CCoarseDetail).toFloat() != 0.0f) ||
         (!FConfig.items()[cfgIdx(CCurve)].Curve->isNull());
}

//==============================================================================

void ptFilter_ShadowsHighlights::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->ShadowsHighlights(FConfig.items()[cfgIdx(CCurve)].Curve.get(),
                            FConfig.value(CScale).toDouble(),
                            FConfig.value(CCoarseDetail).toDouble(),
                            FConfig.value(CFineDetail).toDouble() );
}

//==============================================================================

RegisterHelper ShadowsHighlightsRegister(&ptFilter_ShadowsHighlights::CreateShadowsHighlights, CShadowsHighlightsId);

//==============================================================================
