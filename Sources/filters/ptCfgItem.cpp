/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012-2015 Bernd Schoeler <brjohn@brother-john.net>
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
#include "../ptCurve.h"
#include "../ptDefines.h"
#include "../ptInfo.h"

//==============================================================================

ptCfgItem::ptCfgItem(const TColorSelectButton& AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storable(AValues.Storable),
  ToolTip(AValues.ToolTip),
  Default(QVariant::fromValue(AValues.Default))
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
  Default(AValues.Default)
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
  EntryList(AValues.EntryList)
{
  init();
}

//==============================================================================

ptCfgItem::ptCfgItem(ptCfgItem::TCombo&& AValues):
  Id(AValues.Id),
  Type(AValues.Type),
  UseCommonDispatch(AValues.UseCommonDispatch),
  Storable(AValues.Storable),
  Caption(AValues.Caption),
  ToolTip(AValues.ToolTip),
  Default(AValues.Default),
  EntryList(std::move(AValues.EntryList))
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
  Min(AValues.Min),
  Max(AValues.Max),
  StepSize(AValues.StepSize),
  Decimals(AValues.Decimals)
{
  init();
}

//==============================================================================

ptCfgItem::ptCfgItem(const ptCfgItem::TCurve &AValues):
  Id(AValues.Id),
  Type(CurveWin),
  UseCommonDispatch(true),
  Caption(AValues.Caption),
  AssocObject(AValues.Curve.get()),
  Default(QVariant(AssocObject->storeConfig(""))),
  Curve(AValues.Curve)
{
  init();
}

//------------------------------------------------------------------------------
ptCfgItem::ptCfgItem(const ptCfgItem::TCustom& AValues):
  Id(AValues.Id),
  Type(CustomType),
  AssocObject(AValues.Object),
  Default(QVariant(AssocObject->storeConfig("")))
{
  init();
}

ptCfgItem::ptCfgItem(const ptCfgItem::TGeneric& AValue):
  Id(AValue.Id),
  Type(Generic),
  Storable(AValue.Storable),
  Default(AValue.Default)
{
  init();
}

//==============================================================================

void ptCfgItem::init() {
  setVariantType();
}

//==============================================================================

QVariant ptCfgItem::validate(const QVariant &AValue) const {
  auto hResult = QVariant(AValue);
  this->ensureVariantType(hResult);

  switch (this->Type) {
    case SpinEdit:  // fall through
    case Slider:    // fall through
    case HueSlider: {
      if (this->Decimals > 0) {
        return ptBound(this->Min.toDouble(), hResult.toDouble(), this->Max.toDouble());
      } else {
        return ptBound(this->Min.toInt(), hResult.toInt(), this->Max.toInt());
      }
    }

    case Combo: {
      auto hFoundEntry = std::find_if(
          this->EntryList.constBegin(),
          this->EntryList.constEnd(),
          [&](const TComboEntry& AEntry) { return hResult.toInt() == AEntry.value; });

      if (hFoundEntry != this->EntryList.constEnd()) {
        return hFoundEntry->value;
      };

      return this->EntryList[0].value;
    }

    default:
      return hResult;
  }
}

//==============================================================================

void ptCfgItem::ensureVariantType(QVariant &AValue) const {
  if (FIntendedType == QMetaType::User) {
    return;
  }

  if (static_cast<QMetaType::Type>(AValue.type()) != FIntendedType) {
#   if QT_VERSION >= 0x050000
    if (!AValue.convert(FIntendedType)) {
#   else
    if (!AValue.convert(static_cast<QVariant::Type>(FIntendedType))) {
#   endif
      GInfo->Raise(QString("Could not cast QVariant with value \"%1\" from type \"%2\" to "
                           "type \"%3\".")
                   .arg(AValue.toString()).arg((int)AValue.type()).arg((int)FIntendedType),
                   AT);
    }
  }
}

//==============================================================================

void ptCfgItem::setVariantType() {
  if (this->Type == Generic) {
    FIntendedType = QMetaType::User;

  } else if (this->Type == ColorSelectButton) {
    FIntendedType = QMetaType::QColor;

  } else if (this->Type >= CFirstCustomType) {
    FIntendedType = QMetaType::QVariantMap;

  } else if ((this->Type == Check) || (this->Type == Combo) || (this->Decimals == 0)) {
    FIntendedType = QMetaType::Int;

  } else if (this->Decimals > 0) {
    FIntendedType = QMetaType::Double;

  } else {
    GInfo->Raise(QString("Could not determine data type of \"%1\".").arg(this->Id));
  }
}

