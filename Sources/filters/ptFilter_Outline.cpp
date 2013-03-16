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

#include "ptFilter_Outline.h"
#include "ui_ptFilter_Outline.h"
#include "ptCfgItem.h"
#include <ptImage.h>
#include <ptCurve.h>
#include <ptConstants.h>

//==============================================================================

const QString COutlineId = "Outline";

const QString COverlayMode   = "OverlayMode";
const QString CImageOnTop    = "ImageOnTop";
const QString CGradientMode  = "GradientMode";
const QString CColorWeight   = "ColorWeight";
const QString CBlurRadius    = "BlurRadius";
const QString CCurve         = "Curve";

//==============================================================================

ptFilter_Outline::ptFilter_Outline()
: ptFilterBase()
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_Outline::CreateOutline() {
  auto hInstance         = new ptFilter_Outline;
  hInstance->FFilterName = COutlineId;
  hInstance->FCaption    = tr("Outline");
  return hInstance;
}

//==============================================================================

void ptFilter_Outline::doDefineControls() {
  QList<ptCfgItem::TComboEntry> hOverlayModes;
  hOverlayModes.append({tr("Disabled"), ptOverlayMode_None, "none"});
  hOverlayModes.append({tr("SoftLight"), ptOverlayMode_SoftLight, "softlight"});
  hOverlayModes.append({tr("Multiply"), ptOverlayMode_Multiply, "multiply"});
  hOverlayModes.append({tr("Screen"), ptOverlayMode_Screen, "screen"});
  hOverlayModes.append({tr("Gamma dark"), ptOverlayMode_GammaDark, "gammadark"});
  hOverlayModes.append({tr("Gamma bright"), ptOverlayMode_GammaBright, "gammabright"});
  hOverlayModes.append({tr("Color burn"), ptOverlayMode_ColorBurn, "colorburn"});
  hOverlayModes.append({tr("Color dodge"), ptOverlayMode_ColorDodge, "colordodge"});
  hOverlayModes.append({tr("Darken only"), ptOverlayMode_Darken, "darken"});
  hOverlayModes.append({tr("Lighten only"), ptOverlayMode_Lighten, "lighten"});
  hOverlayModes.append({tr("Show outlines"), ptOverlayMode_Replace, "outlines"});

  QList<ptCfgItem::TComboEntry> hGradientModes;
  hGradientModes.append({tr("Backward finite differences"), ptGradientMode_Backward, "backward"});
  hGradientModes.append({tr("Centered finite differences"), ptGradientMode_Centered, "centered"});
  hGradientModes.append({tr("Forward finite differences"), ptGradientMode_Forward, "forward"});
  hGradientModes.append({tr("Sobel masks"), ptGradientMode_Sobel, "sobel"});
  hGradientModes.append({tr("Rotation invariant masks"), ptGradientMode_RotInv, "rotinv"});
  hGradientModes.append({tr("Deriche recursive filter"), ptGradientMode_Deriche, "deriche"});

  FConfig.initStores(TCfgItemList()                                      //--- Combo: list of entries               ---//
                                                                         //--- Check: not available                 ---//
    //            Id               Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COverlayMode,    ptCfgItem::Combo,         ptOverlayMode_None, hOverlayModes,                            true, true, tr("Overlay mode"),   tr("")})
    << ptCfgItem({CImageOnTop,     ptCfgItem::Check,         0,                                                            true, true, tr("Image on top"), tr("Overlay the image on top of the outlines instead of vice versa")})
    << ptCfgItem({CGradientMode,   ptCfgItem::Combo,         ptGradientMode_Backward, hGradientModes,                      true, true, tr("Outlines mode"),   tr("Method for calculating the outline gradients")})
    << ptCfgItem({CColorWeight,    ptCfgItem::Slider,        1.0,        0.0,          5.0,          0.5,        2,        true, true, tr("Color weight"), tr("Weight of the A/B channels in the outlines calculation")})
    << ptCfgItem({CBlurRadius,     ptCfgItem::Slider,        0.0,        0.0,         20.0,          0.2,        2,        true, true, tr("Blur radius"), tr("")})
    << ptCfgItem({CCurve,          ptCfgItem::CurveWin,      std::make_shared<ptCurve>(ptCurve::diagonalNull(),
                                                                                       ptCurve::NoMask,
                                                                                       ptCurve::NoMask,
                                                                                       ptCurve::SplineInterpol),                tr("")})
  );
}

//==============================================================================

QWidget *ptFilter_Outline::doCreateGui() {
  auto hGuiBody = new QWidget;
  Ui_OutlineForm hForm;

  hForm.setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  return hGuiBody;
}

//==============================================================================

bool ptFilter_Outline::doCheckHasActiveCfg() {
  return FConfig.value(COverlayMode).toInt() != 0;
}

//==============================================================================

void ptFilter_Outline::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->Outline(FConfig.value(COverlayMode).toInt(),
                  FConfig.value(CGradientMode).toInt(),
                  FConfig.items()[cfgIdx(CCurve)].Curve.get(),
                  FConfig.value(CColorWeight).toDouble(),
                  FConfig.value(CBlurRadius).toDouble(),
                  FConfig.value(CImageOnTop).toBool());
}

//==============================================================================

RegisterHelper OutlineRegister(&ptFilter_Outline::CreateOutline, COutlineId);

//==============================================================================
