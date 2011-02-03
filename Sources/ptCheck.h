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

#ifndef DLCHECK_H
#define DLCHECK_H

#include <QtGui>

////////////////////////////////////////////////////////////////////////////////
//
// ptCheck is a object showing a checkbox
//
////////////////////////////////////////////////////////////////////////////////

class ptCheck : public QObject {

Q_OBJECT

public :

ptCheck(const QWidget* MainWindow,
        const QString  ObjectName,
        const QString  ParentName,
        const QVariant Default,
        const QString  Label,
        const QString  ToolTip);
// Destructor.
~ptCheck();

void SetValue(const QVariant Value, const short BlockSignal = 1);
void SetEnabled(const short Enabled);
void Show(const short Show);
void Reset();
QString GetName() {return m_SettingsName;}

protected:

private slots:
void OnValueChanged(int Value);

signals :
void valueChanged(QVariant Value);

private:
QVariant   m_DefaultValue;
QVariant   m_Value;
QCheckBox* m_CheckBox;
QString    m_SettingsName;
};

#endif

////////////////////////////////////////////////////////////////////////////////
