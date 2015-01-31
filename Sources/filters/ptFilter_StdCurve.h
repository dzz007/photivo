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

#ifndef PTFILTER_StdCurve_H
#define PTFILTER_StdCurve_H

#include "ptFilterBase.h"
#include "../ptCurve.h"
#include <memory>

class ptCurveWindow;

/*!
   \brief The \c ptFilter_StdCurve class holds all curve-only filters.

   Curve filters are here when they have only a curve and no additional config items.
 */
class ptFilter_StdCurve: public ptFilterBase {
  Q_OBJECT

public:
  static ptFilterBase *CreateRgbCurve();
  static ptFilterBase *CreateTextureCurve();
  static ptFilterBase *CreateLumaByHueCurve();
  static ptFilterBase *CreateHueCurve();
  static ptFilterBase *CreateLCurve();
  static ptFilterBase *CreateRToneCurve();
  static ptFilterBase *CreateGToneCurve();
  static ptFilterBase *CreateBToneCurve();
  static ptFilterBase *CreateAfterGammaCurve();

protected:
  void      doDefineControls() override;
  bool      doCheckHasActiveCfg() override;
  void      doRunFilter(ptImage *AImage) override;

private:
  ptFilter_StdCurve(std::shared_ptr<ptCurve> ACurve);
};

#endif // PTFILTER_StdCurve_H
