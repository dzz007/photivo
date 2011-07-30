/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptCheck::ptCheck(const QWidget* MainWindow,
                 const QString  ObjectName,
                 const QString  ParentName,
                 const QVariant Default,
                 const QString  Label,
                 const QString  ToolTip)
  :QWidget() {

  setObjectName(ObjectName);
  m_SettingsName = ObjectName;
  m_SettingsName.chop(5);

  m_Parent = MainWindow->findChild <QWidget*> (ParentName);

  if (!m_Parent) {
    fprintf(stderr,"(%s,%d) Could not find '%s'. Aborting\n",
           __FILE__,__LINE__,ParentName.toAscii().data());
    assert(m_Parent);
  }
  setParent(m_Parent);

  QHBoxLayout *Layout = new QHBoxLayout(m_Parent);

  Layout->setContentsMargins(2,2,2,2);
  Layout->setMargin(2);
  m_Parent->setLayout(Layout);

  m_CheckBox = new QCheckBox(m_Parent);
  m_CheckBox->setToolTip(ToolTip);
  m_CheckBox->setText(Label);
  m_CheckBox->setFocusPolicy(Qt::NoFocus);

  Layout->addWidget(m_CheckBox);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setMargin(0);
  Layout->addStretch(1);

  // Connect the check
  connect(m_CheckBox,SIGNAL(stateChanged(int)),
          this,SLOT(OnValueChanged(int)));

  m_DefaultValue = Default;
}

////////////////////////////////////////////////////////////////////////////////
//
// Reset
//
////////////////////////////////////////////////////////////////////////////////

void ptCheck::Reset() {
  SetValue(m_DefaultValue,0 /* Generate signal */);
}

////////////////////////////////////////////////////////////////////////////////
//
// SetValue
//
////////////////////////////////////////////////////////////////////////////////

void ptCheck::SetValue(const QVariant Value,
                       const short    BlockSignal) {
  if (Value.type() != QVariant::Int) {
    printf("(%s,%d) this : %s Value.type() : %d\n",
           __FILE__,__LINE__,
           this->objectName().toAscii().data(),
           Value.type());
    assert(Value.type() == QVariant::Int);
  }

  m_Value = Value;

  m_CheckBox->blockSignals(BlockSignal);
  m_CheckBox->setChecked(Value.toInt());
  m_CheckBox->blockSignals(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// SetEnabled
//
////////////////////////////////////////////////////////////////////////////////

void ptCheck::SetEnabled(const short Enabled) {
  m_CheckBox->setEnabled(Enabled);
}

////////////////////////////////////////////////////////////////////////////////
//
// Show
//
////////////////////////////////////////////////////////////////////////////////

void ptCheck::Show(const short Show) {
  m_Parent->setVisible(Show);
}

////////////////////////////////////////////////////////////////////////////////
//
// OnValueChanged
// Translate to an 'external' signal
//
////////////////////////////////////////////////////////////////////////////////

void ptCheck::OnValueChanged(int) {
  // We don't want bools in our variant system. Complex enough without.
  m_Value = (int) m_CheckBox->isChecked();
  //printf("(%s,%d) emiting signal(%d)\n",__FILE__,__LINE__,m_Value.toInt());
  emit(valueChanged(m_Value));
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptCheck::~ptCheck() {
  //printf("(%s,%d) %s this:%p name:%s\n",
  //       __FILE__,__LINE__,__PRETTY_FUNCTION__,
  //       this,objectName().toAscii().data());
  // Do not remove what is handled by Parent !
}

////////////////////////////////////////////////////////////////////////////////
