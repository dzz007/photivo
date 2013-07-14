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
/*!
  \class ptInput
  \brief \c ptInput is a object showing a input element with associated widgets
 */

#ifndef PTINPUT_H
#define PTINPUT_H

//==============================================================================

#include "ptSlider.h"
#include "ptWidget.h"

#include <QLabel>

class ptCfgItem;

//==============================================================================

class ptInput: public ptWidget {
Q_OBJECT

public:
  ptInput(const QWidget*   MainWindow,
          const QString    ObjectName,
          const QString    ParentName,
          const short      HasSlider,
          const short      ColorSetting,
          const short      HasDefaultValue,
          const QVariant   Default,
          const QVariant   Minimum,
          const QVariant   Maximum,
          const QVariant   Step,
          const int        Decimals,
          const QString    LabelText,
          const QString    ToolTip,
          const int        TimeOut);
  ptInput(const ptCfgItem &ACfgItem, QWidget *AParent);
  ptInput(QWidget *AParent);
  ~ptInput();

  /*! Reimplemented from base class.
      For compatibility with the new GUI structure. Just calls SetValue().
   */
  void    setValue(const QVariant &AValue);

  void    init(const ptCfgItem &ACfgItem);

  // BlockSignal avoids a signal emitted on programmatic update.
  void SetValue(const QVariant Value, const short BlockSignal = 1);
  void SetMaximum(const QVariant Value);
  void SetMinimum(const QVariant Value);
  void SetEnabled(const short Enabled);
  void Show(const short Show);
  void Reset();
  QString GetName() {return m_SettingsName;}

//-------------------------------------

protected:
  bool eventFilter(QObject *obj, QEvent *event);

//-------------------------------------

private:
  void CheckTypes(const QVariant &Default, const QVariant &Minimum, const QVariant &Maximum,
                  const QVariant &Step);
  void createGUI(const QVariant &Minimum, const QVariant &Maximum, const QVariant &Step,
                 const int Decimals, const QString &ToolTip, const QString &LabelText,
                 const short ColorSetting, const int TimeOut);

  QVariant m_Value;
  QVariant m_DefaultValue;
  int      m_TimeOut;
  short    m_HaveDefault;
  short    m_HaveSlider;
  int      m_KeyTimeOut;
  int      m_Emited;

  QWidget*          m_Parent;
  QVariant::Type    m_Type; // All values (and determines spinbox f.i.)
  QAbstractSpinBox* m_SpinBox; // Common base for int and double.
  ptSlider*         m_Slider;
  //QToolButton*      m_Button;
  QLabel*           m_Label;
  QTimer*           m_Timer;
  QString           m_SettingsName;

//-------------------------------------

signals:
  void valueChanged(QVariant Value);

//-------------------------------------

private slots:
  void OnSpinBoxChanged(int Value);
  void OnSpinBoxChanged(double Value);
  //void OnSliderChanged(int Value);
  void OnSliderChanged(QVariant Value);
  void OnButtonClicked();
  void OnValueChanged(int Value);
  void OnValueChanged(double Value);
  void OnValueChangedTimerExpired();
  void EditingFinished();

};

#endif // PTINPUT_H
