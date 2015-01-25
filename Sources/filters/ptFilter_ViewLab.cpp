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

#include "ptFilter_ViewLab.h"
#include "ptCfgItem.h"
#include "../ptConstants.h"
#include "../ptImage.h"

//------------------------------------------------------------------------------

const QString CViewLabId        = "LabChannelView";
const QString CChannelSelection = "ChannelSelection";

//------------------------------------------------------------------------------

ptFilter_ViewLab::ptFilter_ViewLab():
  ptFilterBase()
{
//  FHelpUri = "http://photivo.org/something";
  this->internalInit();
}

//------------------------------------------------------------------------------

ptFilterBase *ptFilter_ViewLab::createViewLab() {
  auto hInstance         = new ptFilter_ViewLab;
  hInstance->FFilterName = CViewLabId;
  hInstance->FCaption    = tr("View Lab channels");
  return hInstance;
}

//------------------------------------------------------------------------------

void ptFilter_ViewLab::doDefineControls() {
  const ptCfgItem::TComboEntryList channelModes({
    {tr("Lab (normal image)"),     static_cast<int>(TViewLabChannel::Lab),        "Lab"},
    {tr("L channel only"),         static_cast<int>(TViewLabChannel::L),          "L"},
    {tr("Structure in L channel"), static_cast<int>(TViewLabChannel::LStructure), "LStructure"},
    {tr("a channel"),              static_cast<int>(TViewLabChannel::a),          "a"},
    {tr("b channel"),              static_cast<int>(TViewLabChannel::b),          "b"},
    {tr("C channel"),              static_cast<int>(TViewLabChannel::C),          "C"},
    {tr("H channel"),              static_cast<int>(TViewLabChannel::H),          "H"},
  });

  FConfig.initStores(TCfgItemList()
    //            Id                 Type              Default                                 list of entries, commonConnect, storeable, caption, tooltip
    << ptCfgItem({CChannelSelection, ptCfgItem::Combo, static_cast<int>(TViewLabChannel::Lab), channelModes,    true, true, tr("Channel mode"), tr("")})
  );
}

//------------------------------------------------------------------------------

bool ptFilter_ViewLab::doCheckHasActiveCfg() {
  return static_cast<TViewLabChannel>(FConfig.value(CChannelSelection).toInt()) != TViewLabChannel::Lab;
}

//------------------------------------------------------------------------------

void ptFilter_ViewLab::doRunFilter(ptImage *AImage) const {
  AImage->toLab();
  AImage->ViewLab(static_cast<TViewLabChannel>(FConfig.value(CChannelSelection).toInt()));
}

//------------------------------------------------------------------------------

RegisterHelper ViewLabRegister(&ptFilter_ViewLab::createViewLab, CViewLabId);

//------------------------------------------------------------------------------
