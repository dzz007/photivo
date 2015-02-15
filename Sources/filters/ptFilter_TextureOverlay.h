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

#ifndef PTFILTER_TextureOverlay_H
#define PTFILTER_TextureOverlay_H

#include "ui_ptFilter_TextureOverlay.h"
#include "ptFilterBase.h"
#include "../ptImage.h"

class ptFilter_TextureOverlay: public ptFilterBase {
  Q_OBJECT

public:
  static ptFilterBase* createTextureOverlay();

protected:
  QWidget*  doCreateGui() override;
  void      doDefineControls() override;
  bool      doCheckHasActiveCfg() override;
  void      doRunFilter(ptImage* AImage) override;
  void      doUpdateGui() override;

private:
  ptFilter_TextureOverlay();

  bool loadOverlayImage(const QString& AFilePath);

  Ui_TextureOverlayForm FForm;
  ptImage FOverlayImage;

private slots:
  void onMaskModeChanged(const QString AId, const QVariant ANewValue);
  void onInnerRadiusChanged(const QString AId, const QVariant ANewValue);
  void onOuterRadiusChanged(const QString AId, const QVariant ANewValue);
  void onLoadImageButtonClicked();

};

#endif // PTFILTER_TextureOverlay_H
