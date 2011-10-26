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
/**
** Wrapper around QMessageBox to ensure all message boxes have the system's
** palette. Avoids partly styled and possibly illegible dialogs.
**
** Always use ptMessageBox instead of QMessageBox!
**/

#ifndef PTMESSAGEBOX_H
#define PTMESSAGEBOX_H

#include <QMessageBox>

class ptMessageBox: public QMessageBox {
Q_OBJECT

public:
  ptMessageBox(QWidget *parent = 0);
  ptMessageBox(Icon icon, const QString &title, const QString &text,
              StandardButtons buttons = NoButton, QWidget *parent = 0,
              Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

  static StandardButton information(QWidget *parent, const QString &title,
       const QString &text, StandardButtons buttons = Ok,
       StandardButton defaultButton = NoButton);
  static StandardButton question(QWidget *parent, const QString &title,
       const QString &text, StandardButtons buttons = Ok,
       StandardButton defaultButton = NoButton);
  static StandardButton warning(QWidget *parent, const QString &title,
       const QString &text, StandardButtons buttons = Ok,
       StandardButton defaultButton = NoButton);
  static StandardButton critical(QWidget *parent, const QString &title,
       const QString &text, StandardButtons buttons = Ok,
       StandardButton defaultButton = NoButton);
};

#endif
