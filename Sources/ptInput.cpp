////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <QMessageBox>
#include <cassert>

#include "ptInput.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptInput::ptInput(const QWidget* MainWindow,
                 const QString  ObjectName,
                 const QString  ParentName,
                 const short    HasSlider,
                 const short    ColorSetting,
                 const short    HasDefaultValue,
                 const QVariant Default,
                 const QVariant Minimum,
                 const QVariant Maximum,
                 const QVariant Step,
                 const int      Decimals,
                 const QString  LabelText,
                 const QString  ToolTip,
                 const int      TimeOut)
  :QObject() {

  m_Type = Minimum.type();

  // Check consistency of all types.
  if (Default.type() != m_Type ||
      Maximum.type() != m_Type ||
      Step.type()    != m_Type ) {
    printf("(%s,%d) this : %s\n"
           "Default.type() : %d\n"
           "Minimum.type() : %d\n"
           "Maximum.type() : %d\n"
           "Step.type() : %d\n"
           "m_Type : %d\n",
           __FILE__,__LINE__,
           ObjectName.toAscii().data(),
           Default.type(),
           Minimum.type(),
           Maximum.type(),
           Step.type(),
           m_Type);
    assert (Default.type() == m_Type);
    assert (Maximum.type() == m_Type);
    assert (Step.type()    == m_Type);
 }

  m_HaveDefault = HasDefaultValue;

  setObjectName(ObjectName);
  m_SettingsName = ObjectName;
  m_SettingsName.chop(5);

  QWidget* Parent = MainWindow->findChild <QWidget*> (ParentName);

  if (!Parent) {
    fprintf(stderr,"(%s,%d) Could not find '%s'. Aborting\n",
           __FILE__,__LINE__,ParentName.toAscii().data());
    assert(Parent);
  }
  setParent(Parent);

  QHBoxLayout *Layout = new QHBoxLayout(Parent);
  //~ Layout->setContentsMargins(2,2,2,2);
  //~ Layout->setMargin(2);
  Parent->setLayout(Layout);

  if (m_Type == QVariant::Int) {
    m_SpinBox = new QSpinBox(Parent);
    QSpinBox* IntSpinBox = qobject_cast <QSpinBox *> (m_SpinBox);
    IntSpinBox->setMinimum(Minimum.toInt());
    IntSpinBox->setMaximum(Maximum.toInt());
    IntSpinBox->setSingleStep(Step.toInt());
  } else if (m_Type == QVariant::Double) {
    m_SpinBox = new QDoubleSpinBox(Parent);
    QDoubleSpinBox* DoubleSpinBox = qobject_cast <QDoubleSpinBox *> (m_SpinBox);
    DoubleSpinBox->setMinimum(Minimum.toDouble());
    DoubleSpinBox->setMaximum(Maximum.toDouble());
    DoubleSpinBox->setSingleStep(Step.toDouble());
    DoubleSpinBox->setDecimals(Decimals);
  } else {
    assert (m_Type == QVariant::Int || m_Type == QVariant::Double);
  }

  m_SpinBox->setToolTip(ToolTip);
  m_SpinBox->setFixedHeight(24);
  m_SpinBox->setAlignment(Qt::AlignRight);
  m_SpinBox->installEventFilter(this);
  m_SpinBox->setFocusPolicy(Qt::ClickFocus);

  m_Slider = new QSlider(Qt::Horizontal,Parent);
  m_Slider->setMinimumWidth(100);
  m_Slider->setMaximumWidth(100);
  m_Slider->setMinimum(0);
  m_Slider->setMaximum(100);
  m_Slider->setToolTip(ToolTip);
  if (ColorSetting == 1) m_Slider->setObjectName("HueSlider");
  if (!HasSlider) m_Slider->hide();
  m_Slider->installEventFilter(this);
  m_Slider->setFocusPolicy(Qt::ClickFocus);

  //~ m_Button  = new QToolButton(Parent);
  //~ QIcon ButtonIcon;
  //~ ButtonIcon.addPixmap(QPixmap(
    //~ QString::fromUtf8(":/photivo/Icons/reload.png")),
    //~ QIcon::Normal, QIcon::Off);
  //~ m_Button->setIcon(ButtonIcon);
  //~ m_Button->setIconSize(QSize(14,14));
  //~ m_Button->setText(QObject::tr("Reset"));
  //~ m_Button->setToolTip(QObject::tr("Reset to defaults"));
  //~ if (!m_HaveDefault) m_Button->hide();

  m_Label = new QLabel(Parent);
  m_Label->setText(LabelText);
  m_Label->setToolTip(ToolTip);
  m_Label->setTextInteractionFlags(Qt::NoTextInteraction);
  m_Label->installEventFilter(this);

  //~ Layout->addWidget(m_Button);
  Layout->addWidget(m_SpinBox);
  Layout->addWidget(m_Slider);
  Layout->addWidget(m_Label);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setMargin(0);
  Layout->addStretch(1);
  Layout->setSpacing(2);

  // A timer for time filtering signals going outside.
  m_TimeOut = TimeOut;
  m_KeyTimeOut = 2000;
  m_Timer = new QTimer(this);
  m_Timer->setSingleShot(1);

  connect(m_Timer,SIGNAL(timeout()),
          this,SLOT(OnValueChangedTimerExpired()));

  // Linking values together.
  if (m_Type == QVariant::Int) {
    connect(m_SpinBox,SIGNAL(valueChanged(int)),
            this,SLOT(OnSpinBoxChanged(int)));
  }
  if (m_Type == QVariant::Double) {
    connect(m_SpinBox,SIGNAL(valueChanged(double)),
            this,SLOT(OnSpinBoxChanged(double)));
  }
  connect(m_Slider,SIGNAL(valueChanged(int)),
          this,SLOT(OnSliderChanged(int)));
  //~ connect(m_Button,SIGNAL(clicked()),
          //~ this,SLOT(OnButtonClicked()));
  connect(m_SpinBox,SIGNAL(editingFinished()),
          this,SLOT(EditingFinished()));

  // Set the default Value (and remember for later).
  m_DefaultValue = Default;
  SetValue(Default,1 /* block signals */);
  m_Emited = 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetValue
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::SetValue(const QVariant Value,
                       const short    BlockSignal) {

  // Hack for QT 4.6 update:
  // QVariant has now a new implicit constructor that takes a float. This
  // means that code that assigned a float to a variant would create a
  // variant with userType QMetaType::Float, instead of QVariant::Double.
  QVariant TempValue = Value;
  if (static_cast<QMetaType::Type>(TempValue.type()) == QMetaType::Float) TempValue.convert(QVariant::Double);

  // For the double->int implication , I don't want to use canConvert.
  if ( (TempValue.type() == QVariant::Double && m_Type == QVariant::Int)  ||
       (TempValue.type() == QVariant::Double && m_Type == QVariant::UInt) ||
       (TempValue.type() == QVariant::Int    && m_Type == QVariant::Double) ||
       (TempValue.type() != QVariant::Int  &&
        TempValue.type() != QVariant::UInt &&
        TempValue.type() != QVariant::Double) ) {
    printf("(%s,%d) this : %s Value.type() : %d m_Type : %d\n",
           __FILE__,__LINE__,
           this->objectName().toAscii().data(),
           TempValue.type(),
           m_Type);
    assert(TempValue.type() == m_Type);
  }

  m_Value = TempValue;
  m_SpinBox->blockSignals(BlockSignal);
  m_Slider->blockSignals(BlockSignal);
  if (m_Type == QVariant::Int) {
     QSpinBox* IntSpinBox = qobject_cast <QSpinBox *> (m_SpinBox);
     IntSpinBox->setValue(Value.toInt());
     m_Slider->setValue((int)
       (0.5 + 100.0*(Value.toInt() - IntSpinBox->minimum()) /
          (IntSpinBox->maximum() - IntSpinBox->minimum())) );
  } else if (m_Type == QVariant::Double) {
    QDoubleSpinBox* DoubleSpinBox = qobject_cast <QDoubleSpinBox *> (m_SpinBox);
    DoubleSpinBox->setValue(Value.toDouble());
    m_Slider->setValue((int)
      (0.5 + 100.0*(Value.toDouble() - DoubleSpinBox->minimum()) /
        (DoubleSpinBox->maximum() - DoubleSpinBox->minimum())) );
  } else {
    assert(0);
  }
  m_SpinBox->blockSignals(0);
  m_Slider->blockSignals(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// SetMinimum
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::SetMinimum(const QVariant Value) {

  if (Value.type() != m_Type) {
    printf("(%s,%d) this : %s Value.type() : %d m_Type : %d\n",
           __FILE__,__LINE__,
           this->objectName().toAscii().data(),
           Value.type(),
           m_Type);
    assert(Value.type() == m_Type);
  }

  if (m_Type == QVariant::Int) {
    QSpinBox* IntSpinBox = qobject_cast <QSpinBox *> (m_SpinBox);
    IntSpinBox->setMinimum(Value.toInt());
  }
  if (m_Type == QVariant::Double) {
    QDoubleSpinBox* DoubleSpinBox = qobject_cast <QDoubleSpinBox *> (m_SpinBox);
    DoubleSpinBox->setMinimum(Value.toDouble());
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// SetMaximum
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::SetMaximum(const QVariant Value) {

  if (Value.type() != m_Type) {
    printf("(%s,%d) this : %s Value.type() : %d m_Type : %d\n",
           __FILE__,__LINE__,
           this->objectName().toAscii().data(),
           Value.type(),
           m_Type);
    assert(Value.type() == m_Type);
  }

  if (m_Type == QVariant::Int) {
    QSpinBox* IntSpinBox = qobject_cast <QSpinBox *> (m_SpinBox);
    IntSpinBox->setMaximum(Value.toInt());
  }
  if (m_Type == QVariant::Double) {
    QDoubleSpinBox* DoubleSpinBox = qobject_cast <QDoubleSpinBox *> (m_SpinBox);
    DoubleSpinBox->setMaximum(Value.toDouble());
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// SetEnabled
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::SetEnabled(const short Enabled) {
  m_SpinBox->setEnabled(Enabled);
  m_Slider->setEnabled(Enabled);
  //~ m_Button->setEnabled(Enabled);
}

////////////////////////////////////////////////////////////////////////////////
//
// Show
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::Show(const short Show) {
  if (Show) {
    m_SpinBox->show();
    m_Slider->show();
    //~ m_Button->show();
    m_Label->show();
  } else {
    m_SpinBox->hide();
    m_Slider->hide();
    //~ m_Button->hide();
    m_Label->hide();
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Reset
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::Reset() {
  m_SpinBox->clearFocus();
  SetValue(m_DefaultValue,0 /* Generate signal */);
}

////////////////////////////////////////////////////////////////////////////////
//
// OnSpinBoxChanged
// Set the slider. Int and Double variant.
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::OnSpinBoxChanged(int Value) {
  QSpinBox* IntSpinBox = qobject_cast <QSpinBox *> (m_SpinBox);
  m_Slider->blockSignals(true);
  m_Slider->setValue((int)
    (100.0*(Value - IntSpinBox->minimum()) /
       (IntSpinBox->maximum() - IntSpinBox->minimum()))  );
  m_Slider->blockSignals(false);
  m_Value = Value;
  OnValueChanged(Value);
}

void ptInput::OnSpinBoxChanged(double Value) {
  QDoubleSpinBox* DoubleSpinBox = qobject_cast <QDoubleSpinBox *> (m_SpinBox);
  m_Slider->blockSignals(true);
  m_Slider->setValue((int)
    (0.5 + 100.0*(Value - DoubleSpinBox->minimum()) /
       (DoubleSpinBox->maximum() - DoubleSpinBox->minimum()))  );
  m_Slider->blockSignals(false);
  m_Value = Value;
  OnValueChanged(Value);
}

////////////////////////////////////////////////////////////////////////////////
//
// OnSliderChanged
// Set the spinbox.
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::OnSliderChanged(int Value) {
  if (m_Type == QVariant::Int) {
    QSpinBox* IntSpinBox = qobject_cast <QSpinBox *> (m_SpinBox);
    IntSpinBox->blockSignals(true);
    IntSpinBox->setValue(
      (int)(IntSpinBox->minimum() +
            Value/100.0 * (IntSpinBox->maximum() - IntSpinBox->minimum())));
    IntSpinBox->blockSignals(false);
    OnValueChanged(IntSpinBox->value());
  }
  if (m_Type == QVariant::Double) {
    QDoubleSpinBox* DoubleSpinBox = qobject_cast <QDoubleSpinBox *> (m_SpinBox);
    DoubleSpinBox->blockSignals(true);
    DoubleSpinBox->setValue(
      DoubleSpinBox->minimum() +
      Value/100.0 * (DoubleSpinBox->maximum() - DoubleSpinBox->minimum()));
    DoubleSpinBox->blockSignals(false);
    OnValueChanged(DoubleSpinBox->value());
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// OnButtonClicked
// Reset to the defaults.
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::OnButtonClicked() {
  m_SpinBox->clearFocus();
  SetValue(m_DefaultValue,0 /* Generate signal */);
}

////////////////////////////////////////////////////////////////////////////////
//
// OnValueChanged
// Int and Double variant.
// Translate to an 'external' signal, maybe after time filtering.
//
////////////////////////////////////////////////////////////////////////////////

void ptInput::EditingFinished() {
  if (m_Emited == 0) {
    OnValueChangedTimerExpired();
  }
}


void ptInput::OnValueChanged(int Value) {
  m_Emited = 0;
  m_Value = QVariant(Value);
  if (m_TimeOut) {
    if(!m_SpinBox->hasFocus()) {
      m_Timer->start(m_TimeOut);
    } else {
      m_Timer->start(m_KeyTimeOut);
    }
  } else {
    OnValueChangedTimerExpired();
  }
}

void ptInput::OnValueChanged(double Value) {
  m_Emited = 0;
  m_Value = QVariant(Value);
  if (m_TimeOut) {
    if(!m_SpinBox->hasFocus()) {
      m_Timer->start(m_TimeOut);
    } else {
      m_Timer->start(m_KeyTimeOut);
    }
  } else {
    OnValueChangedTimerExpired();
  }
}

void ptInput::OnValueChangedTimerExpired() {
  if (m_Emited == 0) {
    if (m_Type == QVariant::Int)
      printf("(%s,%d) emiting signal(%d)\n",__FILE__,__LINE__,m_Value.toInt());
    if (m_Type == QVariant::Double)
      printf("(%s,%d) emiting signal(%f)\n",__FILE__,__LINE__,m_Value.toDouble());
    emit(valueChanged(m_Value));
  }
  m_Emited = 1;
  m_SpinBox->clearFocus();
}

////////////////////////////////////////////////////////////////////////////////
//
// Event filter
//
////////////////////////////////////////////////////////////////////////////////

bool ptInput::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::ContextMenu) {
    if (m_HaveDefault)
      OnButtonClicked();
    return true;
  } else if (0 && ((QMouseEvent*) event)->button()==Qt::RightButton) {
    if (m_HaveDefault)
      OnButtonClicked();
    return true;
  } else {
    // pass the event on to the parent class
    return QObject::eventFilter(obj, event);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptInput::~ptInput() {
  //printf("(%s,%d) %s this:%p name:%s\n",
  //       __FILE__,__LINE__,__PRETTY_FUNCTION__,
  //       this,objectName().toAscii().data());
  // Do not remove what is handled by Parent !
}

////////////////////////////////////////////////////////////////////////////////
