/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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
  \class ptImageSpotEditor

  \brief Editor widget for spot ListViews.

  This widget represents the editor for items in the spot ListView. It contains
  a QLineEdit for the spot’s name and a button to delete the spot.

  This widget is managed by ptImageSpotItemDelegate.
*/

#ifndef PTIMAGESPOTEDITOR_H
#define PTIMAGESPOTEDITOR_H

//==============================================================================

#include <QWidget>
#include <QLineEdit>
#include <QToolButton>

//==============================================================================

class ptImageSpotEditor: public QWidget {
Q_OBJECT

public:
  /*! Creates an editor widget.
    \param AParent
      The editor’s parent widget. Must be set to the item row in the spot ListView
      for proper positioning of the editor.
    \param AInitialName
      Current name of the spot.
  */
  explicit ptImageSpotEditor(QWidget        *AParent,
                             const QString  &AInitialName);
  ~ptImageSpotEditor();

  /*! A pointer to the \c QToolButton used to delete the spot. */
  QToolButton *DelButton;

  /*! A pointer to the \c QLineEdit for editing the spot’s name. */
  QLineEdit *NameEditor;

//------------------------------------------------------------------------------

protected:
  bool eventFilter(QObject *obj, QEvent *event);

//------------------------------------------------------------------------------

signals:
  /*! [SIGNAL] Emitted when the delete button is pressed. Actual deletion is done outside
    the editor because we need to destroy the editor object in the process.
  */
  void deleteButtonClicked();


};
#endif // PTIMAGESPOTEDITOR_H
