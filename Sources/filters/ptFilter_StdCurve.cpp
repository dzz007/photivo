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


#include "ptFilter_StdCurve.h"
#include "ptCfgItem.h"
#include "../ptImage.h"
#include "../ptCurveWindow.h"

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

#include <QVBoxLayout>

//==============================================================================

const QString CRgbCurveId         = "RgbCurve";
const QString CTextureCurveId     = "TextureCurve";
const QString CLumaByHueCurveId   = "LumaByHueCurve";
const QString CHueCurveId         = "HueCurve";
const QString CLCurveId           = "LCurve";
const QString CRToneCurveId       = "RToneCurve";
const QString CGToneCurveId       = "GToneCurve";
const QString CBToneCurveId       = "BToneCurve";
const QString CAfterGammaCurveId  = "AfterGammaCurve";

const QString CCurveObject        = "Curve";

//==============================================================================

ptFilter_StdCurve::ptFilter_StdCurve(std::shared_ptr<ptCurve> ACurve)
: ptFilterBase()
{
  // Create the config item for the curve. ptFilterBase is smart enough to distinguish
  // between default store widgets and widgets with their own custom store.
  FConfig.initStores(TCfgItemList()
    << ptCfgItem({CCurveObject, ptCfgItem::CurveWin, ACurve, ""})
  );

  internalInit();
}

//==============================================================================

void ptFilter_StdCurve::doRunFilter(ptImage *AImage) const {
  if (FFilterName =="RgbCurve") {
    AImage->toRGB();
    AImage->ApplyCurve(FConfig.items()[0].Curve.get(), ChMask_RGB);

  } else if (FFilterName == "TextureCurve") {
    AImage->toLab();
    AImage->ApplyTextureCurve(FConfig.items()[0].Curve.get(),
                              (int) (logf(TheProcessor->m_ScaleFactor)/logf(0.5)));

  } else if (FFilterName == "LumaByHueCurve") {
    AImage->toLab();
    AImage->ApplyLByHueCurve(FConfig.items()[0].Curve.get());

  } else if (FFilterName == "HueCurve") {
    AImage->toLab();
    AImage->ApplyHueCurve(FConfig.items()[0].Curve.get());

  } else if (FFilterName == "LCurve") {
    AImage->toLab();
    AImage->ApplyCurve(FConfig.items()[0].Curve.get(), ChMask_L);

  } else if (FFilterName == "RToneCurve") {
    AImage->toRGB();
    AImage->ApplyCurve(FConfig.items()[0].Curve.get(), ChMask_R);

  } else if (FFilterName == "GToneCurve") {
    AImage->toRGB();
    AImage->ApplyCurve(FConfig.items()[0].Curve.get(), ChMask_G);

  } else if (FFilterName == "BToneCurve") {
    AImage->toRGB();
    AImage->ApplyCurve(FConfig.items()[0].Curve.get(), ChMask_B);

  } else if (FFilterName == "AfterGammaCurve") {
    AImage->ApplyCurve(FConfig.items()[0].Curve.get(), ChMask_RGB);
  }
}

//==============================================================================

bool ptFilter_StdCurve::doCheckHasActiveCfg() {
  return !FConfig.items()[0].Curve->isNull();
}

//==============================================================================

void ptFilter_StdCurve::doDefineControls() {
  /* nothing to do */
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateRgbCurve() {
  auto hInstance = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                           ptCurve::diagonalNull(),
                                           ptCurve::GammaMask,
                                           ptCurve::GammaMask,
                                           ptCurve::SplineInterpol));
  hInstance->FFilterName = CRgbCurveId;
  hInstance->FCaption    = tr("RGB curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateTextureCurve() {
  auto hInstance = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                           ptCurve::horizontalQuarterNull(),
                                           ptCurve::LumaMask|ptCurve::ChromaMask,
                                           ptCurve::ChromaMask,
                                           ptCurve::CosineInterpol));
  hInstance->FFilterName = CTextureCurveId;
  hInstance->FCaption    = tr("Texture curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateLumaByHueCurve() {
  auto hInstance = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                           ptCurve::horizontalMidNull(),
                                           ptCurve::ChromaMask,
                                           ptCurve::ChromaMask,
                                           ptCurve::CosineInterpol));
  hInstance->FFilterName = CLumaByHueCurveId;
  hInstance->FCaption    = tr("Luminance by hue curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateHueCurve() {
  auto hInstance = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                           ptCurve::horizontalMidNull(),
                                           ptCurve::LumaMask|ptCurve::ChromaMask,
                                           ptCurve::ChromaMask,
                                           ptCurve::CosineInterpol));
  hInstance->FFilterName = CHueCurveId;
  hInstance->FCaption    = tr("Hue curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateLCurve() {
  auto hInstance = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                           ptCurve::diagonalNull(),
                                           ptCurve::LumaMask,
                                           ptCurve::LumaMask,
                                           ptCurve::SplineInterpol));
  hInstance->FFilterName = CLCurveId;
  hInstance->FCaption    = tr("L* curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateRToneCurve() {
  auto hInstance = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                           ptCurve::diagonalNull(),
                                           ptCurve::NoMask,
                                           ptCurve::NoMask,
                                           ptCurve::SplineInterpol));
  hInstance->FFilterName = CRToneCurveId;
  hInstance->FCaption    = tr("R tone curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateGToneCurve() {
  auto hInstance         = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                                   ptCurve::diagonalNull(),
                                                   ptCurve::NoMask,
                                                   ptCurve::NoMask,
                                                   ptCurve::SplineInterpol));
  hInstance->FFilterName = CGToneCurveId;
  hInstance->FCaption    = tr("G tone curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateBToneCurve() {
  auto hInstance         = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                                   ptCurve::diagonalNull(),
                                                   ptCurve::NoMask,
                                                   ptCurve::NoMask,
                                                   ptCurve::SplineInterpol));
  hInstance->FFilterName = CBToneCurveId;
  hInstance->FCaption    = tr("B tone curve");
  return hInstance;
}

//==============================================================================

ptFilterBase *ptFilter_StdCurve::CreateAfterGammaCurve() {
  auto hInstance         = new ptFilter_StdCurve(std::make_shared<ptCurve>(
                                                   ptCurve::diagonalNull(),
                                                   ptCurve::LumaMask,
                                                   ptCurve::LumaMask,
                                                   ptCurve::SplineInterpol));
  hInstance->FFilterName = CAfterGammaCurveId;
  hInstance->FCaption    = tr("After gamma curve");
  return hInstance;
}

//==============================================================================

RegisterHelper RgbCurveRegister         (&ptFilter_StdCurve::CreateRgbCurve,          CRgbCurveId);
RegisterHelper TextureCurveRegister     (&ptFilter_StdCurve::CreateTextureCurve,      CTextureCurveId);
RegisterHelper LumaByHueCurveRegister   (&ptFilter_StdCurve::CreateLumaByHueCurve,    CLumaByHueCurveId);
RegisterHelper HueCurveRegister         (&ptFilter_StdCurve::CreateHueCurve,          CHueCurveId);
RegisterHelper LCurveRegister           (&ptFilter_StdCurve::CreateLCurve,            CLCurveId);
RegisterHelper RToneCurveRegister       (&ptFilter_StdCurve::CreateRToneCurve,        CRToneCurveId);
RegisterHelper GToneCurveRegister       (&ptFilter_StdCurve::CreateGToneCurve,        CGToneCurveId);
RegisterHelper BToneCurveRegister       (&ptFilter_StdCurve::CreateBToneCurve,        CBToneCurveId);
RegisterHelper AfterGammeCurveRegister  (&ptFilter_StdCurve::CreateAfterGammaCurve,   CAfterGammaCurveId);

//==============================================================================
