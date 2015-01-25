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

#include "ptMessageBox.h"
#include "ptTheme.h"

#include <QMessageBox>

extern ptTheme* Theme;


ptMessageBox::ptMessageBox(QWidget *parent)
  : QMessageBox(parent)
{
  if (Theme != NULL) {
    this->setPalette(Theme->systemPalette());
  }
}

ptMessageBox::ptMessageBox(Icon icon, const QString &title, const QString &text,
                           StandardButtons buttons, QWidget *parent,
                           Qt::WindowFlags flags)
  : QMessageBox(icon, title, text, buttons, parent, flags)
{
  if (Theme != NULL) {
    this->setPalette(Theme->systemPalette());
  }
}


QMessageBox::StandardButton ptMessageBox::information(QWidget *parent, const QString &title,
     const QString &text, StandardButtons buttons,
     StandardButton defaultButton)
{
  QMessageBox* msgBox = new QMessageBox(Information, title, text, buttons, parent);
  if (Theme != NULL) {
    msgBox->setPalette(Theme->systemPalette());
  }
  msgBox->setDefaultButton(defaultButton);
  int result = msgBox->exec();
  delete msgBox;
  return (QMessageBox::StandardButton)result;
}

QMessageBox::StandardButton ptMessageBox::question(QWidget *parent, const QString &title,
     const QString &text, StandardButtons buttons,
     StandardButton defaultButton)
{
  QMessageBox* msgBox = new QMessageBox(Question, title, text, buttons, parent);
  if (Theme != NULL) {
    msgBox->setPalette(Theme->systemPalette());
  }
  msgBox->setDefaultButton(defaultButton);
  int result = msgBox->exec();
  delete msgBox;
  return (QMessageBox::StandardButton)result;
}

QMessageBox::StandardButton ptMessageBox::warning(QWidget *parent, const QString &title,
     const QString &text, StandardButtons buttons,
     StandardButton defaultButton)
{
  QMessageBox* msgBox = new QMessageBox(Warning, title, text, buttons, parent);
  if (Theme != NULL) {
    msgBox->setPalette(Theme->systemPalette());
  }
  msgBox->setDefaultButton(defaultButton);
  int result = msgBox->exec();
  delete msgBox;
  return (QMessageBox::StandardButton)result;
}

QMessageBox::StandardButton ptMessageBox::critical(QWidget *parent, const QString &title,
     const QString &text, StandardButtons buttons,
     StandardButton defaultButton)
{
  QMessageBox* msgBox = new QMessageBox(Critical, title, text, buttons, parent);
  if (Theme != NULL) {
    msgBox->setPalette(Theme->systemPalette());
  }
  msgBox->setDefaultButton(defaultButton);
  int result = msgBox->exec();
  delete msgBox;
  return (QMessageBox::StandardButton)result;
}
