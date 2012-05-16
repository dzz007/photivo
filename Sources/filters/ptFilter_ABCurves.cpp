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

#include "ptFilter_ABCurves.h"
#include "ptCfgItem.h"
#include <ptImage.h>
#include <ptCurve.h>

//==============================================================================

const QString CABCurvesId = "ABCurves";

const QString CACurve     = "ACurve";
const QString CBCurve     = "BCurve";

//==============================================================================

ptFilter_ABCurves::ptFilter_ABCurves()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_ABCurves::CreateABCurves() {
  auto hInstance         = new ptFilter_ABCurves;
  hInstance->FFilterName = CABCurvesId;
  hInstance->FCaption    = tr("a* b* curves");
  return hInstance;
}

//==============================================================================

void ptFilter_ABCurves::doDefineControls() {
  auto hNullAnchors = TAnchorList({TAnchor(0.0, 0.0),
                                            TAnchor((double)0x8080/0xffff, (double)0x8080/0xffff),
                                            TAnchor(1.0, 1.0)});
  FCfgItems = QList<ptCfgItem>()
    //            Id       Type                 Curve                                                  Caption
    << ptCfgItem({CACurve, ptCfgItem::CurveWin, std::make_shared<ptCurve>(hNullAnchors,
                                                                       ptCurve::AChannelMask,
                                                                       ptCurve::AChannelMask,
                                                                       ptCurve::SplineInterpol),  tr("")})
    << ptCfgItem({CBCurve, ptCfgItem::CurveWin, std::make_shared<ptCurve>(hNullAnchors,
                                                                       ptCurve::BChannelMask,
                                                                       ptCurve::BChannelMask,
                                                                       ptCurve::SplineInterpol),  tr("")})
  ;
}

//==============================================================================

bool ptFilter_ABCurves::doCheckHasActiveCfg() {
  return (!FCfgItems[0].Curve->isNull()) || (!FCfgItems[1].Curve->isNull());
}

//==============================================================================

void ptFilter_ABCurves::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  if (!FCfgItems[0].Curve->isNull())
    AImage->ApplyCurve(FCfgItems[0].Curve.get(), ChMask_a);
  if (!FCfgItems[1].Curve->isNull())
    AImage->ApplyCurve(FCfgItems[1].Curve.get(), ChMask_b);
}

//==============================================================================

RegisterHelper ABCurvesRegister(&ptFilter_ABCurves::CreateABCurves, CABCurvesId);

//==============================================================================
