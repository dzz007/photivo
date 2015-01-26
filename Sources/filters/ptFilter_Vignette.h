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

#ifndef PTFILTER_Vignette_H
#define PTFILTER_Vignette_H

#include "ui_ptFilter_Vignette.h"
#include "ptFilterBase.h"

//==============================================================================

class ptFilter_Vignette: public ptFilterBase {
  Q_OBJECT

public:
  static ptFilterBase* createVignetteRgb();
  static ptFilterBase* createVignetteLab();


protected:
  QWidget*  doCreateGui() override;
  void      doDefineControls() override;
  bool      doCheckHasActiveCfg() override;
  void      doRunFilter(ptImage *AImage) const override;


private:
  enum class TColorSpace { Rgb, Lab };

  ptFilter_Vignette(TColorSpace AColorSpace);

  TColorSpace FColorSpace;
  Ui_VignetteForm FForm;

private slots:
  void onInnerRadiusChanged(const QString, const QVariant ANewValue);
  void onOuterRadiusChanged(const QString, const QVariant ANewValue);
};

#endif // PTFILTER_Vignette_H
