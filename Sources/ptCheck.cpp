/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include <cassert>

#include "ptCheck.h"
#include "ptInfo.h"
#include "filters/ptCfgItem.h"

//==============================================================================

ptCheck::ptCheck(const QWidget* MainWindow,
                 const QString  ObjectName,
                 const QString  ParentName,
                 const QVariant Default,
                 const QString  Label,
                 const QString  ToolTip)
: ptWidget(nullptr),
  m_CheckBox(nullptr)
{
  FIsNewSchool = false;
  setObjectName(ObjectName);
  m_SettingsName = ObjectName;
  m_SettingsName.chop(5);
  m_DefaultValue = Default;

  m_Parent = MainWindow->findChild <QWidget*> (ParentName);
  if (!m_Parent) {
    fprintf(stderr,"(%s,%d) Could not find '%s'. Aborting\n",
           __FILE__,__LINE__,ParentName.toAscii().data());
    assert(m_Parent);
  }
  setParent(m_Parent);

  createGUI(m_Parent);
  m_CheckBox->setToolTip(ToolTip);
  m_CheckBox->setText(Label);
}

//==============================================================================

ptCheck::ptCheck(const ptCfgItem &ACfgItem, QWidget *AParent)
: ptWidget(AParent),
  m_CheckBox(nullptr)
{
  init(ACfgItem);
}

//==============================================================================

ptCheck::ptCheck(QWidget *AParent)
: ptWidget(AParent),
  m_CheckBox(nullptr)
{}

//==============================================================================

void ptCheck::init(const ptCfgItem &ACfgItem) {
  GInfo->Assert(this->parent(), "Parent cannot be null.", AT);
  GInfo->Assert(!m_CheckBox, "Trying to initialize an already initalized check box.", AT);
  GInfo->Assert(ACfgItem.Type == ptCfgItem::Check,
                QString("Wrong GUI type (%1). Must be ptCfgItem::Check.").arg(ACfgItem.Type), AT);

  this->setObjectName(ACfgItem.Id);
  FIsNewSchool    = true;
  m_SettingsName  = "";  // not used
  m_Parent        = this;
  m_DefaultValue  = ACfgItem.Default;

  createGUI(m_Parent);
  m_CheckBox->setToolTip(ACfgItem.ToolTip);
  m_CheckBox->setText(ACfgItem.Caption);

}

//==============================================================================

void ptCheck::createGUI(QWidget *AParent) {
  QHBoxLayout *Layout = new QHBoxLayout(AParent);

  Layout->setContentsMargins(2,2,2,2);
  Layout->setMargin(2);
  AParent->setLayout(Layout);

  m_CheckBox = new QCheckBox(AParent);
  m_CheckBox->setFocusPolicy(Qt::NoFocus);

  Layout->addWidget(m_CheckBox);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setMargin(0);
  Layout->addStretch(1);

  // Connect the check
  connect(m_CheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged(int)));
}

//==============================================================================

ptCheck::~ptCheck() {/*nothing to do here*/}

//==============================================================================

void ptCheck::Reset() {
  SetValue(m_DefaultValue,0 /* Generate signal */);
}

//==============================================================================

void ptCheck::setValue(const QVariant &AValue) {
  this->SetValue(AValue, 1);
}

//==============================================================================

void ptCheck::SetValue(const QVariant Value, const short BlockSignal) {
  // Workaround for bool in new-style filters.
  auto hValue = Value;
  if (hValue.type() == QVariant::Bool)
    hValue.convert(QVariant::Int);

  if (hValue.type() != QVariant::Int) {
    printf("(%s,%d) this : %s Value.type() : %d\n",
           __FILE__,__LINE__,
           this->objectName().toAscii().data(),
           hValue.type());
    assert(hValue.type() == QVariant::Int);
  }

  m_Value = hValue;

  m_CheckBox->blockSignals(BlockSignal);
  m_CheckBox->setChecked(hValue.toInt());
  m_CheckBox->blockSignals(0);
}

//==============================================================================

void ptCheck::SetEnabled(const short Enabled) {
  m_CheckBox->setEnabled(Enabled);
}

//==============================================================================

void ptCheck::Show(const short Show) {
  m_Parent->setVisible(Show);
}

//==============================================================================

void ptCheck::OnValueChanged(int) {
  // We don't want bools in our variant system. Complex enough without.
  m_Value = (int) m_CheckBox->isChecked();

  if (FIsNewSchool)
    emit ptWidget::valueChanged(this->objectName(), m_CheckBox->isChecked());
  else
    emit valueChanged(m_Value);
}

//==============================================================================
