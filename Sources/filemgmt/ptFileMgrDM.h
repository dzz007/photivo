/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILEMGRDM_h
#define PTFILEMGRDM_h

#include "ptGraphicsThumbGroup.h"
#include "ptSingleDirModel.h"
#include "ptTagModel.h"
#include "ptThumbGen.h"
#include <QObject>
#include <QPixmap>
#include <QFileSystemModel>
#include <QGraphicsItemGroup>
#include <QList>
#include <QHash>
#include <wand/magick_wand.h>

class ptImage8;

//------------------------------------------------------------------------------
/*!
  \class ptFileMgrDM

  \brief Data module for file management.

  This data module will handle the thumbnail creation and manage the corresponding
  memory.
*/
class ptFileMgrDM: public QObject {
Q_OBJECT

public:
  ptFileMgrDM(QObject* AParent = nullptr);
  ~ptFileMgrDM();

  void clear();

  QString currentDir();
  void setCurrentDir(const QString& AAbsolutePath);
  int setThumDir(const QString& AAbsolutePath);
  int focusedThumb();
  int focusedThumb(QGraphicsItem* group);
  ptGraphicsThumbGroup* moveFocus(const int index);

  void abortThumbGen();
  void connectThumbGen(const QObject* AReceiver, const char* ABroadcastSlot);
  void populateThumbs(QGraphicsScene* AScene);
  QList<ptGraphicsThumbGroup*>* thumbGroupList();
  bool thumbGenRunning() const;

  ptSingleDirModel* dirModel() const;
  ptTagModel* tagModel();

private:
  void createThumbGroup(const QFileInfo &AFileInfo, uint AId, QGraphicsScene* AScene);

  int                           FFocusedThumb;
  QDir                          FCurrentDir;
  ptSingleDirModel*             FDirModel;
  bool                          FIsMyComputer;
  ptTagModel*                   FTagModel;
  std::unique_ptr<ptThumbGen>   FThumbGen;
  QList<ptGraphicsThumbGroup*>* FThumbGroupList;

};

//==============================================================================

#endif // PTFILEMGRDM_h
