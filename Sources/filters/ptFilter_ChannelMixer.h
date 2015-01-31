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

#ifndef PTFILTER_ChannelMixer_H
#define PTFILTER_ChannelMixer_H

#include "ptFilterBase.h"
#include "../ptDefines.h"

class ptFilter_ChannelMixer: public ptFilterBase {
  Q_OBJECT

public:
  static ptFilterBase *createChannelMixer();

protected:
  QWidget  *doCreateGui();

  void      doDefineControls() override;
  bool      doCheckHasActiveCfg() override;
  void      doRunFilter(ptImage *AImage) override;

private:
  class ptStorableLabel: public ptStorable {
  public:
    void setAssocLabel(QLabel* AAssocLabel) { FAssocLabel = AAssocLabel; }
    QString text() const { return FAssocLabel->text(); }
    void setText(const QString& AText) { FAssocLabel->setText(AText); }
  protected:
    TConfigStore doStoreConfig(const QString &APrefix) const override;
    void doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) override;
  private:
    QLabel* FAssocLabel = nullptr;
  };

  ptFilter_ChannelMixer();
  TChannelMatrix configToMatrix() const;
  void loadFromFile(const QString& AFilePath);
  void saveToFile(const QString& AFilePath, const QString& ADescription);

  ptStorableLabel FMixerNameStore;

private slots:
  void onValueChanged(QString ACfgItemId, QVariant ANewValue);
  void onLoadButtonClicked();
  void onSaveButtonClicked();
  
};

#endif // PTFILTER_ChannelMixer_H
