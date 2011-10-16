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
#include <QPen>

#include "../ptConstants.h"

//==============================================================================

class ptGraphicsThumbGroup: public QGraphicsRectItem {

public:
  /*! Creates a \c ptGraphicsThumbGroup object. */
  ptGraphicsThumbGroup(QGraphicsItem* parent = 0);

  /*! Adds informative items to the thumbnail group.
    \param fullPath
      The path to the file or directory that this thumbnail group refers to.
    \param description
      The text appearing below the thumbnail image.
    \param isDir
      Defines if this thumbnail group refers to a directory.
    \param
      Defines if this thumbnail group refers to the parent directory.
      When you set this to \c true also set \c isDir to \c true.
  */
  void addInfoItems(const QString fullPath,
                    const QString description,
                    const ptFSOType fsoType);

  /*! Adds the thumbnail image to the thumbnail group. Warning: \c addImage() deletes \c image.
      \param pixmap
        A pointer to the \c QPixmap thumnail image.
  */
  void addImage(QImage* image);

  /*! Return the type of file system object this thumbnail group refers to. */
  ptFSOType fsoType() { return m_FSOType; }

  /*! Returns the full path of the file or directory this thumbnail group refers to. */
  QString fullPath() { return m_FullPath; }

  /*! Returns \c true if a pixmap is part of the thumbnail group. */
  bool hasPixmap() { return m_Pixmap != NULL; }

  /*! Paints the thumbnail group.
    Reimplements \c QGraphicsRectItem::paint()
  */
  void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);


  /*! Returns that type that identifies \c ptGraphicsThumbGroup objects
      within the graphics view framework.
  */
  int type() const { return Type; }

  /*! Defines the padding in pixels between the items in the thumbnail group and the group’s border. */
  static const int InnerPadding = 8;

  /*! Defines the ID that identifies \c ptGraphicsThumbGroup objects
      within the graphics view framework.
  */
  enum { Type = UserType + 1 };


protected:
  /*! Event handler for the thumbnail group.
      Reimplements \c QGraphicsRectItem::sceneEvent().
  */
  bool sceneEvent(QEvent* event);


private:
  QString   m_FullPath;
  ptFSOType m_FSOType;

  // Following objects don’t need to be destroyed explicitely in the destructor.
  // Because they are children that happens automatically.
  QGraphicsPixmapItem*      m_Pixmap;
  QGraphicsSimpleTextItem*  m_ImgTypeText;
  QGraphicsSimpleTextItem*  m_InfoText;
};

//==============================================================================

#endif // PTGRAPHICSTHUMBGROUP_H
