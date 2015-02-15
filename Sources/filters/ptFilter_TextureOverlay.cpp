/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2015 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptFilter_TextureOverlay.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptGuiOptions.h"
#include "../ptMessageBox.h"
#include "../ptSettings.h"
#include <QFileDialog>
#include <QFileInfo>

extern QString BitmapPattern;

//------------------------------------------------------------------------------

const QString CTextureOverlayId = "TextureOverlay";

const QString COverlayMode    = "OverlayMode";
const QString CMaskMode       = "MaskMode";
const QString CImageFilePath  = "ImageFilePath";
const QString COpacity        = "Opacity";
const QString CSaturation     = "Saturation";
const QString CVigShape       = "VigShape";
const QString CVigInnerRadius = "VigInnerRadius";
const QString CVigOuterRadius = "VigOuterRadius";
const QString CVigRoundness   = "VigRoundness";
const QString CVigCenterX     = "VigCenterX";
const QString CVigCenterY     = "VigCenterY";
const QString CVigSoftness    = "VigSoftness";

//------------------------------------------------------------------------------

ptFilter_TextureOverlay::ptFilter_TextureOverlay():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_TextureOverlay::createTextureOverlay() {
  auto instance         = new ptFilter_TextureOverlay;
  instance->FFilterName = CTextureOverlayId;
  instance->FCaption    = tr("Texture overlay");
  return instance;
}

//------------------------------------------------------------------------------

void ptFilter_TextureOverlay::doDefineControls() {
  const ptCfgItem::TComboEntryList maskTypes({
    {QObject::tr("Full image"),        static_cast<int>(TTextureOverlayMaskMode::Full),        "Full"},
    {QObject::tr("Vignette"),          static_cast<int>(TTextureOverlayMaskMode::Vignette),    "Vignette"},
    {QObject::tr("Inverted vignette"), static_cast<int>(TTextureOverlayMaskMode::InvVignette), "InvVignette"},
  });

  FConfig.initStores(TCfgItemList()                                      //--- Combo: list of entries               ---//
    //            Id               Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COverlayMode,    ptCfgItem::Combo,         static_cast<int>(TOverlayMode::Disabled), pt::ComboEntries::OverlayModes, true, true, tr("Overlay mode"), tr("Overlay mode")})
    << ptCfgItem({CMaskMode,       ptCfgItem::Combo,         static_cast<int>(TTextureOverlayMaskMode::Full), maskTypes,  false, true, tr("Mask mode"), tr("Mask mode")})
    << ptCfgItem({CImageFilePath, QVariant(QString()), true})
    << ptCfgItem({COpacity,        ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        2,        true, true, tr("Opacity"), tr("")})
    << ptCfgItem({CSaturation,     ptCfgItem::Slider,        1.0,        0.0,          2.0,          0.1,        2,        true, true, tr("Saturation"), tr("")})
    << ptCfgItem({CVigShape,       ptCfgItem::Combo,         static_cast<int>(TVignetteShape::Circle), pt::ComboEntries::VignetteShapes, true, true, tr("Shape"), tr("Shape of the vignette")})
    << ptCfgItem({CVigInnerRadius, ptCfgItem::Slider,        0.7,        0.0,          3.0,          0.1,        2,       false, true, tr("Inner radius"), tr("")})
    << ptCfgItem({CVigOuterRadius, ptCfgItem::Slider,        2.2,        0.0,          3.0,          0.1,        2,       false, true, tr("Outer radius"), tr("")})
    << ptCfgItem({CVigRoundness,   ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Roundness"), tr("")})
    << ptCfgItem({CVigCenterX,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        true, true, tr("Horizontal center"), tr("")})
    << ptCfgItem({CVigCenterY,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Vertical center"), tr("")})
    << ptCfgItem({CVigSoftness,    ptCfgItem::Slider,        0.15,       0.0,          1.0,          0.1,        2,        true, true, tr("Softness"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_TextureOverlay::doCheckHasActiveCfg() {
  return
      (static_cast<TOverlayMode>(FConfig.value(COverlayMode).toInt()) != TOverlayMode::Disabled)
      && !FConfig.value(CImageFilePath).toString().isEmpty()
      && !qFuzzyIsNull(FConfig.value(COpacity).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_TextureOverlay::doRunFilter(ptImage* AImage) {
  if (FOverlayImage.isNull()) {
    return; // no overlay image loaded: nothing to do
  }

  AImage->toRGB();

  ptImage tempOverlayImage;
  tempOverlayImage.Set(&FOverlayImage);
  tempOverlayImage.ptGMResizeWH(AImage->m_Width, AImage->m_Height, ptIMFilter_Catrom);

  // adjust saturation of overlay image
  auto value = (FConfig.value(CSaturation).toFloat() - 1.0f) * 100.0f;
  TChannelMatrix vibranceMixer;

  vibranceMixer[0][0] = 1.0f + value/150.0f;
  vibranceMixer[0][1] = -(value/300.0f);
  vibranceMixer[0][2] = vibranceMixer[0][1];
  vibranceMixer[1][0] = vibranceMixer[0][1];
  vibranceMixer[1][1] = vibranceMixer[0][0];
  vibranceMixer[1][2] = vibranceMixer[0][1];
  vibranceMixer[2][0] = vibranceMixer[0][1];
  vibranceMixer[2][1] = vibranceMixer[0][1];
  vibranceMixer[2][2] = vibranceMixer[0][0];

  tempOverlayImage.mixChannels(vibranceMixer);

  // create vignette if needed
  auto maskMode = static_cast<TTextureOverlayMaskMode>(FConfig.value(CMaskMode).toInt());
  pt::c_unique_ptr<float> vignetteMask;

  if (maskMode != TTextureOverlayMaskMode::Full) {
    vignetteMask =
        tempOverlayImage.GetVignetteMask(
            maskMode == TTextureOverlayMaskMode::InvVignette,
            static_cast<TVignetteShape>(FConfig.value(CVigShape).toInt()),
            FConfig.value(CVigInnerRadius).toDouble(),
            FConfig.value(CVigOuterRadius).toDouble(),
            FConfig.value(CVigRoundness).toDouble(),
            FConfig.value(CVigCenterX).toDouble(),
            FConfig.value(CVigCenterY).toDouble(),
            FConfig.value(CVigSoftness).toDouble() );
  }

  // finally perform the actual overlay
  AImage->Overlay(
      tempOverlayImage.m_Image,
      FConfig.value(COpacity).toDouble(),
      vignetteMask.get(),
        static_cast<TOverlayMode>(FConfig.value(COverlayMode).toInt()));
}

// -----------------------------------------------------------------------------

void ptFilter_TextureOverlay::doUpdateGui() {
  FForm.ImageFilePathEdit->setText(QFileInfo(FConfig.value(CImageFilePath).toString()).completeBaseName());
}

// -----------------------------------------------------------------------------

bool ptFilter_TextureOverlay::loadOverlayImage(const QString& AFilePath) {
  bool success = false;
  FOverlayImage.clear();

  FOverlayImage.ptGMCOpenImage(
      AFilePath.toLocal8Bit().data(),
      Settings->GetInt("WorkColor"),
      Settings->GetInt("PreviewColorProfileIntent"),
      0,
      false,
      nullptr,
      success);

  if (!success) {
    ptMessageBox::critical(
        nullptr,
        "File not found",
        "Please open a valid image for texture overlay.");
  }

  return success;
}

//------------------------------------------------------------------------------

QWidget* ptFilter_TextureOverlay::doCreateGui() {
  auto guiBody = new QWidget;

  FForm.setupUi(guiBody);
  this->initDesignerGui(guiBody);
  FForm.VignetteGroup->setEnabled(false);

  connect(FForm.VigInnerRadius,
          SIGNAL(valueChanged(QString,QVariant)),
          SLOT(onInnerRadiusChanged(QString,QVariant)));
  connect(FForm.VigOuterRadius,
          SIGNAL(valueChanged(QString,QVariant)),
          SLOT(onOuterRadiusChanged(QString,QVariant)));
  connect(FForm.MaskMode,
          SIGNAL(valueChanged(QString,QVariant)),
          SLOT(onMaskModeChanged(QString,QVariant)));
  connect(FForm.LoadImageButton,
          SIGNAL(clicked()),
          SLOT(onLoadImageButtonClicked()));

  return guiBody;
}

// -----------------------------------------------------------------------------

void ptFilter_TextureOverlay::onMaskModeChanged(const QString AId, const QVariant ANewValue) {
  FConfig.setValue(AId, ANewValue);
  FForm.VignetteGroup->setEnabled(
      static_cast<TTextureOverlayMaskMode>(ANewValue.toInt()) != TTextureOverlayMaskMode::Full);
  this->requestPipeRun();
}

// -----------------------------------------------------------------------------

void ptFilter_TextureOverlay::onInnerRadiusChanged(const QString AId, const QVariant ANewValue) {
  QVariant radius = qMin(ANewValue.toDouble(), FConfig.value(CVigOuterRadius).toDouble());

  FConfig.setValue(AId, radius);
  FForm.VigInnerRadius->setValue(radius);

  this->requestPipeRun();
}

//------------------------------------------------------------------------------

void ptFilter_TextureOverlay::onOuterRadiusChanged(const QString AId, const QVariant ANewValue) {
  QVariant radius = qMax(ANewValue.toDouble(), FConfig.value(CVigInnerRadius).toDouble());

  FConfig.setValue(AId, radius);
  FForm.VigOuterRadius->setValue(radius);

  this->requestPipeRun();
}

// -----------------------------------------------------------------------------

void ptFilter_TextureOverlay::onLoadImageButtonClicked() {
  QString overlayFilePath = FConfig.value(CImageFilePath).toString();
  QString textureDir;

  if (overlayFilePath.isEmpty()) {
    textureDir = Settings->GetString("UserDirectory");
  } else {
    QFileInfo(overlayFilePath).absolutePath();
  }

  overlayFilePath = QFileDialog::getOpenFileName(
      nullptr,
      tr("Load overlay image"),
      textureDir,
      BitmapPattern);

  if (overlayFilePath.isEmpty()) {
    return;
  }

  QFileInfo ovInfo(overlayFilePath);
  overlayFilePath = ovInfo.absoluteFilePath();

  if (loadOverlayImage(overlayFilePath)) {
    FConfig.setValue(CImageFilePath, overlayFilePath);
    FForm.ImageFilePathEdit->setText(ovInfo.completeBaseName());
    this->requestPipeRun();
  }
}

//------------------------------------------------------------------------------

RegisterHelper TextureOverlayRegister(&ptFilter_TextureOverlay::createTextureOverlay, CTextureOverlayId);

//------------------------------------------------------------------------------
