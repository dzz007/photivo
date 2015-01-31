/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILTER_SIGCONTRAST_H
#define PTFILTER_SIGCONTRAST_H

#include "ptFilterBase.h"
#include <QObject>


/*! The \c ptFilter_SigContrast class implements sigmodial contrast for Lab and RGB. */
class ptFilter_SigContrast: public ptFilterBase {
  Q_OBJECT

public:
  static  ptFilterBase* CreateLabContrast();
  static  ptFilterBase* CreateRgbContrast();

protected:
  void doDefineControls() override;
  bool doCheckHasActiveCfg() override;
  void doRunFilter(ptImage *AImage) override;

private:
  enum class TColorSpace { Rgb, Lab };

  ptFilter_SigContrast(const QString    &AFilterName,
                       const TColorSpace AColorSpace,
                       const QString    &AGuiCaption);

  TColorSpace FColorSpace;

};

#endif // PTFILTER_SIGCONTRAST_H
