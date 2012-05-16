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

#include "ptFilter_SatCurve.h"
#include "ptCfgItem.h"
#include <ptImage.h>
#include <ptCurve.h>

//==============================================================================

const QString CSatCurveId = "SatCurve";

const QString CCurve    = "Curve";
const QString CMode     = "Mode";

//==============================================================================

ptFilter_SatCurve::ptFilter_SatCurve()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_SatCurve::CreateSatCurve() {
  auto hInstance         = new ptFilter_SatCurve;
  hInstance->FFilterName = CSatCurveId;
  hInstance->FCaption    = tr("Saturation curve");
  return hInstance;
}

//==============================================================================

void ptFilter_SatCurve::doDefineControls() {
  FCfgItems = QList<ptCfgItem>()                                      //--- Combo: list of entries               ---//
                                                                      //--- Check: not available                 ---//
    //            Id            Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CCurve,       ptCfgItem::CurveWin,      std::make_shared<ptCurve>(ptCurve::horizontalMidNull(),
                                                                                    ptCurve::LumaMask|ptCurve::ChromaMask,
                                                                                    ptCurve::ChromaMask,
                                                                                    ptCurve::CosineInterpol), ""})
    << ptCfgItem({CMode,        ptCfgItem::Combo,         0,          QStringList(tr("Absolute"))
                                                                               << tr("Adaptive"),                       true, true, tr("Saturation mode"),   tr("")})
  ;
}

//==============================================================================

bool ptFilter_SatCurve::doCheckHasActiveCfg() {
  return !FCfgItems[0].Curve->isNull();
}

//==============================================================================

void ptFilter_SatCurve::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->ApplySaturationCurve(FCfgItems[0].Curve.get(),
                               FConfig->getValue(CMode).toInt());
}

//==============================================================================

RegisterHelper SatCurveRegister(&ptFilter_SatCurve::CreateSatCurve, CSatCurveId);

//==============================================================================
