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

#include "ptFilter_SimpleTone.h"
#include "ptCfgItem.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CSimpleToneId = "SimpleTone";

const QString CChannelR = "ChannelR";
const QString CChannelG = "ChannelG";
const QString CChannelB = "ChannelB";

//------------------------------------------------------------------------------

ptFilter_SimpleTone::ptFilter_SimpleTone():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_SimpleTone::createSimpleTone() {
  auto hInstance         = new ptFilter_SimpleTone;
  hInstance->FFilterName = CSimpleToneId;
  hInstance->FCaption    = tr("Simple toning");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_SimpleTone::doDefineControls() {
  FConfig.initStores(TCfgItemList()
    //            Id          Type                      Default     Min           Max           Step        Decimals, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CChannelR,  ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Red"),   tr("")})
    << ptCfgItem({CChannelG,  ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Green"), tr("")})
    << ptCfgItem({CChannelB,  ptCfgItem::Slider,        0.0,       -1.0,          1.0,          0.1,        2,        true, true, tr("Blue"),  tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_SimpleTone::doCheckHasActiveCfg() {
  return
      !qFuzzyIsNull(FConfig.value(CChannelR).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CChannelG).toDouble()) ||
      !qFuzzyIsNull(FConfig.value(CChannelB).toDouble());
}

//------------------------------------------------------------------------------

void ptFilter_SimpleTone::doRunFilter(ptImage *AImage) {
  AImage->toRGB();
  AImage->SimpleTone(
      FConfig.value(CChannelR).toDouble(),
      FConfig.value(CChannelG).toDouble(),
      FConfig.value(CChannelB).toDouble());
}

//------------------------------------------------------------------------------

RegisterHelper SimpleToneRegister(&ptFilter_SimpleTone::createSimpleTone, CSimpleToneId);

//------------------------------------------------------------------------------
