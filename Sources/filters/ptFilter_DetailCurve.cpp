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

#include "ptFilter_DetailCurve.h"
#include "ptCfgItem.h"
#include <ptImage.h>
#include <ptCurve.h>

//==============================================================================

const QString CDetailCurveId = "DetailCurve";

const QString CCurve       = "Curve";
const QString CHaloControl = "HaloControl";
const QString CWeight      = "Weight";
const QString CAntiBadpixel = "AntiBadpixel";

//==============================================================================

ptFilter_DetailCurve::ptFilter_DetailCurve()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_DetailCurve::CreateDetailCurve() {
  auto hInstance         = new ptFilter_DetailCurve;
  hInstance->FFilterName = CDetailCurveId;
  hInstance->FCaption    = tr("Detail curve");
  return hInstance;
}

//==============================================================================

void ptFilter_DetailCurve::doDefineControls() {
  auto hNullAnchors = TAnchorList({TAnchor(0.0, 0.2),
                                            TAnchor(0.5, 0.2),
                                            TAnchor(1.0, 0.2)});
  FCfgItems = QList<ptCfgItem>()                                                 //--- Combo: list of entries               ---//
                                                                                 //--- Check: not available                 ---//
    //            Id             Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CCurve,        ptCfgItem::CurveWin,      std::make_shared<ptCurve>(hNullAnchors,
                                                                                     ptCurve::LumaMask|ptCurve::ChromaMask,
                                                                                     ptCurve::ChromaMask,
                                                                                     ptCurve::CosineInterpol),           tr("")})
    << ptCfgItem({CHaloControl,  ptCfgItem::Slider,       40,          1,            200,         10,          0,        true, true, tr("Halo control"), tr("")})
    << ptCfgItem({CWeight,       ptCfgItem::Slider,        0.5,        0.0,            1.0,        0.05,       2,        true, true, tr("Weight"), tr("")})
    << ptCfgItem({CAntiBadpixel, ptCfgItem::Slider,        0.0,        0.0,            1.0,        0.1,        2,        true, true, tr("Anti badpixel"), tr("")})
  ;
}

//==============================================================================

bool ptFilter_DetailCurve::doCheckHasActiveCfg() {
  return !FCfgItems[0].Curve->isNull();
}

//==============================================================================

void ptFilter_DetailCurve::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->MLMicroContrast(0.15,
                          FConfig->getValue(CHaloControl).toDouble(),
                          FConfig->getValue(CWeight).toDouble(),
                          FCfgItems[cfgIdx(CCurve)].Curve.get());
  AImage->HotpixelReduction(FConfig->getValue(CAntiBadpixel).toDouble());
}

//==============================================================================

RegisterHelper DetailCurveRegister(&ptFilter_DetailCurve::CreateDetailCurve, CDetailCurveId);

//==============================================================================
