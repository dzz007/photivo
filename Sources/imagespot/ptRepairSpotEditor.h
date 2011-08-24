/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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
  \class ptRepairSpotEditor

  \brief Editor widget for the spot repair ListView.

  This widget represents the editor for items in the spot repair ListView. It contains
  a combobox for selecting the repair algorithm and a button to delete the repair spot.

  This widget is managed by ptRepairSpotItemDelegate.
*/

#ifndef PTREPAIRSPOTEDITOR_H
#define PTREPAIRSPOTEDITOR_H

#include <QWidget>
#include <QComboBox>
#include <QToolButton>

class ptRepairSpotEditor : public QWidget
{
    Q_OBJECT
public:
  /*! Creates an editor widget.
    \param Parent
      The editor’s parent widget, i.e. the item row in the spot repair ListView.
    \param InitialAlgoIndex
      Index of the current algorithm for the spot. Values correspond to the
      \c ptSpotRepairAlgo enum.
  */
  explicit ptRepairSpotEditor(QWidget *Parent,
                              const int InitialAlgoIndex);

  ~ptRepairSpotEditor();

  /*! A pointer to the \c QComboBox for seleting the repair algorithm. */
  QComboBox* AlgoCombo;

  /*! A pointer to the \c QToolButton used to delete the spot. */
  QToolButton* DelButton;

protected:
  bool eventFilter(QObject *obj, QEvent *event);

signals:
  /*! [SIGNAL] Emitted when the delete button is pressed. Actual deletion is done outside
    the editor because we need to destroy the editor object in the process.
  */
  void deleteButtonClicked();

};

#endif // PTREPAIRSPOTEDITOR_H
