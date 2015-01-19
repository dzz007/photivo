/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008-2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009 Michael Munzert <mail@mm-log.com>
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

#include "ptFilter_ChannelMixer.h"
#include "ui_ptFilter_ChannelMixer.h"
#include "ptCfgItem.h"
#include "ptStorable.h"
#include "../ptDefines.h"
#include "../ptImage.h"
#include "../ptMainWindow.h"
#include "../ptMessageBox.h"
#include "../ptSettings.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <stdexcept>

extern QString ChannelMixerFilePattern;
extern ptMainWindow* MainWindow;

// -----------------------------------------------------------------------------

const QString CChannelMixerId = "ChannelMixer";

const QString CRed2Red     = "Red2Red";
const QString CRed2Green   = "Red2Green";
const QString CRed2Blue    = "Red2Blue";
const QString CGreen2Red   = "Green2Red";
const QString CGreen2Green = "Green2Green";
const QString CGreen2Blue  = "Green2Blue";
const QString CBlue2Red    = "Blue2Red";
const QString CBlue2Green  = "Blue2Green";
const QString CBlue2Blue   = "Blue2Blue";
const QString CMixerName   = "MixerName";

//------------------------------------------------------------------------------

ptFilter_ChannelMixer::ptFilter_ChannelMixer():
  ptFilterBase()
{
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_ChannelMixer::createChannelMixer() {
  auto hInstance         = new ptFilter_ChannelMixer;
  hInstance->FFilterName = CChannelMixerId;
  hInstance->FCaption    = tr("Channel mixer");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_ChannelMixer::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id                       Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CRed2Red,                ptCfgItem::SpinEdit,      1.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Red input to Red output ")})
    << ptCfgItem({CRed2Green,              ptCfgItem::SpinEdit,      0.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Red input to Green output ")})
    << ptCfgItem({CRed2Blue,               ptCfgItem::SpinEdit,      0.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Red input to Blue output ")})
    << ptCfgItem({CGreen2Red,              ptCfgItem::SpinEdit,      0.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Green input to Red output ")})
    << ptCfgItem({CGreen2Green,            ptCfgItem::SpinEdit,      1.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Green input to Green output ")})
    << ptCfgItem({CGreen2Blue,             ptCfgItem::SpinEdit,      0.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Green input to Blue output ")})
    << ptCfgItem({CBlue2Red,               ptCfgItem::SpinEdit,      0.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Blue input to Red output ")})
    << ptCfgItem({CBlue2Green,             ptCfgItem::SpinEdit,      0.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Blue input to Green output ")})
    << ptCfgItem({CBlue2Blue,              ptCfgItem::SpinEdit,      1.0,       -2.0,          2.0,          0.01,       2,        false, true, tr(""), tr("contribution of Blue input to Blue output ")})
    << ptCfgItem(ptCfgItem::TCustom{CMixerName, ptCfgItem::CustomType, &FMixerNameStore})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_ChannelMixer::doCheckHasActiveCfg() {
  return
      !qFuzzyCompare(FConfig.value(CRed2Red).toFloat(), 1.0f) ||
      !qFuzzyCompare(FConfig.value(CGreen2Green).toFloat(), 1.0f) ||
      !qFuzzyCompare(FConfig.value(CBlue2Blue).toFloat(), 1.0f) ||
      !qFuzzyIsNull(FConfig.value(CRed2Green).toFloat()) ||
      !qFuzzyIsNull(FConfig.value(CRed2Blue).toFloat()) ||
      !qFuzzyIsNull(FConfig.value(CGreen2Red).toFloat()) ||
      !qFuzzyIsNull(FConfig.value(CGreen2Blue).toFloat()) ||
      !qFuzzyIsNull(FConfig.value(CBlue2Red).toFloat()) ||
      !qFuzzyIsNull(FConfig.value(CBlue2Green).toFloat());
}

//------------------------------------------------------------------------------

void ptFilter_ChannelMixer::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();
  AImage->mixChannels(this->configToMatrix());
}

// -----------------------------------------------------------------------------

TChannelMatrix ptFilter_ChannelMixer::configToMatrix() const {
  return TChannelMatrix {{
    {FConfig.value(CRed2Red).toFloat(),   FConfig.value(CGreen2Red).toFloat(),   FConfig.value(CBlue2Red).toFloat()},
    {FConfig.value(CRed2Green).toFloat(), FConfig.value(CGreen2Green).toFloat(), FConfig.value(CBlue2Green).toFloat()},
    {FConfig.value(CRed2Blue).toFloat(),  FConfig.value(CGreen2Blue).toFloat(),  FConfig.value(CBlue2Blue).toFloat()}
  }};
}

//------------------------------------------------------------------------------

QWidget *ptFilter_ChannelMixer::doCreateGui() {
  auto guiBody = new QWidget;
  Ui_ChannelMixerForm form;

  form.setupUi(guiBody);
  this->initDesignerGui(guiBody);
  FMixerNameStore.setAssocLabel(form.MixernameLabel);

  form.ChannelMatrixGrid->setVerticalSpacing(6);
  form.ChannelMatrixGrid->setHorizontalSpacing(6);

  connect(form.LoadButton, SIGNAL(clicked()), SLOT(onLoadButtonClicked()));
  connect(form.SaveButton, SIGNAL(clicked()), SLOT(onSaveButtonClicked()));

  connect(form.Red2Red,     SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Red2Green,   SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Red2Blue,    SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Green2Red,   SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Green2Green, SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Green2Blue,  SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Blue2Red,    SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Blue2Green,  SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));
  connect(form.Blue2Blue,   SIGNAL(valueChanged(QString,QVariant)), SLOT(onValueChanged(QString,QVariant)));

  return guiBody;
}

// -----------------------------------------------------------------------------

void ptFilter_ChannelMixer::onValueChanged(QString ACfgItemId, QVariant ANewValue) {
  if (FConfig.value(ACfgItemId) != ANewValue) {
    FMixerNameStore.setText(QString());
    FConfig.setValue(ACfgItemId, ANewValue);
    this->requestPipeRun();
  }
}

// -----------------------------------------------------------------------------

void ptFilter_ChannelMixer::onLoadButtonClicked() {
  QString filename = QFileDialog::getOpenFileName(
      nullptr,
      tr("Load channel mixer configuration"),
      Settings->GetString("ChannelMixersDirectory"),
      ChannelMixerFilePattern);

  if (filename.isEmpty()) {
    return;
  }

  try {
    this->loadFromFile(filename);
  } catch (const std::runtime_error& e) {
    ptMessageBox::warning(MainWindow, tr("Error loading channel mixer configuration"), e.what());
    return;
  }

  FMixerNameStore.setText(QFileInfo(filename).baseName());
  this->updateGui(/*ARequestPiperun=*/true);
}

// -----------------------------------------------------------------------------

void ptFilter_ChannelMixer::onSaveButtonClicked() {
  QString filename = QFileDialog::getSaveFileName(
      nullptr,
      tr("Save channel mixer configuration"),
      Settings->GetString("ChannelMixersDirectory"),
      ChannelMixerFilePattern);

  if (filename.isEmpty()) {
    return;
  }

  bool success;
  QString descr = QInputDialog::getText(
      nullptr,
      tr("Save Channelmixer"),
      tr("Give a description"),
      QLineEdit::Normal,
      QString(),
      &success);

  if (success && !descr.isEmpty()) {
    descr = "; " + descr;
  }

  try {
    this->saveToFile(filename, descr);
  } catch (const std::runtime_error& e) {
    ptMessageBox::warning(MainWindow, tr("Error saving channel mixer configuration"), e.what());
    return;
  }
}

// -----------------------------------------------------------------------------

// Copied almost verbatim from the old ptChannelMixer. Needs to be rewritten at some point.
// A good time might be once the Filesystem library TS is finished.
void ptFilter_ChannelMixer::loadFromFile(const QString &AFilePath) {
  std::unique_ptr<FILE, void(*)(FILE*)> inputFile(
      fopen(AFilePath.toLocal8Bit().data(), "r"), [](FILE* f) { fclose(f); });

  if (!inputFile) {
    throw std::runtime_error(tr("Could not open %1.").arg(AFilePath).toLocal8Bit().data());
  }

  char Buffer[100];
  char Key[100];
  char Value[100];
  int  LineNr = 0;
  int  NrKeys = 0;
  TChannelMatrix matrix {{}};
  const QString filename = QFileInfo(AFilePath).fileName();

  do {
    if (nullptr == fgets(Buffer, 100, inputFile.get())) {
      break;
    }

    LineNr++;
    if (';' == Buffer[0]) {
      continue;
    }

    sscanf(Buffer,"%s %s",Key,Value);
    NrKeys++;

    if (1 == NrKeys) {
      if ((strcmp(Key,"Magic") || strcmp(Value,"photivoChannelMixerFile")) &&
          (strcmp(Key,"Magic") || strcmp(Value,"dlRawChannelMixerFile")))
      {
        throw std::runtime_error(
            tr("%1 has wrong format at line %2.").arg(filename).arg(LineNr).toLocal8Bit().data());
      }
    } else {
      int    Value1 = strtol(Key, nullptr, 10);
      double Value2 = atof(Value);

      if ( Value1<0 || Value1 >8) {
        throw std::runtime_error(
            tr("Error reading %1 at line %2 (out of range: %3)")
                .arg(filename).arg(LineNr).arg(Value1).toLocal8Bit().data());
      }

      // Expressed in 1/1000ths to avoid ',','.' related issues.
      matrix[Value1/3][Value1%3] = Value2/1000;
    }
  } while (!feof(inputFile.get()));

  FConfig.setValue(CRed2Red,     matrix[0][0]);
  FConfig.setValue(CGreen2Red,   matrix[0][1]);
  FConfig.setValue(CBlue2Red,    matrix[0][2]);
  FConfig.setValue(CRed2Green,   matrix[1][0]);
  FConfig.setValue(CGreen2Green, matrix[1][1]);
  FConfig.setValue(CBlue2Green,  matrix[1][2]);
  FConfig.setValue(CRed2Blue,    matrix[2][0]);
  FConfig.setValue(CGreen2Blue,  matrix[2][1]);
  FConfig.setValue(CBlue2Blue,   matrix[2][2]);
}

// -----------------------------------------------------------------------------

// Copied in large parts from the old ptChannelMixer. Needs to be rewritten at some point.
// A good time might be once the Filesystem library TS is finished.
void ptFilter_ChannelMixer::saveToFile(const QString &AFilePath, const QString &ADescription) {
  TChannelMatrix matrix = this->configToMatrix();

  std::unique_ptr<FILE, void(*)(FILE*)> outputFile(
      fopen(AFilePath.toLocal8Bit().data(), "wb"), [](FILE* f) { fclose(f); });

  if (!outputFile) {
    throw std::runtime_error(tr("Could not create %1.").arg(AFilePath).toLocal8Bit().data());
  }

  if (!ADescription.isEmpty()) {
    fprintf(outputFile.get(), "%s\n", ADescription.toLocal8Bit().data());
  }

  fprintf(outputFile.get(),"Magic photivoChannelMixerFile\n");

  for (short To=0; To<3; To++) {
    for (short From=0; From<3; From++) {
      fprintf(
          outputFile.get(),
          "%d %d\n",
          From+To*3,
          // Expressed in 1/1000ths to avoid ',','.' related issues.
          static_cast<int>(1000*matrix[To][From]));
    }
  }
}

//------------------------------------------------------------------------------

RegisterHelper ChannelMixerRegister(&ptFilter_ChannelMixer::createChannelMixer, CChannelMixerId);

//------------------------------------------------------------------------------

TConfigStore ptFilter_ChannelMixer::ptStorableLabel::doStoreConfig(const QString &APrefix) const {
  TConfigStore store;
  if (FAssocLabel) {
    store.insert(APrefix+CMixerName, FAssocLabel->text());
  }
  return store;
}

// -----------------------------------------------------------------------------

void ptFilter_ChannelMixer::ptStorableLabel::doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) {
  FAssocLabel->setText(AConfig.value(APrefix+CMixerName).toString());
}
