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

#ifndef PTDIRLISTVIEW_H
#define PTDIRLISTVIEW_H

//==============================================================================

#include <QListView>
#include <QFileSystemModel>

//==============================================================================

class ptDirList: public QListView {
Q_OBJECT

public:
  explicit ptDirList(QWidget* parent = NULL);

  QFileSystemModel* model() const { return m_Model; }
  void setModel(QFileSystemModel* model);


protected:


private:
  QFileSystemModel* m_Model;


signals:


public slots:


//==============================================================================
};
#endif // PTDIRLISTVIEW_H
