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

#ifndef PTCHECK_H
#define PTCHECK_H

//==============================================================================

#include "ptWidget.h"

#include <QCheckBox>

class ptCfgItem;

//==============================================================================

class ptCheck: public ptWidget {
Q_OBJECT

public:
  /*! Old school constructor. Do not use in the new GUI structure. */
  ptCheck(const QWidget* MainWindow,
          const QString  ObjectName,
          const QString  ParentName,
          const QVariant Default,
          const QString  Label,
          const QString  ToolTip);

  /*! New school constructor. *Do* use in the new GUI structure. */
  ptCheck(const ptCfgItem &ACfgItem, QWidget *AParent);

  /*! New school constructor for widgets created through Qt Designer. You probably will not
      use this one manually. After construction you must call \c init() before the widget
      is useable.
  */
  ptCheck(QWidget *AParent);

  ~ptCheck();

  /*! Reimplemented from base class.
      For compatibility with the new GUI structure. Just calls SetValue().
   */
  void    setValue(const QVariant &AValue);

  void    setText(const QString&) {}  // dummy for QtDesigner generated promoted widgets
  void    init(const ptCfgItem &ACfgItem);

  void    SetValue(const QVariant Value, const short BlockSignal = 1);
  void    SetEnabled(const short Enabled);
  void    Show(const short Show);
  void    Reset();
  QString GetName() {return m_SettingsName;}

//-------------------------------------

private:
  void createGUI(QWidget* AParent);

  QWidget*   m_Parent;
  QVariant   m_DefaultValue;
  QVariant   m_Value;
  QCheckBox* m_CheckBox;
  QString    m_SettingsName;

//-------------------------------------

signals:
  void valueChanged(QVariant Value);

//-------------------------------------

private slots:
  void OnValueChanged(int Value);

};

#endif // PTCHECK_H
