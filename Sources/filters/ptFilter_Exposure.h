/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012-2015 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTFILTER_Exposure_H
#define PTFILTER_Exposure_H

#include "ptFilterBase.h"
#include "ui_ptFilter_Exposure.h"

//==============================================================================

class ptFilter_Exposure: public ptFilterBase {
  Q_OBJECT

public:
  static ptFilterBase* createExposure();

protected:
  QWidget*  doCreateGui() override;
  void      doDefineControls() override;
  bool      doCheckHasActiveCfg() override;
  void      doRunFilter(ptImage *AImage) const override;

private:
  enum class TMode {
    Manual, Auto, LikeUfraw
  };

  ptFilter_Exposure();
  double calcAutoExposure() const;

  Ui_ExposureForm FForm;

private slots:
  void onExposureModeChanged(QString AId, QVariant ANewValue);
  
};

#endif // PTFILTER_Exposure_H
