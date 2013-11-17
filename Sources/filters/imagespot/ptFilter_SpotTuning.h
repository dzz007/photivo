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

#ifndef PTFILTER_SpotTuning_H
#define PTFILTER_SpotTuning_H

//==============================================================================

#include "ui_ptFilter_SpotTuning.h"
#include "ptImageSpotList.h"
#include "ptTuningSpot.h"
#include "../ptFilterBase.h"
#include <memory>

//==============================================================================

class ptFilter_SpotTuning: public ptFilterBase {
Q_OBJECT

public:
  static ptFilterBase *CreateSpotTuning();

protected:
  void      doDefineControls();
  QWidget  *doCreateGui();
  bool      doCheckHasActiveCfg();
  void      doImportCustomConfig(QSettings*);
  void      doRunFilter(ptImage *AImage) const;
  void      doReset();

private:
  ptFilter_SpotTuning();

  ptImageSpot  *createSpot();
  void          connectWidgets(QWidget *AGuiWidget);
  void          startInteraction();
  void          cleanupAfterInteraction();

  std::unique_ptr<ptTuningSpot> FNullSpot;
  std::unique_ptr<Ui::Form>     FGui;
  bool                          FInteractionOngoing;
  ptImageSpotList               FSpotList;

private slots:
  void updateSpotDetailsGui(int ASpotIdx, QWidget *AGuiWidget = nullptr);
  void updatePreview();
  void setupInteraction(bool AEnable);
  void spotDispatch(const QString AId, const QVariant AValue);
  void curveDispatch(const QString, const QVariant);
};

#endif // PTFILTER_SpotTuning_H
