/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012-2015 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptFilter_Exposure.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptImage.h"
#include "../ptSettings.h"
#include <cassert>

// TODO: Needed for access to m_ScaleFactor. Find a way to avoid when modernising the processor.
#include "../ptProcessor.h"
extern ptProcessor* TheProcessor;

// -----------------------------------------------------------------------------

const QString CExposureId = "Exposure";

const QString CExposureMode  = "ExposureMode";
const QString CClipMode      = "ClipMode";
const QString CExposure      = "Exposure";
const QString CWhiteFraction = "WhiteFraction";
const QString CWhiteLevel    = "WhiteLevel";

// -----------------------------------------------------------------------------

ptFilter_Exposure::ptFilter_Exposure():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

// -----------------------------------------------------------------------------

ptFilterBase* ptFilter_Exposure::createExposure() {
  auto instance         = new ptFilter_Exposure;
  instance->FFilterName = CExposureId;
  instance->FCaption    = tr("Exposure");
  return instance;
}

// -----------------------------------------------------------------------------

void ptFilter_Exposure::doDefineControls() {
  ptCfgItem::TComboEntryList expModes({
    {tr("Manual"),     static_cast<int>(TMode::Manual),    "Manual"},
    {tr("Auto"),       static_cast<int>(TMode::Auto),      "Auto"},
    {tr("Like UFRaw"), static_cast<int>(TMode::LikeUfraw), "LikeUfraw"}
  });

  ptCfgItem::TComboEntryList clipModes({
    {tr("Hard"),       static_cast<int>(TExposureClipMode::Hard),      "Hard"},
    {tr("Ratio"),      static_cast<int>(TExposureClipMode::Ratio),     "Ratio"},
    {tr("Film curve"), static_cast<int>(TExposureClipMode::FilmCurve), "FilmCurve"}
  });

  FConfig.initStores(TCfgItemList()                                         //--- Combo: list of entries               ---//
    //            Id                  Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CExposureMode,      ptCfgItem::Combo,         static_cast<int>(TMode::Manual), expModes,                    false, true, tr("Exposure mode"),   tr("Exposure mode")})
    << ptCfgItem({CClipMode,          ptCfgItem::Combo,         static_cast<int>(TExposureClipMode::FilmCurve), clipModes,    true,  true, tr("Clipping mode"),   tr("Clipping mode")})
    << ptCfgItem({CExposure,          ptCfgItem::Slider,        0.0,       -5.0,          5.0,          0.1,        2,        true,  true, tr("Exposure correction"), tr("Exposure correction in f-stops")})
    << ptCfgItem({CWhiteFraction,     ptCfgItem::Slider,        10,         1,            50,           1,          0,        true,  true, tr("Target white percentage"), tr("Percentage of the image that should become white")})
    << ptCfgItem({CWhiteLevel,        ptCfgItem::Slider,        90,         50,           99,           1,          0,        true,  true, tr("White level"), tr("Brightness of the “white”")})
  );
}

// -----------------------------------------------------------------------------

bool ptFilter_Exposure::doCheckHasActiveCfg() {
  return
      (static_cast<TMode>(FConfig.value(CExposureMode).toInt()) != TMode::Manual) ||
      !qFuzzyIsNull(FConfig.value(CExposure).toDouble());
}

// -----------------------------------------------------------------------------

namespace pt {
  namespace detail {
    // FindAlpha (helper for ExposureFunction)
    // such that f(x)=(1-exp(-Alpha*x)/(1-exp(-Alpha))
    // f(x) is then foto curve for which :
    // f(0) = 0
    // f(1) = 1
    // f'(0)= Exposure
    double findAlpha(double Exposure) {
      assert(Exposure > 1.0);

      double Alpha;
      double fDerivedAt0;
      if (Exposure<2) {
        Alpha=(Exposure-1)/2;
      } else {
        Alpha=Exposure;
      }
      fDerivedAt0 = Alpha/(1-exp(-Alpha));
      while (fabs(fDerivedAt0-Exposure)>0.001) {
        Alpha = Alpha + (Exposure-fDerivedAt0);
        fDerivedAt0 = Alpha/(1-exp(-Alpha));
      }
      return Alpha;
    }

    // ExposureFunction
    // (used to generate an expsure curve)
    static double Alpha;
    static double ExposureForWhichAlphaIsCalculated = -1.0;

    double exposureFunction(double r, double Exposure, double) {
      if (Exposure != ExposureForWhichAlphaIsCalculated) {
        Alpha = findAlpha(Exposure);
      }
      return (1-exp(-Alpha*r))/(1-exp(-Alpha));
    }
  }
}

// -----------------------------------------------------------------------------

void ptFilter_Exposure::doRunFilter(ptImage* AImage) {
  double ev = 0.0;

  switch (static_cast<TMode>(FConfig.value(CExposureMode).toInt())) {
    case TMode::Auto:
      ev = this->calcAutoExposure();
      FConfig.setValue(CExposure, ev);
      this->updateGui(false);
      break;

    case TMode::LikeUfraw:
      ev = Settings->GetDouble("ExposureNormalization");
      FConfig.setValue(CExposure, ev);
      this->updateGui(false);
      break;

    case TMode::Manual:
      // no exposure value to set; is already chosen by the user
      ev = FConfig.value(CExposure).toDouble();
      break;

    default:
      assert("Unexpected exposure mode.");
  }

  if (qFuzzyIsNull(ev)) {
    return;
  }

  AImage->toRGB();

  auto exposureFactor = pow(2, ev);
  auto clipMode = static_cast<TExposureClipMode>(FConfig.value(CClipMode).toInt());

  if ((clipMode != TExposureClipMode::FilmCurve) || (exposureFactor <= 1.0)) {
    AImage->Expose(exposureFactor, clipMode);
  } else {
    auto exposureCurve = make_unique<ptCurve>();
    exposureCurve->setFromFunc(pt::detail::exposureFunction, exposureFactor, 0);
    AImage->ApplyCurve(exposureCurve.get(), ChMask_RGB);
  }
}

// -----------------------------------------------------------------------------

double ptFilter_Exposure::calcAutoExposure() const {
  auto whiteFrac = TheProcessor->m_Image_AfterGeometry->CalculateFractionLevel(
      FConfig.value(CWhiteFraction).toInt() / 100.0,
      ChMask_RGB);

  return log(FConfig.value(CWhiteLevel).toInt() / 100.0 * 0xFFFF / whiteFrac) / log(2);
}

// -----------------------------------------------------------------------------

QWidget *ptFilter_Exposure::doCreateGui() {
  auto guiBody = new QWidget;

  FForm.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  connect(
      FForm.ExposureMode,
      SIGNAL(valueChanged(QString,QVariant)),
      SLOT(onExposureModeChanged(QString,QVariant)));

  return guiBody;
}

// -----------------------------------------------------------------------------

void ptFilter_Exposure::onExposureModeChanged(QString AId, QVariant ANewValue) {
  auto newValue = static_cast<TMode>(ANewValue.toInt());

  FForm.Exposure->setEnabled(newValue == TMode::Manual);
  FForm.AutoCfgWidget->setVisible(newValue == TMode::Auto);

  FConfig.setValue(AId, ANewValue);
  this->updateGui();
}

// -----------------------------------------------------------------------------

RegisterHelper ExposureRegister(&ptFilter_Exposure::createExposure, CExposureId);

// -----------------------------------------------------------------------------
