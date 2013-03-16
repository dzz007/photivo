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

#include "ptFilter_LumaDenoiseCurve.h"
#include "ptCfgItem.h"
#include <ptImage.h>
#include <ptCurve.h>

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include <ptProcessor.h>
extern ptProcessor* TheProcessor;

//==============================================================================

const QString CLumaDenoiseCurveId = "LumaDenoiseCurve";

const QString CCurve      = "Curve";
const QString CLStrength  = "LStrength";
const QString CLScale     = "LScale";

//==============================================================================

ptFilter_LumaDenoiseCurve::ptFilter_LumaDenoiseCurve()
: ptFilterBase()
{
  FIsSlow = true;
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_LumaDenoiseCurve::CreateLumaDenoiseCurve() {
  auto hInstance         = new ptFilter_LumaDenoiseCurve;
  hInstance->FFilterName = CLumaDenoiseCurveId;
  hInstance->FCaption    = tr("Luminance denoise curve");
  return hInstance;
}

//==============================================================================

void ptFilter_LumaDenoiseCurve::doDefineControls() {
  FConfig.initStores(TCfgItemList()                                              //--- Combo: list of entries               ---//
                                                                                 //--- Check: not available                 ---//
    //            Id           Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CCurve,      ptCfgItem::CurveWin,      std::make_shared<ptCurve>(ptCurve::horizontalMidNull(),
                                                                                   ptCurve::LumaMask|ptCurve::ChromaMask,
                                                                                   ptCurve::ChromaMask,
                                                                                   ptCurve::CosineInterpol),           tr("")})
    << ptCfgItem({CLStrength,  ptCfgItem::Slider,        0.3,        0.0,          3.0,          0.02,       2,        true, true, tr("L strength"), tr("")})
    << ptCfgItem({CLScale,     ptCfgItem::Slider,        8.0,        4.0,         50.0,          4.0,        1,        true, true, tr("L scale"), tr("")})
  );
}

//==============================================================================

bool ptFilter_LumaDenoiseCurve::doCheckHasActiveCfg() {
  return !FConfig.items()[0].Curve->isNull();
}

//==============================================================================

void ptFilter_LumaDenoiseCurve::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->ApplyDenoiseCurve(FConfig.value(CLScale).toDouble()*TheProcessor->m_ScaleFactor,
                            FConfig.value(CLStrength).toDouble()/10,
                            FConfig.items()[cfgIdx(CCurve)].Curve.get());
}

//==============================================================================

RegisterHelper LumaDenoiseCurveRegister(&ptFilter_LumaDenoiseCurve::CreateLumaDenoiseCurve, CLumaDenoiseCurveId);

//==============================================================================
