/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Sergey Salnikov <salsergey@gmail.com>
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

#include "ptSlider.h"
#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>

ptSlider::ptSlider(QWidget *parent,
                   const QString  Label,
                   const QString  ToolTip,
                   const QVariant::Type Type,
                   const QVariant Minimum,
                   const QVariant Maximum,
                   const QVariant Default,
                   const QVariant Step,
                   const int      Decimals) :
  QSlider(parent)
{
  Q_ASSERT(Minimum.type() == Type && Maximum.type() == Type && Default.type() == Type && Step.type() == Type);
  m_EditBox=NULL;
  setMinimum(0);
  setMaximum(100);
  init(Label, ToolTip, Type, Minimum, Maximum, Default, Step, Decimals);
  m_IsSliderMoving=false;
  m_IsEditingEnabled=false;
  setMinimumSize(100, 20);
  setFixedHeight(22);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setOrientation(Qt::Horizontal);

  connect(this, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));
  connect(this, SIGNAL(sliderMoved(int)), this, SLOT(setValue(int)));
}

void ptSlider::init(const QString  Label,
                    const QString  ToolTip,
                    const QVariant::Type Type,
                    const QVariant Minimum,
                    const QVariant Maximum,
                    const QVariant Default,
                    const QVariant Step,
                    const int      Decimals)
{
  Q_ASSERT(Minimum.type() == Type && Maximum.type() == Type && Default.type() == Type && Step.type() == Type);
  m_Label=Label;
  setToolTip(ToolTip);
  m_Type=Type;
  m_Minimum=Minimum;
  m_Maximum=Maximum;
  m_Default=Default;
  m_Step=Step;
  m_Decimals=Decimals;
  m_Value=Default;
  setValue(minimum()+(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble())*
           (m_Value.toDouble()-m_Minimum.toDouble()));

  if (m_EditBox)
    delete m_EditBox;
  if (m_Type == QVariant::Int)
  {
    m_EditBox=new QSpinBox(this);
    QSpinBox* IntSpinBox = qobject_cast<QSpinBox*>(m_EditBox);
    IntSpinBox->setMinimum(m_Minimum.toInt());
    IntSpinBox->setMaximum(m_Maximum.toInt());
    IntSpinBox->setSingleStep(m_Step.toInt());
    connect(m_EditBox, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));
  }
  if (m_Type == QVariant::Double)
  {
    m_EditBox=new QDoubleSpinBox(this);
    QDoubleSpinBox* DoubleSpinBox = qobject_cast<QDoubleSpinBox*>(m_EditBox);
    DoubleSpinBox->setMinimum(m_Minimum.toDouble());
    DoubleSpinBox->setMaximum(m_Maximum.toDouble());
    DoubleSpinBox->setSingleStep(m_Step.toDouble());
    DoubleSpinBox->setDecimals(m_Decimals);
    connect(m_EditBox, SIGNAL(valueChanged(double)), this, SLOT(setValue(double)));
  }

  m_EditBox->setGeometry(m_ValueRect);
  m_EditBox->setAlignment(Qt::AlignRight);
  m_EditBox->installEventFilter(this);
  m_EditBox->hide();
  connect(m_EditBox, SIGNAL(editingFinished()), this, SIGNAL(editingFinished()));
}

bool ptSlider::event(QEvent *event)
{
  switch (event->type())
  {
  case QEvent::HoverMove:
    hoverMoveEvent((QHoverEvent*)event);
    break;
  default:
    break;
  }
  return QSlider::event(event);
}

bool ptSlider::eventFilter(QObject *obj, QEvent *ev)
{
  if (obj == m_EditBox)
  {
    switch (ev->type())
    {
    case QEvent::MouseButtonPress:
      if (((QMouseEvent*)ev)->button() == Qt::RightButton)
        enableEditor(false);
      break;
    case QEvent::ContextMenu:
    case QEvent::FocusOut:
      enableEditor(false);
      return true;
    case QEvent::KeyPress:
      m_EditBox->blockSignals(true);
      break;
    case QEvent::Wheel:
      m_EditBox->blockSignals(false);
      break;
    default:
      break;
    }
    return QSlider::eventFilter(obj, ev);
  }
  else
    return QSlider::eventFilter(obj, ev);
}

void ptSlider::hoverMoveEvent(QHoverEvent *ev)
{
  ev->accept();

  int pos=qRound((sliderPosition()*double(width())/(maximum()-minimum())));
  if (qAbs(ev->pos().x()-pos) < 8)
    setCursor(QCursor(Qt::SizeHorCursor));
  else
    if (m_ValueRect.contains(ev->pos()))
      setCursor(QCursor(Qt::IBeamCursor));
    else
      setCursor(QCursor(Qt::ArrowCursor));
}

void ptSlider::keyPressEvent(QKeyEvent *ev)
{
  ev->accept();

  if (ev->key() == Qt::Key_Return && m_IsEditingEnabled)
  {
    m_EditBox->blockSignals(false);
    enableEditor(false);
  }
}

void ptSlider::mouseMoveEvent(QMouseEvent *ev)
{
  ev->accept();

  if (m_IsSliderMoving)
    setValue(qMin(minimum()+qRound(ev->pos().x()/double(width())*(maximum()-minimum())), maximum()));
}

void ptSlider::mousePressEvent(QMouseEvent *ev)
{
  ev->accept();

  if (ev->button() == Qt::RightButton)
  {
    if (m_ValueRect.contains(ev->pos()))
      enableEditor(true);
    else
    {
      if (m_IsEditingEnabled)
        enableEditor(false);
      setValue(minimum()+(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble())*
               (m_Default.toDouble()-m_Minimum.toDouble()));
      triggerAction(SliderNoAction);
      setRepeatAction(SliderNoAction);
    }
    return;
  }

  if (m_IsEditingEnabled)
    enableEditor(false);
  int pos=qRound((sliderPosition()*double(width())/(maximum()-minimum())));
  if (qAbs(ev->pos().x()-pos) < 8)
  {
    m_IsSliderMoving=true;
    triggerAction(SliderMove);
    setRepeatAction(SliderNoAction);
  }
  else
  {
    SliderAction action=SliderNoAction;
    if (ev->pos().x() > pos)
    {
      action=SliderPageStepAdd;
    }
    else
      if (ev->pos().x() < pos)
      {
        action=SliderPageStepSub;
      }
    triggerAction(action);
    setRepeatAction(action);
  }
}

void ptSlider::mouseReleaseEvent(QMouseEvent *ev)
{
  ev->accept();

  m_IsSliderMoving=false;
  setRepeatAction(SliderNoAction);
}

void ptSlider::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  QStyleOptionProgressBar optBar;
  optBar.initFrom(this);
  optBar.minimum=minimum();
  optBar.maximum=maximum();
  optBar.progress=minimum()+qRound((maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble())*
                                   (m_Value.toDouble()-m_Minimum.toDouble()));
  QString val;
  if (m_Type == QVariant::Int)
    val=m_Value.toString();
  if (m_Type == QVariant::Double)
    val=locale().toString(m_Value.toDouble(), 'f', m_Decimals);

  style()->drawControl(QStyle::CE_ProgressBar, &optBar, &p, this);

//  QStyleOptionSpinBox optBox;
//  optBox.initFrom(this);
//  optBox.activeSubControls=QStyle::SC_SpinBoxDown | QStyle::SC_SpinBoxUp;
//  style()->drawComplexControl(QStyle::CC_SpinBox, &optBox, &p, this);

  p.setPen(palette().text().color());
  if (!m_IsEditingEnabled)
    p.drawText(m_ValueRect.translated(-8, 0), Qt::AlignRight | Qt::AlignVCenter, val);
  p.drawText(QRect(0, 0, width()-m_ValueRect.width(), height()), Qt::AlignLeft | Qt::AlignVCenter, "  "+m_Label);
}

void ptSlider::resizeEvent(QResizeEvent *)
{
  initValueRect();
  m_EditBox->setGeometry(m_ValueRect);
}

void ptSlider::wheelEvent(QWheelEvent *ev)
{
  if (m_IsEditingEnabled)
    enableEditor(false);
  if (m_ValueRect.contains(ev->pos()))
    setValue(minimum()+(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble())*
             (m_Value.toDouble()+m_Step.toDouble()*ev->delta()/120-m_Minimum.toDouble()));
  else
    QSlider::wheelEvent(ev);
}

void ptSlider::setValue(int val)
{
  if (m_EditBox && sender() == m_EditBox)
  {
    setValue(QVariant(val));
    val=minimum()+qRound((val-m_Minimum.toInt())*(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble()));
    QSlider::setValue(val);
  }
  else
  {
    QSlider::setValue(val);
    double v=m_Minimum.toDouble()+
        (val-minimum())/double(maximum()-minimum())*(m_Maximum.toDouble()-m_Minimum.toDouble());
    if (m_Type == QVariant::Int)
      setValue(QVariant(qRound(v)));
    if (m_Type == QVariant::Double)
      setValue(QVariant(v));
  }
}

void ptSlider::setValue(double val)
{
  if (m_EditBox && sender() == m_EditBox)
  {
    setValue(QVariant(val));
    val=minimum()+(val-m_Minimum.toDouble())*(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble());
    QSlider::setValue(qRound(val));
  }
  else
  {
    QSlider::setValue(qRound(val));
    double v=m_Minimum.toDouble()+
        (val-minimum())/double(maximum()-minimum())*(m_Maximum.toDouble()-m_Minimum.toDouble());
    if (m_Type == QVariant::Int)
      setValue(QVariant(qRound(v)));
    if (m_Type == QVariant::Double)
      setValue(QVariant(v));
  }
}

void ptSlider::setValue(QVariant val)
{
  if (m_Value != val)
  {
    if (m_Type == QVariant::Int)
      m_Value=qBound(m_Minimum.toInt(), val.toInt(), m_Maximum.toInt());
    if (m_Type == QVariant::Double)
      m_Value=qBound(m_Minimum.toDouble(), val.toDouble(), m_Maximum.toDouble());
    emit valueChanged(val);
    //    updateValueRect();
  }
}

void ptSlider::enableEditor(bool e)
{
  if (e)
  {
    m_IsEditingEnabled=true;
    if (m_Type == QVariant::Int)
      qobject_cast<QSpinBox*>(m_EditBox)->setValue(m_Value.toInt());
    if (m_Type == QVariant::Double)
      qobject_cast<QDoubleSpinBox*>(m_EditBox)->setValue(m_Value.toDouble());
    m_EditBox->show();
    m_EditBox->selectAll();
    m_EditBox->setFocus();
  }
  else
  {
    m_IsEditingEnabled=false;
    if (m_Type == QVariant::Int)
      setValue(minimum()+(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble())*
               (qobject_cast<QSpinBox*>(m_EditBox)->value()-m_Minimum.toInt()));
    if (m_Type == QVariant::Double)
      setValue(minimum()+(maximum()-minimum())/(m_Maximum.toDouble()-m_Minimum.toDouble())*
               (qobject_cast<QDoubleSpinBox*>(m_EditBox)->value()-m_Minimum.toDouble()));
    m_EditBox->hide();
    setFocus();
  }
  update();
}

void ptSlider::initValueRect()
{
  QFontMetrics metrics=fontMetrics();
  int w=55;
  if (m_Type == QVariant::Int)
    w=metrics.width(m_Maximum.toString()+"     ");
  if (m_Type == QVariant::Double)
    w=metrics.width(locale().toString(m_Maximum.toDouble(), 'f', m_Decimals)+"     ");
  //	w=qMax(w, 50);
  //  int h=metrics.height();
  m_ValueRect=QRect(width()-w, 0, w, height());
}

void ptSlider::SetMinimum(const QVariant min)
{
  Q_ASSERT(min.type() == m_Type);
  m_Minimum=min;
  if (m_Type == QVariant::Int)
    m_Value=qBound(m_Minimum.toInt(), m_Value.toInt(), m_Maximum.toInt());
  if (m_Type == QVariant::Double)
    m_Value=qBound(m_Minimum.toDouble(), m_Value.toDouble(), m_Maximum.toDouble());
}

void ptSlider::SetMaximum(const QVariant max)
{
  Q_ASSERT(max.type() == m_Type);
  m_Maximum=max;
  if (m_Type == QVariant::Int)
    m_Value=qBound(m_Minimum.toInt(), m_Value.toInt(), m_Maximum.toInt());
  if (m_Type == QVariant::Double)
    m_Value=qBound(m_Minimum.toDouble(), m_Value.toDouble(), m_Maximum.toDouble());
}
