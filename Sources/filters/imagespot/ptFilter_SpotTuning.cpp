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

#include "ptFilter_SpotTuning.h"
#include "ptTuningSpot.h"
#include "../ptCfgItem.h"
#include "../../ptImage.h"
#include "../../ptCurve.h"
#include "../../ptProcessor.h"
#include "../../ptSettings.h"
#include "../../ptViewWindow.h"
#include "../../ptUtils.h"
#include <functional>

extern ptProcessor  *TheProcessor;
extern ptViewWindow *ViewWindow;
extern short NextPhase;

void ReportProgress(const QString Message);
void BlockTools(const ptBlockToolsMode ANewState, QStringList AExcludeIds = QStringList());
void Update(short Phase,
            short SubPhase       = -1,
            short WithIdentify   = 1,
            short ProcessorMode  = ptProcessorMode_Preview);
void UpdatePreviewImage(const ptImage* ForcedImage   = NULL,
                        const short    OnlyHistogram = 0);

//==============================================================================

const QString CSpotTuningId       = "SpotTuning";
const QString CSpotListId         = "SpotList";
// All other id constants are included from ptTuningSpot.h

//==============================================================================

ptFilter_SpotTuning::ptFilter_SpotTuning()
: ptFilterBase(),
  FInteractionOngoing(false),
  FSpotList(ptImageSpotList(std::bind(&ptFilter_SpotTuning::createSpot, this)))
{
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_SpotTuning::CreateSpotTuning() {
  auto hInstance         = new ptFilter_SpotTuning;
  hInstance->FFilterName = CSpotTuningId;
  hInstance->FCaption    = tr("Spot tuning");
  return hInstance;
}

//==============================================================================

void ptFilter_SpotTuning::doDefineControls() {
  // Calling ptCfgItem’s TCustom ctor as usual with a braced initializer results in an
  // “ambiguous call” compiler error. No idea why, I don’t see any ambiguity anywhere.
  // If anyone figures it out please enlighten me. (Brother John)
  const ptCfgItem::TCustom hSpotsCfgItem = {CSpotListId, ptCfgItem::CustomType, &FSpotList};

  auto hCfgItems = TCfgItemList()                                        //--- Combo: list of entries               ---//
                                                                         //--- Check: not available                 ---//
    //            Id                     Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem(hSpotsCfgItem)
    << ptCfgItem({CSpotHasMaxRadiusId,   ptCfgItem::Check,         false,                                                        false, false, tr("Use maximum radius"), tr("")})
    << ptCfgItem({CSpotMaxRadiusId,      ptCfgItem::Slider,        500,        1,            7000,         10,         0,        false, false, tr("Maximum radius"), tr("Pixels outside this radius will never be included in the mask.")})
    << ptCfgItem({CSpotChromaWeightId,   ptCfgItem::Slider,        0.5,        0.0,          1.0,          0.1,        2,        false, false, tr("Brightness/color ratio"), tr("Defines how brightness and color affect the threshold.\n0.0: ignore color, 1.0: ignore brightness, 0.5: equal weight for both")})
    << ptCfgItem({CSpotThresholdId,      ptCfgItem::Slider,        0.25,       0.0,          1.0,          0.05,       3,        false, false, tr("Threshold"), tr("Maximum amount a pixel may differ from the spot's source pixel to get included in the mask.")})
    << ptCfgItem({CSpotSaturationId,     ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.05,       2,        false, false, tr("Saturation"), tr("")})
    << ptCfgItem({CSpotIsAdaptiveSatId,  ptCfgItem::Check,         false,                                                        false, false, tr("Adaptive saturation"), tr("Prevent clipping when adjusting saturation")})
    << ptCfgItem({CSpotColorShiftId,     ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.001,      3,        false, false, tr("Color shift"), tr("")})
  ;
  FConfig.initStores(hCfgItems);

  // Create the null curve that gets loaded into the curve window whenever no spots are
  // present or none is selected.
  TAnchorList hNullAnchors = { TAnchor(0.0, 0.0), TAnchor(0.4, 0.6), TAnchor(1.0, 1.0) };
  FNullSpot = make_unique<ptTuningSpot>(&FConfig.items(), ptCurve(hNullAnchors,
                                                                  ptCurve::LumaMask,
                                                                  ptCurve::LumaMask,
                                                                  ptCurve::SplineInterpol));
}

//==============================================================================

void ptFilter_SpotTuning::connectWidgets(QWidget *AGuiWidget) {
  for (const auto& hCfgItem: FConfig.items()) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType) {
      auto hWidget = AGuiWidget->findChild<QWidget*>(hCfgItem.Id);
      if (hWidget) {
        connect(hWidget, SIGNAL(valueChanged(QString,QVariant)),
                this,    SLOT(spotDispatch(QString,QVariant)));
      }
    }
  }
}

//==============================================================================

QWidget *ptFilter_SpotTuning::doCreateGui() {
  auto hGuiBody = new QWidget;
  FGui = make_unique<Ui::Form>();

  FGui->setupUi(hGuiBody);
  this->initDesignerGui(hGuiBody);

  FGui->CurveWin->init(ptCfgItem({CSpotLumaCurveId,
                                  ptCfgItem::CurveWin,
                                  FNullSpot->curve(),
                                  tr("Luminance curve")}));
  FGui->SpotList->init(&FSpotList, std::bind(&ptFilter_SpotTuning::createSpot, this));

  connect(FGui->SpotList, SIGNAL(rowChanged(int)),                SLOT(updateSpotDetailsGui(int)));
  connect(FGui->SpotList, SIGNAL(dataChanged()),                  SLOT(updatePreview()));
  connect(FGui->SpotList, SIGNAL(editModeChanged(bool)),          SLOT(setupInteraction(bool)));
  connect(FGui->CurveWin, SIGNAL(valueChanged(QString,QVariant)), SLOT(spotDispatch(QString,QVariant)));
  this->connectWidgets(hGuiBody);
  this->updateSpotDetailsGui(-1, hGuiBody);

  return hGuiBody;
}

//------------------------------------------------------------------------------

bool ptFilter_SpotTuning::doCheckHasActiveCfg() {
  return FSpotList.hasEnabledSpots();
}

//------------------------------------------------------------------------------

void ptFilter_SpotTuning::doImportCustomConfig(QSettings*) {
  if (FGuiContainer) {
    FGui->SpotList->model()->rebuildModel();
    this->updateSpotDetailsGui(-1);
  }
}

//------------------------------------------------------------------------------

void ptFilter_SpotTuning::doRunFilter(ptImage *AImage) {
  AImage->RGBToLch();
  for (int i = 0; i < FSpotList.count(); ++i) {
    ptTuningSpot *hTSpot = static_cast<ptTuningSpot*>(FSpotList.at(i));
    if (hTSpot->isEnabled()) {
      AImage->MaskedColorAdjust(hTSpot->x(),
                                hTSpot->y(),
                                hTSpot->value(CSpotThresholdId).toFloat(),
                                hTSpot->value(CSpotChromaWeightId).toFloat(),
                                hTSpot->value(CSpotMaxRadiusId).toInt(),
                                hTSpot->value(CSpotHasMaxRadiusId).toBool(),
                                hTSpot->curvePtr(),
                                hTSpot->value(CSpotIsAdaptiveSatId).toBool(),
                                hTSpot->value(CSpotSaturationId).toFloat(),
                                hTSpot->value(CSpotColorShiftId).toFloat());
    }
  }
}

//------------------------------------------------------------------------------

void ptFilter_SpotTuning::doReset() {
  FSpotList.clear();
  if (FGuiContainer) {
    FGui->SpotList->clear();
    updateSpotDetailsGui(-1);
  }
}

//==============================================================================

ptImageSpot *ptFilter_SpotTuning::createSpot() {
  return new ptTuningSpot(&FConfig.items(), *(FNullSpot->curvePtr()));
}

//==============================================================================

void ptFilter_SpotTuning::startInteraction() {
  if (!ViewWindow)
    return;

  FGui->SpotList->setEditMode(true);
  ViewWindow->ShowStatus(QObject::tr("Prepare"));
  ReportProgress(QObject::tr("Prepare for local adjust"));
  TheProcessor->RunLocalEdit(ptProcessorStopBefore::SpotTuning);
  this->updatePreview();

  // Allow to be selected in the view window. And deactivate main.
  ViewWindow->ShowStatus(QObject::tr("Local adjust"));
  ReportProgress(QObject::tr("Local adjust"));
  BlockTools(btmBlockForSpotTuning, QStringList(this->uniqueName()));

  ViewWindow->StartLocalAdjust(std::bind(&ptFilter_SpotTuning::cleanupAfterInteraction, this));
  QObject::connect(ViewWindow->spotTuning(), SIGNAL(clicked(QPoint)),
                   FGui->SpotList, SLOT(processCoordinates(QPoint)));
  ViewWindow->setFocus();
}

//==============================================================================

void ptFilter_SpotTuning::cleanupAfterInteraction() {
  BlockTools(btmUnblock, QStringList(this->uniqueName()));
  FGui->SpotList->setEditMode(false);

  if (Settings->GetInt("RunMode") == ptRunMode_Once) {
    UpdatePreviewImage();
    NextPhase = ptProcessorPhase_LocalEdit;
  } else {
    Update(ptProcessorPhase_LocalEdit);
  }
}

//==============================================================================

void ptFilter_SpotTuning::updateSpotDetailsGui(int ASpotIdx, QWidget *AGuiWidget /*=nullptr*/) {
  ptTuningSpot *hSpot = nullptr;
  if (isBetween(0, ASpotIdx, FSpotList.count()-1)) {
    hSpot = static_cast<ptTuningSpot*>(FSpotList.at(ASpotIdx));
    FGui->SpotDetailsGroup->setEnabled(true);
  } else {
    hSpot = FNullSpot.get();
    FGui->SpotDetailsGroup->setEnabled(false);
  }

  if (AGuiWidget == nullptr)
    AGuiWidget = FGuiContainer;

  FGui->CurveWin->updateView(hSpot->curve());

  for (const ptCfgItem &hCfgItem: FConfig.items()) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType) {
      ptWidget *hWidget = this->findPtWidget(hCfgItem.Id, AGuiWidget);
      hWidget->blockSignals(true);
      hWidget->setValue(hSpot->value(hCfgItem.Id));
      hWidget->blockSignals(false);
    }
  }

  FGui->MaxRadius->setEnabled(hSpot->value("HasMaxRadius").toBool());
}

//==============================================================================

// Updates the preview image in the ViewWindow and takes into account if the ViewWindow
// interaction is running or not.
void ptFilter_SpotTuning::updatePreview() {
  this->checkActiveChanged(true);
  if (FInteractionOngoing) {
    // We’re in interactive mode: only recalc spots
    auto hImage = make_unique<ptImage>();
    hImage->Set(TheProcessor->m_Image_AfterLocalEdit);
    this->runFilter(hImage.get());
    hImage->LchToRGB(Settings->GetInt("WorkColor"));
    UpdatePreviewImage(hImage.get());

  } else {
    // not in interactive mode: run pipe
    Update(ptProcessorPhase_LocalEdit);
  }
}

//==============================================================================

void ptFilter_SpotTuning::setupInteraction(bool AEnable) {
  if (AEnable == FInteractionOngoing)
    return;

  FInteractionOngoing = AEnable;
  if (AEnable) {
    startInteraction();
  } else {
    ViewWindow->spotTuning()->stop();
  }
}

//==============================================================================

void ptFilter_SpotTuning::spotDispatch(const QString AId, const QVariant AValue) {
  if (!AValue.isValid()) return;

  auto hListIdx = FGui->SpotList->currentIndex();
  if (hListIdx < 0) return;

  auto hSpot = static_cast<ptTuningSpot*>(FSpotList.at(hListIdx));
  hSpot->setValue(AId, AValue);

  if (AId == CSpotHasMaxRadiusId)
    FGui->MaxRadius->setEnabled(hSpot->value("HasMaxRadius").toBool());

  this->updatePreview();
}

//------------------------------------------------------------------------------
// The current spot’s ptCurve object is already updated to its new values as is the
// curve window’s display. So this slot only needs to trigger a preview image update.
void ptFilter_SpotTuning::curveDispatch(const QString, const QVariant) {
  this->updatePreview();
}

//==============================================================================

RegisterHelper SpotTuningRegister(&ptFilter_SpotTuning::CreateSpotTuning, CSpotTuningId);

//==============================================================================
