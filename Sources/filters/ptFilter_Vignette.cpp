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
#include "../ptImage.h"
#include "../ptInfo.h"
#include "../ptDefines.h"

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

ptFilterBase *ptFilter_Vignette::createVignetteRgb() {
  auto hInstance         = new ptFilter_Vignette(TColorSpace::Rgb);
  hInstance->FFilterName = CVignetteRgbId;
  hInstance->FCaption    = tr("Vignette");
  return hInstance;
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_Vignette::createVignetteLab() {
  auto hInstance         = new ptFilter_Vignette(TColorSpace::Lab);
  hInstance->FFilterName = CVignetteLabId;
  hInstance->FCaption    = tr("Vignette");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::doDefineControls() {
  float hDefStrength = (FColorSpace == TColorSpace::Rgb) ? 0.5f  : 0.3f;
  float hDefSoftness = (FColorSpace == TColorSpace::Rgb) ? 0.15f : 0.06f;

  QList<ptCfgItem::TComboEntry> hMaskTypes;
  hMaskTypes.append({tr("Disabled"),  NoVignetteMask,    "disabled"});
  hMaskTypes.append({tr("Soft"),      SoftVignetteMask,  "soft"});
  hMaskTypes.append({tr("Hard"),      HardVignetteMask,  "hard"});
  hMaskTypes.append({tr("Fancy"),     FancyVignetteMask, "fancy"});
  hMaskTypes.append({tr("Show mask"), MaskVignetteMask,  "mask"});

  QList<ptCfgItem::TComboEntry> hShapes;
  hShapes.append({tr("Diamond"),               DiamondVignetteShape, "diamond"});
  hShapes.append({tr("Circle"),                CircleVignetteShape,  "circle"});
  hShapes.append({tr("Rectangle 1 (rounded)"), Rect1VignetteShape,   "rect1"});
  hShapes.append({tr("Rectangle 2"),           Rect2VignetteShape,   "rect2"});
  hShapes.append({tr("Rectangle 3"),           Rect3VignetteShape,   "rect3"});
  hShapes.append({tr("Rectangle 4"),           Rect4VignetteShape,   "rect4"});
  hShapes.append({tr("Rectangle 5"),           Rect5VignetteShape,   "rect5"});
  hShapes.append({tr("Rectangle 6"),           Rect6VignetteShape,   "rect6"});
  hShapes.append({tr("Rectangle 7"),           Rect7VignetteShape,   "rect7"});
  hShapes.append({tr("Rectangle 8 (sharp)"),   Rect8VignetteShape,   "rect8"});

  FCfgItems = QList<ptCfgItem>()                                              //--- Combo: list of entries               ---//
    //            Id                    Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMaskType,            ptCfgItem::Combo,         NoVignetteMask,      hMaskTypes,                              true, true, tr("Mask type"),   tr("")})
    << ptCfgItem({CShape,               ptCfgItem::Combo,         CircleVignetteShape, hShapes,                                 true, true, tr("Shape"),   tr("")})
    << ptCfgItem({CStrength,            ptCfgItem::Slider,      hDefStrength,-1.0,          1.0,          0.10,       2,        true, true, tr("Strength"), tr("")})
    << ptCfgItem({CInnerRadius,         ptCfgItem::Slider,        0.7,        0.0,          3.0,          0.10,       2,       false, true, tr("Inner radius"), tr("")})
    << ptCfgItem({COuterRadius,         ptCfgItem::Slider,        2.2,        0.0,          3.0,          0.10,       2,       false, true, tr("Outer radius"), tr("")})
    << ptCfgItem({CRoundness,           ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Roundness"), tr("")})
    << ptCfgItem({CCenterX,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.10,       2,        true, true, tr("Center X"), tr("")})
    << ptCfgItem({CCenterY,             ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.10,       2,        true, true, tr("Center Y"), tr("")})
    << ptCfgItem({CSoftness,            ptCfgItem::Slider,      hDefSoftness, 0.0,          1.0,          0.10,       2,        true, true, tr("Softness"), tr("")})
  ;

}

//------------------------------------------------------------------------------

bool ptFilter_Vignette::doCheckHasActiveCfg() {
  return (FConfig->getValue(CMaskType).value<TVignetteMask>() != NoVignetteMask) &&
         (FConfig->getValue(CStrength).toFloat() != 0.0f);
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::doRunFilter(ptImage *AImage) const {
  switch (FColorSpace) {
    case TColorSpace::Rgb: AImage->toRGB();
    case TColorSpace::Lab: AImage->toLab();
    default:               GInfo->Raise("Unknown color space", AT);
  }

  AImage->Vignette(FConfig->getValue(CMaskType).value<TVignetteMask>(),
                   FConfig->getValue(CShape).toInt(),
                   FConfig->getValue(CStrength).toFloat(),
                   FConfig->getValue(CInnerRadius).toFloat(),
                   FConfig->getValue(COuterRadius).toFloat(),
                   FConfig->getValue(CRoundness).toFloat() / 2.0f,
                   FConfig->getValue(CCenterX).toFloat(),
                   FConfig->getValue(CCenterY).toFloat(),
                   FConfig->getValue(CSoftness).toFloat() );
}

//------------------------------------------------------------------------------

QWidget *ptFilter_Vignette::doCreateGui() {
  auto hGuiBody = new QWidget;
  if (!FUi)
    FUi = make_unique<Ui_VignetteForm>();

  FUi->setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  return hGuiBody;
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::innerRadiusChanged(const QString, const QVariant ANewValue) {
  if (!ANewValue.isValid()) return;

  QVariant hRadius = qMin(ANewValue.toFloat(), FConfig->getValue(COuterRadius).toFloat());
  FConfig->setValue(CInnerRadius, hRadius);
  FUi->InnerRadius->setValue(hRadius);

  requestPipeRun();
}

//------------------------------------------------------------------------------

void ptFilter_Vignette::outerRadiusChanged(const QString, const QVariant ANewValue) {
  if (!ANewValue.isValid()) return;

  QVariant hRadius = qMax(ANewValue.toFloat(), FConfig->getValue(CInnerRadius).toFloat());
  FConfig->setValue(COuterRadius, hRadius);
  FUi->OuterRadius->setValue(hRadius);

  requestPipeRun();
}

//------------------------------------------------------------------------------

RegisterHelper VignetteRegisterRgb(&ptFilter_Vignette::createVignetteRgb, CVignetteRgbId);
RegisterHelper VignetteRegisterLab(&ptFilter_Vignette::createVignetteLab, CVignetteLabId);
