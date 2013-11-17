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

#ifndef PTSINGLEDIRMODEL_H
#define PTSINGLEDIRMODEL_H

//==============================================================================

#include "../ptConstants.h"

#include <QStandardItemModel>
#include <QDir>
#include <QStringList>
#include <QString>

//==============================================================================

class ptSingleDirModel: public QStandardItemModel {
Q_OBJECT

public:
  explicit ptSingleDirModel(QObject* parent = NULL);
  ~ptSingleDirModel();

  QString absolutePath();
  void ChangeAbsoluteDir(const QString& path);
  void ChangeDir(const QModelIndex& index);
  ptFSOType pathType() const;

private:
  void UpdateModel();

  QDir*           m_CurrentDir;
  QStringList     m_EntryList;


#ifdef Q_OS_WIN
/*
  Windows needs special handling. We need a parent above the level of drives to make
  changing of drives possible from the directory ListView.
  In the Windows virtual file system hierarchy “My Computer” is that parent (basically
  the same as / on *nix), but Qt has no built-in support for it. So we extend the
  model ourselfs to add support for "My Computer".
*/
private:
  void FillListWithDrives();
  ptFSOType m_CurrentDirType;
#endif // Q_OS_WIN


//==============================================================================
};
#endif // PTSINGLEDIRMODEL_H
