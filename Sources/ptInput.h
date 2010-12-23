////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#ifndef DLINPUT_H
#define DLINPUT_H

#include <QtGui>

////////////////////////////////////////////////////////////////////////////////
//
// ptInput is a object showing a input element with associated widgets
//
////////////////////////////////////////////////////////////////////////////////

class ptInput : public QObject {

Q_OBJECT

public :

// Constructor.
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
// Destructor.
~ptInput();

// BlockSignal avoids a signal emitted on programmatic update.
void SetValue(const QVariant Value, const short BlockSignal = 1);
void SetMaximum(const QVariant Value);
void SetMinimum(const QVariant Value);
void SetEnabled(const short Enabled);
void Show(const short Show);
void Reset();
QString GetName() {return m_SettingsName;}

private slots:
void OnSpinBoxChanged(int Value);
void OnSpinBoxChanged(double Value);
void OnSliderChanged(int Value);
void OnButtonClicked();
void OnValueChanged(int Value);
void OnValueChanged(double Value);
void OnValueChangedTimerExpired();
void EditingFinished();

signals :
void valueChanged(QVariant Value);

protected:
bool eventFilter(QObject *obj, QEvent *event);

private:
QVariant m_Value;
QVariant m_DefaultValue;
int      m_TimeOut;
short    m_HaveDefault;
int      m_KeyTimeOut;
int      m_Emited;

QVariant::Type    m_Type; // All values (and determines spinbox f.i.)
QAbstractSpinBox* m_SpinBox; // Common base for int and double.
QSlider*          m_Slider;
QToolButton*      m_Button;
QLabel*           m_Label;
QTimer*           m_Timer;
QString           m_SettingsName;
};

#endif

////////////////////////////////////////////////////////////////////////////////
