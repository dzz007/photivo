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

#ifndef PTCHOICE_H
#define PTCHOICE_H

//==============================================================================

#include <QtGui>
#include "ptGuiOptions.h"
#include "ptWidget.h"
#include "ptInfo.h"

class ptCfgItem;

//==============================================================================

class ptChoice: public ptWidget {
Q_OBJECT

public:
  ptChoice(const QWidget*          MainWindow,
           const QString           ObjectName,
           const QString           ParentName,
           const short             HasDefaultValue,
           const QVariant          Default,
           const ptGuiOptionsItem* InitialOptions,
           const QString           ToolTip,
           const int               TimeOut);
  ptChoice(const ptCfgItem &ACfgItem, QWidget *AParent);
  ptChoice(QWidget *AParent);
  ~ptChoice();

  /*! Reimplemented from base class.
      For compatibility with the new GUI structure. Just calls SetValue().
   */
  void    setValue(const QVariant &AValue);

  void    init(const ptCfgItem &ACfgItem);

  void      SetValue(const QVariant Value, const short BlockSignal = 1);
  void      AddOrReplaceItem(const QString Text,const QVariant Data);
  int       Count(void) { return m_ComboBox->count(); }
  void      SetEnabled(const short Enabled);
  void      Clear(const short WithDefault = 0);
  void      Show(const short Show);
  QVariant  GetItemData(const int Index) { return m_ComboBox->itemData(Index); }
  QString   CurrentText(void) { return m_ComboBox->currentText(); }
  void      Reset();
  QString   GetName() {return m_SettingsName;}

//-------------------------------------

protected:
  bool eventFilter(QObject *obj, QEvent *event);

//-------------------------------------

private:
  void createGUI();

  QVariant m_Value;
  QVariant m_DefaultValue;
  short    m_HaveDefault;
  int      m_TimeOut;
  const ptGuiOptionsItem* m_InitialOptions;
  QStringList             m_InitialOptionsNewschool;

  QWidget*     m_Parent;
  QComboBox*   m_ComboBox;
  QToolButton* m_Button;
  QTimer*      m_Timer;
  QString      m_SettingsName;

//-------------------------------------

signals:
  void valueChanged(QVariant Value);

//-------------------------------------

private slots:
  void OnValueChanged(int Value);
  void OnButtonClicked();
  void OnValueChangedTimerExpired();

};

#endif // PTCHOICE_H
