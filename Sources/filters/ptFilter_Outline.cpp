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
  FCfgItems = QList<ptCfgItem>()                                         //--- Combo: list of entries               ---//
                                                                         //--- Check: not available                 ---//
    //            Id               Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COverlayMode,    ptCfgItem::Combo,         0,          QStringList(tr("Disabled"))
                                                                                  << tr("SoftLight")
                                                                                  << tr("Multiply")
                                                                                  << tr("Screen")
                                                                                  << tr("Gamma dark")
                                                                                  << tr("Gamma bright")
                                                                                  << tr("Color burn")
                                                                                  << tr("Color dodge")
                                                                                  << tr("Darken only")
                                                                                  << tr("Lighten only")
                                                                                  << tr("Show outlines"),                  true, true, tr("Overlay mode"),   tr("")})
    << ptCfgItem({CImageOnTop,     ptCfgItem::Check,         0,                                                            true, true, tr("Image on top"), tr("Overlay the image on top of the outlines instead of vice versa")})
    << ptCfgItem({CGradientMode,   ptCfgItem::Combo,         0,          QStringList(tr("Backward finite differences"))
                                                                                  << tr("Centered finite differences")
                                                                                  << tr("Forward finite differences")
                                                                                  << tr("Sobel masks")
                                                                                  << tr("Rotation invariant masks")
                                                                                  << tr("Deriche recursive filter"),       true, true, tr("Outlines mode"),   tr("Method for calculating the outline gradients")})
    << ptCfgItem({CColorWeight,    ptCfgItem::Slider,        1.0,        0.0,          5.0,          0.5,        2,        true, true, tr("Color weight"), tr("Weight of the A/B channels in the outlines calculation")})
    << ptCfgItem({CBlurRadius,     ptCfgItem::Slider,        0.0,        0.0,         20.0,          0.2,        2,        true, true, tr("Blur radius"), tr("")})
    << ptCfgItem({CCurve,          ptCfgItem::CurveWin,      std::make_shared<ptCurve>(ptCurve::diagonalNull(),
                                                                                       ptCurve::NoMask,
                                                                                       ptCurve::NoMask,
                                                                                       ptCurve::SplineInterpol),                tr("")})
      ;
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
  return FConfig->getValue(COverlayMode).toInt() != 0;
}

//==============================================================================

void ptFilter_Outline::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->Outline(FConfig->getValue(COverlayMode).toInt(),
                  FConfig->getValue(CGradientMode).toInt(),
                  FCfgItems[cfgIdx(CCurve)].Curve.get(),
                  FConfig->getValue(CColorWeight).toDouble(),
                  FConfig->getValue(CBlurRadius).toDouble(),
                  FConfig->getValue(CImageOnTop).toBool());
}

//==============================================================================

RegisterHelper OutlineRegister(&ptFilter_Outline::CreateOutline, COutlineId);

//==============================================================================
