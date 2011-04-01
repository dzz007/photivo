/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#ifndef DLGUIOPTIONS_H
#define DLGUIOPTIONS_H

#include <QtCore>

#include "ptConstants.h"
#include "lensfun.h"

////////////////////////////////////////////////////////////////////////////////
//
// ptGuiOptions
//
// Bunch of structured options for the Gui choice elements;
//
////////////////////////////////////////////////////////////////////////////////

struct ptGuiOptionsItem {
  QVariant Value;
  QString  Text;
};

// Attention : heavy use of static, ptGuiOptions are
// obviously only meant to be instantiated once.

class ptGuiOptions {
public:
  static const ptGuiOptionsItem RememberSettingLevel[];
  static const ptGuiOptionsItem CameraColor[];
  static const ptGuiOptionsItem CameraColorProfileIntent[];
  static const ptGuiOptionsItem CameraColorGamma[];
  static const ptGuiOptionsItem WorkColor[];
  static const ptGuiOptionsItem CMQuality[];
  static const ptGuiOptionsItem PreviewColorProfileIntent[];
  static const ptGuiOptionsItem OutputColorProfileIntent[];
  static const ptGuiOptionsItem Style[];
  static const ptGuiOptionsItem StyleHighLight[];
  static const ptGuiOptionsItem PipeSize[];
  static const ptGuiOptionsItem RunMode[];

  static const ptGuiOptionsItem LfunCAModel[];
  static const ptGuiOptionsItem LfunVignetteModel[];
  static const ptGuiOptionsItem LfunGeo[];
  static const ptGuiOptionsItem LfunDistModel[];

  static const ptGuiOptionsItem CropGuidelines[];
  static const ptGuiOptionsItem ResizeFilter[];
  static const ptGuiOptionsItem IMResizeFilter[];
  static const ptGuiOptionsItem LqrEnergy[];
  static const ptGuiOptionsItem WhiteBalance[];
  static const ptGuiOptionsItem CACorrect[];
  static const ptGuiOptionsItem Interpolation[];
  static const ptGuiOptionsItem BayerDenoise[];
  static const ptGuiOptionsItem ClipMode[];
  static const ptGuiOptionsItem AspectRatioW[];
  static const ptGuiOptionsItem AspectRatioH[];
  static const ptGuiOptionsItem ExposureClipMode[];
  static const ptGuiOptionsItem AutoExposureMode[];
  static const ptGuiOptionsItem LMHLightRecoveryMaskType[];
  static const ptGuiOptionsItem LABTransformMode[];
  static const ptGuiOptionsItem DenoiseMask[];
  static const ptGuiOptionsItem ChannelMixer[];
  static const ptGuiOptionsItem Curve[];
  static const ptGuiOptionsItem SpecialPreview[];
  static const ptGuiOptionsItem ViewLAB[];
  static const ptGuiOptionsItem GREYCInterpolation[];
  static const ptGuiOptionsItem FilmType[];
  static const ptGuiOptionsItem ColorFilterType[];
  static const ptGuiOptionsItem FlipMode[];
  static const ptGuiOptionsItem GrainMode[];
  static const ptGuiOptionsItem MaskType[];
  static const ptGuiOptionsItem GrainMaskType[];
  static const ptGuiOptionsItem OverlayMode[];
  static const ptGuiOptionsItem OverlayMaskMode[];
  static const ptGuiOptionsItem CrossprocessMode[];
  static const ptGuiOptionsItem VignetteMode[];
  static const ptGuiOptionsItem SoftglowMode[];
  static const ptGuiOptionsItem Enable[];
  static const ptGuiOptionsItem EnableGreyC[];
  static const ptGuiOptionsItem SaveFormat[];
  static const ptGuiOptionsItem SaveSampling[];
  static const ptGuiOptionsItem OutputMode[];
  static const ptGuiOptionsItem ResetMode[];
  static const ptGuiOptionsItem BadPixels[];
  static const ptGuiOptionsItem DarkFrame[];
};

extern ptGuiOptions* GuiOptions;

#endif

////////////////////////////////////////////////////////////////////////////////
