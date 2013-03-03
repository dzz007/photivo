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
#include <ptDefines.h>
#include <ptInfo.h>

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TButton &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storable(AValues.Storable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Checkable(AValues.Checkable),
  Decimals(-1),
  AssocObject(nullptr)
{
  init();
}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCheck &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storable(AValues.Storable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  Checkable(false),
  Decimals(-1),
  AssocObject(nullptr)
{
  init();
}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCombo &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storable(AValues.Storable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  Checkable(false),
  EntryList(AValues.EntryList),
  Decimals(-1),
  AssocObject(nullptr)
{
  init();
}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TInput &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storable(AValues.Storable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  Checkable(false),
  Min(AValues.Min),
  Max(AValues.Max),
  StepSize(AValues.StepSize),
  Decimals(AValues.Decimals),
  AssocObject(nullptr)
{
  init();
}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCurve &AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(true),
  Storable(false),  // Curves have an extra default store
  Caption(AValues.Caption),
  AssocObject(AValues.Curve.get()),
  Curve(AValues.Curve)
{
  Default = QVariant(AssocObject->storeConfig(""));
  init();
}

//==============================================================================

void ptCfgItem::init() {
  setVariantType();
}

//==============================================================================

QVariant ptCfgItem::validate(QVariant AValue) {
  ensureVariantType(AValue);

  switch (this->Type) {
    case SpinEdit:  // fall through
    case Slider:    // fall through
    case HueSlider: {
      if (this->Decimals > 0)
        return ptBound(this->Min.toDouble(), AValue.toDouble(), this->Max.toDouble());
      else
        return ptBound(this->Min.toInt(), AValue.toInt(), this->Max.toInt());
    }

    case Combo: {
      auto hValue = AValue.toInt();
      for (TComboEntry &hEntry: this->EntryList) {
        if (hValue == hEntry.value)
          return hValue;
      }
      return this->EntryList[0].value;
    }

    default:
      return AValue;
  }
}

//==============================================================================

void ptCfgItem::ensureVariantType(QVariant &AValue) {
  if (AValue.type() != FIntendedType) {
    if (!AValue.convert(FIntendedType)) {
      GInfo->Raise(QString("Could not cast QVariant with value \"%1\" from type \"%2\" to "
                           "type \"%3\".")
                   .arg(AValue.toString()).arg((int)AValue.type()).arg((int)FIntendedType),
                   AT);
    }
  }
}

//==============================================================================

void ptCfgItem::setVariantType() {
  if (this->Type == Button) {
    FIntendedType = QVariant::Bool;

  } else if (this->Type == CurveWin) {
    FIntendedType = QVariant::UserType;

  } else if ((this->Type == Check) || (this->Type == Combo) || (this->Decimals == 0)) {
    FIntendedType = QVariant::Int;

  } else if (this->Decimals > 0) {
    FIntendedType = QVariant::Double;

  } else {
    GInfo->Raise(QString("Could not determine data type of \"%1\".").arg(this->Id));
  }
}

