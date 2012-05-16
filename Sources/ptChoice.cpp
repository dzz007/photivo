/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

#include "ptChoice.h"
#include "ptSettings.h"
#include "ptInfo.h"
#include "filters/ptCfgItem.h"

extern QTranslator appTranslator;

//==============================================================================

ptChoice::ptChoice(const QWidget*          MainWindow,
                   const QString           ObjectName,
                   const QString           ParentName,
                   const short             HasDefaultValue,
                   const QVariant          Default,
                   const ptGuiOptionsItem* InitialOptions,
                   const QString           ToolTip,
                   const int               TimeOut)
: ptWidget(nullptr),
  m_ComboBox(nullptr)
{
  if (Default.type() != QVariant::Int) {
    printf("(%s,%d) this : %s\n"
           "Default.type() : %d\n",
           __FILE__,__LINE__,
           ObjectName.toAscii().data(),
           Default.type());
    assert (Default.type() == QVariant::Int);
 }

  FIsNewSchool  = false;
  m_HaveDefault = HasDefaultValue;

  setObjectName(ObjectName);
  m_SettingsName = ObjectName;
  m_SettingsName.chop(6);

  m_Parent = MainWindow->findChild <QWidget*> (ParentName);

  if (!m_Parent) {
    fprintf(stderr,"(%s,%d) Could not find '%s'. Aborting\n",
           __FILE__,__LINE__,ParentName.toAscii().data());
    assert(m_Parent);
  }
  setParent(m_Parent);
  m_InitialOptions = InitialOptions;
  m_TimeOut = TimeOut;
  m_DefaultValue = Default;

  createGUI();
  m_ComboBox->setToolTip(ToolTip);
}

//==============================================================================

ptChoice::ptChoice(const ptCfgItem &ACfgItem, QWidget *AParent)
: ptWidget(AParent),
  m_ComboBox(nullptr)
{
  init(ACfgItem);
}

//==============================================================================

ptChoice::ptChoice(QWidget *AParent)
: ptWidget(AParent),
  m_ComboBox(nullptr)
{}

//==============================================================================

void ptChoice::init(const ptCfgItem &ACfgItem) {
  GInfo->Assert(this->parent(), "Parent cannot be null.", AT);
  GInfo->Assert(!m_ComboBox, "Trying to initialize an already initalized check box.", AT);
  GInfo->Assert(ACfgItem.Type == ptCfgItem::Combo,
                QString("Wrong GUI type (%1). Must be ptCfgItem::Combo.").arg(ACfgItem.Type), AT);

  this->setObjectName(ACfgItem.Id);
  m_InitialOptionsNewschool = ACfgItem.EntryList;

  FIsNewSchool      = true;
  m_SettingsName    = "";  // not used
  m_Parent          = this;
  m_DefaultValue    = ACfgItem.Default;
  m_HaveDefault     = true;
  m_TimeOut         = ptTimeout_Input;
  m_InitialOptions  = nullptr;

  createGUI();
  m_ComboBox->setToolTip(ACfgItem.ToolTip);
}

//==============================================================================

void ptChoice::createGUI() {
  QHBoxLayout *Layout = new QHBoxLayout(m_Parent);
  m_Parent->setLayout(Layout);

  m_ComboBox = new QComboBox(m_Parent);
  m_ComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  m_ComboBox->setFixedHeight(24);
  m_ComboBox->setMaxVisibleItems(24);

  if (FIsNewSchool) {
    m_ComboBox->addItems(m_InitialOptionsNewschool);

  } else {
    for (short i=0; m_InitialOptions && m_InitialOptions[i].Value !=-1; i++ ) {
      QString Temp = appTranslator.translate("QObject",m_InitialOptions[i].Text.toAscii().data());
      if (Temp == "") Temp = m_InitialOptions[i].Text;
      m_ComboBox->addItem(Temp,m_InitialOptions[i].Value);
    }
  }

  m_ComboBox->installEventFilter(this);
  m_ComboBox->setFocusPolicy(Qt::ClickFocus);
  Layout->addWidget(m_ComboBox);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setMargin(0);
  Layout->addStretch(1);
  Layout->setSpacing(2);

  // A timer for time filtering signals going outside.
  m_Timer = new QTimer(this);
  m_Timer->setSingleShot(1);

  connect(m_Timer,    SIGNAL(timeout()),      this, SLOT(OnValueChangedTimerExpired()));
  connect(m_ComboBox, SIGNAL(activated(int)), this, SLOT(OnValueChanged(int)));

  // Set the default Value (and remember for later).
  SetValue(m_DefaultValue,1);
}

//==============================================================================

ptChoice::~ptChoice() {
  /* nothing to do here */
}

//==============================================================================

void ptChoice::setValue(const QVariant &AValue) {
  bool ok;
  int i = AValue.toInt(&ok);

  if (!ok || i<0 || i>=m_ComboBox->count()) {
    GInfo->Raise("Invalid combo index: " + AValue.toString(), AT);
  } else {
    m_ComboBox->setCurrentIndex(i);
  }
}

//==============================================================================

void ptChoice::SetValue(const QVariant Value,
                        const short    BlockSignal) {
  m_Value = Value;
  for (int i=0; i<m_ComboBox->count(); i++) {
   if (Value == m_ComboBox->itemData(i)) {
     m_ComboBox->setCurrentIndex(i);
     // setCurrentIndex does not generate signals !
     if (!BlockSignal) OnValueChanged(i);
     break;
   }
  }
}

//==============================================================================

// Keeps Data unique, i.e. replace if exist.
// NOTE: This function is only used by ptSettings, i.e. oldschool
void ptChoice::AddOrReplaceItem(const QString Text,const QVariant Data) {
  for (int i=0; i<m_ComboBox->count(); i++) {
    if (Data == m_ComboBox->itemData(i)) {
      QString Temp = appTranslator.translate("QObject",Text.toAscii().data());
      if (Temp == "") Temp = Text;
      m_ComboBox->setItemText(i,Temp);
      return;
    }
  }
  QString Temp = appTranslator.translate("QObject",Text.toAscii().data());
  if (Temp == "") Temp = Text;
  m_ComboBox->addItem(Temp,Data);
}

//==============================================================================

void ptChoice::Clear(const short WithDefault) {
  m_ComboBox->clear();
  m_Value.clear();

  if (WithDefault) {
    // Add the standard choices again.
    if (FIsNewSchool) {
      m_ComboBox->addItems(m_InitialOptionsNewschool);

    } else {
      for (short i=0; m_InitialOptions && m_InitialOptions[i].Value !=-1; i++ ) {
        QString Temp = appTranslator.translate("QObject",m_InitialOptions[i].Text.toAscii().data());
        if (Temp == "") Temp = m_InitialOptions[i].Text;
        m_ComboBox->addItem(Temp,m_InitialOptions[i].Value);
      }
    }

    m_Value = m_DefaultValue;
  }
}

//==============================================================================

void ptChoice::Show(const short Show) {
  m_Parent->setVisible(Show);
}

//==============================================================================

void ptChoice::Reset() {
  SetValue(m_DefaultValue,0 /* Generate signal */);
}

//==============================================================================

void ptChoice::SetEnabled(const short Enabled) {
  m_ComboBox->setEnabled(Enabled);
}

//==============================================================================

void ptChoice::OnButtonClicked() {
  SetValue(m_DefaultValue,0); // Generate signal !
}

//==============================================================================

void ptChoice::OnValueChanged(int Value) {
  m_Value = m_ComboBox->itemData(Value);
  if (m_TimeOut) {
    m_Timer->start(m_TimeOut);
  } else {
    OnValueChangedTimerExpired();
  }
}

//==============================================================================

void ptChoice::OnValueChangedTimerExpired() {
  m_ComboBox->clearFocus();

  if (FIsNewSchool)
    emit ptWidget::valueChanged(this->objectName(), m_ComboBox->currentIndex());
  else
    emit valueChanged(m_Value);
}

//==============================================================================

bool ptChoice::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::ContextMenu) {
    // reset to default value
    if (m_HaveDefault && m_ComboBox->isEnabled())
      OnButtonClicked();
    return true;

  } else if (event->type() == QEvent::Wheel
             && ((QMouseEvent*)event)->modifiers()==Qt::AltModifier)
  {
    // send Alt+Wheel to parent
    if (FIsNewSchool)
      QApplication::sendEvent(this->parentWidget(), event);
    else
      QApplication::sendEvent(m_Parent, event);
    return true;

  } else {
    // pass the event on to the parent class
    return ptWidget::eventFilter(obj, event);
  }
}

//==============================================================================
