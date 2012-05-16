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

#include "ptCfgItem.h"
#include "ptFilterConfig.h"
#include <ptCurve.h>

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TButton &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storeable(AValues.Storeable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Checkable(AValues.Checkable),
  Decimals(-1)
{}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCheck &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storeable(AValues.Storeable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  Checkable(false),
  Decimals(-1)
{}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCombo &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storeable(AValues.Storeable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  Checkable(false),
  EntryList(AValues.EntryList),
  Decimals(-1)
{}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TInput &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storeable(AValues.Storeable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  Checkable(false),
  Min(AValues.Min),
  Max(AValues.Max),
  StepSize(AValues.StepSize),
  Decimals(AValues.Decimals)
{}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCurve &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(true),
  Storeable(false),
  Caption(AValues.Caption),
  Curve(AValues.Curve)
{
  Default = QVariant(Curve->filterConfig());
}

//==============================================================================
