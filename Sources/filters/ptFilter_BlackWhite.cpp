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

#include "ptFilter_BlackWhite.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CBlackWhiteId = "BlackWhite";

const QString COpacity         = "Opacity";
const QString CFilmType        = "FilmType";
const QString CColorFilterType = "ColorFilterType";
const QString CMultRed         = "MultRed";
const QString CMultGreen       = "MultGreen";
const QString CMultBlue        = "MultBlue";

//------------------------------------------------------------------------------

ptFilter_BlackWhite::ptFilter_BlackWhite():
  ptFilterBase()
{
  FHelpUri = "http://photivo.org/manual/tabs/eyecandy#black_and_white";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase* ptFilter_BlackWhite::createBlackWhite() {
  auto instance         = new ptFilter_BlackWhite;
  instance->FFilterName = CBlackWhiteId;
  instance->FCaption    = tr("Black and White");
  return instance;
}

//------------------------------------------------------------------------------

void ptFilter_BlackWhite::doDefineControls() {
  const ptCfgItem::TComboEntryList filmTypes({
    {tr("Low sensitivity"),   static_cast<int>(TBWFilmType::LowSensitivity),    "LowSensitivity"},
    {tr("High sensitivity"),  static_cast<int>(TBWFilmType::HighSensitivity),   "HighSensitivity"},
    {tr("Hyperpanchromatic"), static_cast<int>(TBWFilmType::Hyperpanchromatic), "Hyperpanchromatic"},
    {tr("Orthochromatic"),    static_cast<int>(TBWFilmType::Orthochromatic),    "Orthochromatic"},
    {tr("Normal contrast"),   static_cast<int>(TBWFilmType::NormalContrast),    "NormalContrast"},
    {tr("High contrast"),     static_cast<int>(TBWFilmType::HighContrast),      "HighContrast"},
    {tr("Luminance"),         static_cast<int>(TBWFilmType::Luminance),         "Luminance"},
    {tr("Landscape"),         static_cast<int>(TBWFilmType::Landscape),         "Landscape"},
    {tr("Face in interior"),  static_cast<int>(TBWFilmType::FaceInterior),      "FaceInterior"},
    {tr("Channel mixer"),     static_cast<int>(TBWFilmType::ChannelMixer),      "ChannelMixer"},
  });

  const ptCfgItem::TComboEntryList filterTypes({
    {tr("None"),    static_cast<int>(TBWColorFilter::None),   "None"},
    {tr("Red"),     static_cast<int>(TBWColorFilter::Red),    "Red"},
    {tr("Orange"),  static_cast<int>(TBWColorFilter::Orange), "Orange"},
    {tr("Yellow"),  static_cast<int>(TBWColorFilter::Yellow), "Yellow"},
    {tr("Lime"),    static_cast<int>(TBWColorFilter::Lime),   "Lime"},
    {tr("Green"),   static_cast<int>(TBWColorFilter::Green),  "Green"},
    {tr("Blue"),    static_cast<int>(TBWColorFilter::Blue),   "Blue"},
    {tr("Fake IR"), static_cast<int>(TBWColorFilter::FakeIR), "FakeIR"},
  });

  FConfig.initStores(TCfgItemList()                                        //--- Combo: list of entries               ---//
    //            Id                 Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({COpacity,          ptCfgItem::Slider,        0.0,        0.0,          1.0,          0.05,       2,        true,  true, tr("Opacity"), tr("")})
    << ptCfgItem({CFilmType,         ptCfgItem::Combo,         static_cast<int>(TBWFilmType::Luminance), filmTypes,          false, true, tr("Film type"), tr("type of analog film to emulate")})
    << ptCfgItem({CColorFilterType,  ptCfgItem::Combo,         static_cast<int>(TBWColorFilter::None), filterTypes,          true,  true, tr("Color filter"), tr("type of color filter to emulate")})
    << ptCfgItem({CMultRed,          ptCfgItem::Slider,        0.5,       -1.0,          1.0,          0.1,        2,        true,  true, tr("Red multiplier"), tr("")})
    << ptCfgItem({CMultGreen,        ptCfgItem::Slider,        0.5,       -1.0,          1.0,          0.1,        2,        true,  true, tr("Green multiplier"), tr("")})
    << ptCfgItem({CMultBlue,         ptCfgItem::Slider,        0.5,       -1.0,          1.0,          0.1,        2,        true,  true, tr("Blue multiplier"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_BlackWhite::doCheckHasActiveCfg() {
  return !qFuzzyIsNull(FConfig.value(COpacity).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_BlackWhite::doRunFilter(ptImage* AImage) {
  AImage->toRGB();
  AImage->BWStyler(
      static_cast<TBWFilmType>(FConfig.value(CFilmType).toInt()),
      static_cast<TBWColorFilter>(FConfig.value(CColorFilterType).toInt()),
      FConfig.value(CMultRed).toDouble(),
      FConfig.value(CMultGreen).toDouble(),
      FConfig.value(CMultBlue).toDouble(),
      FConfig.value(COpacity).toDouble());
}

//------------------------------------------------------------------------------

QWidget* ptFilter_BlackWhite::doCreateGui() {
  auto guiBody = new QWidget;

  FForm.setupUi(guiBody);
  this->initDesignerGui(guiBody);

  connect(
      FForm.FilmType,
      SIGNAL(valueChanged(QString,QVariant)),
      SLOT(onFilmTypeChanged(QString,QVariant)));

  return guiBody;
}

//------------------------------------------------------------------------------

void ptFilter_BlackWhite::doUpdateGui() {
  bool isChannelMixer =
      static_cast<TBWFilmType>(FConfig.value(CFilmType).toInt()) == TBWFilmType::ChannelMixer;

  FForm.MultRed->setEnabled(isChannelMixer);
  FForm.MultGreen->setEnabled(isChannelMixer);
  FForm.MultBlue->setEnabled(isChannelMixer);
}

// -----------------------------------------------------------------------------

void ptFilter_BlackWhite::onFilmTypeChanged(QString AId, QVariant ANewValue) {
  FConfig.setValue(AId, ANewValue);
  this->doUpdateGui();
  this->requestPipeRun();
}

// -----------------------------------------------------------------------------

RegisterHelper BlackWhiteRegister(&ptFilter_BlackWhite::createBlackWhite, CBlackWhiteId);

//------------------------------------------------------------------------------
