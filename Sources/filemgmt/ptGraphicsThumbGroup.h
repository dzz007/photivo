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

#ifndef PTGRAPHICSTHUMBGROUP_H
#define PTGRAPHICSTHUMBGROUP_H

//==============================================================================

#include <QObject>
#include <QGraphicsRectItem>

#include "../ptConstants.h"

//==============================================================================

class ptGraphicsThumbGroup: public QGraphicsRectItem {

public:
  ptGraphicsThumbGroup(QGraphicsItem* parent = 0);
  void addItems(QGraphicsPixmapItem* pixmap,
                const QString fullPath,
                const QString description,
                const bool isDir);
  QString fullPath() { return m_FullPath; }
  int type() const { return Type; }

  enum { Type = UserType + 1 };


protected:
  bool sceneEvent(QEvent* event);


private:
  QString CutFileName(const QString FileName);

  QString   m_FullPath;
  bool      m_isDir;

  // These two objects don’t need to be destroyed explicitely in the destructor.
  // Because they are children that happens automatically.
  QGraphicsPixmapItem* m_Pixmap;
  QGraphicsTextItem*   m_InfoText;
};

//==============================================================================

#endif // PTGRAPHICSTHUMBGROUP_H
