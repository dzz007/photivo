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

#include "ptFilter_LabTransform.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//==============================================================================

const QString CLabTransformId = "LabTransform";
const QString CMode           = "Mode";

//==============================================================================

ptFilter_LabTransform::ptFilter_LabTransform()
: ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  internalInit();
}

//==============================================================================

ptFilterBase *ptFilter_LabTransform::createLabTransform() {
  auto hInstance         = new ptFilter_LabTransform;
  hInstance->FFilterName = CLabTransformId;
  hInstance->FCaption    = tr("Lab transform");
  return hInstance;
}

//==============================================================================

void ptFilter_LabTransform::doDefineControls() {
  QList<ptCfgItem::TComboEntry> hTransformMode;
  hTransformMode.append({tr("Regular L*"), ptLABTransform_L, "regular"});
  hTransformMode.append({tr("R -> L*"),    ptLABTransform_R, "RtoL"});
  hTransformMode.append({tr("G -> L*"),    ptLABTransform_G, "GtoL"});
  hTransformMode.append({tr("B -> L*"),    ptLABTransform_B, "BtoL"});

  FCfgItems = QList<ptCfgItem>()
    //            Id         Type                  Default                              commonConnect, storeable, caption, tooltip
    << ptCfgItem({CMode,     ptCfgItem::Combo,     ptLABTransform_L, hTransformMode,    true, true, tr("Transformation"),   tr("")})
  ;
}

//==============================================================================

bool ptFilter_LabTransform::doCheckHasActiveCfg() {
  return FConfig.value(CMode).toInt() != ptLABTransform_L;
}

//==============================================================================

void ptFilter_LabTransform::doRunFilter(ptImage *AImage) const {
  AImage->toRGB();  // Yes, this filter needs RGB input
  AImage->LABTransform(FConfig.value(CMode).toInt());
}

//==============================================================================

RegisterHelper LabTransformRegister(&ptFilter_LabTransform::createLabTransform, CLabTransformId);

//==============================================================================
