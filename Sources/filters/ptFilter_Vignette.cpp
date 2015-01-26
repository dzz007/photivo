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

#include "ptFilter_Vignette.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptDefines.h"
#include "../ptGuiOptions.h"
#include "../ptImage.h"
#include "../ptInfo.h"

//------------------------------------------------------------------------------

const QString CVignetteRgbId = "VignetteRgb";
const QString CVignetteLabId = "VignetteLab";

const QString CMaskType    = "MaskType";
const QString CShape       = "Shape";
const QString CStrength    = "Strength";
const QString CInnerRadius = "InnerRadius";
const QString COuterRadius = "OuterRadius";
const QString CRoundness   = "Roundness";
const QString CCenterX     = "CenterX";
const QString CCenterY     = "CenterY";
const QString CSoftness    = "Softness";

//------------------------------------------------------------------------------

ptFilter_Vignette::ptFilter_Vignette(TColorSpace AColorSpace)
: ptFilterBase(),
  FColorSpace(AColorSpace)
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_Vignette::createVignetteRgb() {
  auto hInstance         = new ptFilter_Vignette(TColorSpace::Rgb);
  hInstance->FFilterName = CVignetteRgbId;
  hInstance->FCaption    = tr("Vignette");
  return hInstance;
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_Vignette::createVignetteLab() {
  auto instance         = new ptFilter_Vignette(TColorSpace::Lab);
  instance->FFilterName = CVignetteLabId;
  instance->FCaption    = tr("Vignette");
  return instance;
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::doDefineControls() {
  double defStrength = (FColorSpace == TColorSpace::Rgb) ? 0.5  : 0.3;
  double defSoftness = (FColorSpace == TColorSpace::Rgb) ? 0.15 : 0.06;

  ptCfgItem::TComboEntryList maskTypes;
  maskTypes.append({tr("Disabled"),  static_cast<int>(TVignetteMask::Disabled), "Disabled"});
  maskTypes.append({tr("Soft"),      static_cast<int>(TVignetteMask::Soft),     "Soft"});
  maskTypes.append({tr("Hard"),      static_cast<int>(TVignetteMask::Hard),     "Hard"});
  maskTypes.append({tr("Fancy"),     static_cast<int>(TVignetteMask::Fancy),    "Fancy"});
  maskTypes.append({tr("Show mask"), static_cast<int>(TVignetteMask::ShowMask), "ShowMask"});

  FConfig.initStores(TCfgItemList()                                           //--- Combo: list of entries               ---//
    //            Id                    Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaskType,            ptCfgItem::Combo,         static_cast<int>(TVignetteMask::Disabled), maskTypes,         true, true, tr("Mask type"),    tr("Mask type")})
    << ptCfgItem({CShape,               ptCfgItem::Combo,         static_cast<int>(TVignetteShape::Circle), pt::ComboEntries::VignetteShapes, true, true, tr("Shape"),        tr("Shape of the vignette")})
    << ptCfgItem({CStrength,            ptCfgItem::Slider,      defStrength, -1.0,          1.0,          0.10,       2,        true, true, tr("Strength"),     tr("")})
    << ptCfgItem({CInnerRadius,         ptCfgItem::Slider,        0.7,        0.0,          3.0,          0.10,       2,       false, true, tr("Inner radius"), tr("")})
    << ptCfgItem({COuterRadius,         ptCfgItem::Slider,        2.2,        0.0,          3.0,          0.10,       2,       false, true, tr("Outer radius"), tr("")})
    << ptCfgItem({CRoundness,           ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Roundness"),    tr("")})
    << ptCfgItem({CCenterX,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.10,       2,        true, true, tr("Horizontal center"),     tr("")})
    << ptCfgItem({CCenterY,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.10,       2,        true, true, tr("Vertical center"),     tr("")})
    << ptCfgItem({CSoftness,            ptCfgItem::Slider,      defSoftness,  0.0,          1.0,          0.10,       2,        true, true, tr("Softness"),     tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_Vignette::doCheckHasActiveCfg() {
  return
      (static_cast<TVignetteMask>(FConfig.value(CMaskType).toInt()) != TVignetteMask::Disabled) &&
      !qFuzzyIsNull(FConfig.value(CStrength).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::doRunFilter(ptImage *AImage) const {
  switch (FColorSpace) {
    case TColorSpace::Rgb: AImage->toRGB(); break;
    case TColorSpace::Lab: AImage->toLab(); break;
    default:               GInfo->Raise("Unknown color space", AT); break;
  }

  AImage->Vignette(
      static_cast<TVignetteMask>(FConfig.value(CMaskType).toInt()),
      static_cast<TVignetteShape>(FConfig.value(CShape).toInt()),
      FConfig.value(CStrength).toDouble(),
      FConfig.value(CInnerRadius).toDouble(),
      FConfig.value(COuterRadius).toDouble(),
      FConfig.value(CRoundness).toDouble() / 2.0,
      FConfig.value(CCenterX).toDouble(),
      FConfig.value(CCenterY).toDouble(),
      FConfig.value(CSoftness).toDouble() );
}

//------------------------------------------------------------------------------

QWidget* ptFilter_Vignette::doCreateGui() {
  auto guiBody = new QWidget;

  FForm.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  connect(FForm.InnerRadius,
          SIGNAL(valueChanged(QString,QVariant)),
          SLOT(onInnerRadiusChanged(QString,QVariant)));
  connect(FForm.OuterRadius,
          SIGNAL(valueChanged(QString,QVariant)),
          SLOT(onOuterRadiusChanged(QString,QVariant)));

  return guiBody;
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::onInnerRadiusChanged(const QString, const QVariant ANewValue) {
  if (!ANewValue.isValid()) {
    return;
  }

  QVariant hRadius = qMin(ANewValue.toDouble(), FConfig.value(COuterRadius).toDouble());
  FConfig.setValue(CInnerRadius, hRadius);
  FForm.InnerRadius->setValue(hRadius);

  this->requestPipeRun();
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::onOuterRadiusChanged(const QString, const QVariant ANewValue) {
  if (!ANewValue.isValid()) {
    return;
  }

  QVariant hRadius = qMax(ANewValue.toDouble(), FConfig.value(CInnerRadius).toDouble());
  FConfig.setValue(COuterRadius, hRadius);
  FForm.OuterRadius->setValue(hRadius);

  this->requestPipeRun();
}

//------------------------------------------------------------------------------

RegisterHelper VignetteRegisterRgb(&ptFilter_Vignette::createVignetteRgb, CVignetteRgbId);
RegisterHelper VignetteRegisterLab(&ptFilter_Vignette::createVignetteLab, CVignetteLabId);

// -----------------------------------------------------------------------------
