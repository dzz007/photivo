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

#ifndef PTFILTER_GradualBlur_H
#define PTFILTER_GradualBlur_H

#include "ui_ptFilter_GradualBlur.h"
#include "ptFilterBase.h"
#include <QString>
#include <QVariant>

class ptFilter_GradualBlur: public ptFilterBase {
  Q_OBJECT

public:
  static ptFilterBase *createGradualBlur();

protected:
  QWidget*  doCreateGui() override;
  void      doDefineControls() override;
  bool      doCheckHasActiveCfg() override;
  void      doRunFilter(ptImage *AImage) override;
  void      doUpdateGui() override;

private:
  ptFilter_GradualBlur();
  Ui_GradualBlurForm FForm;

private slots:
  void onModeValueChanged(QString AId, QVariant ANewValue);
};

#endif // PTFILTER_GradualBlur_H
