/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#ifndef PTIMAGEVIEW_H
#define PTIMAGEVIEW_H

//==============================================================================

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QThread>

#include "ptFileMgrDM.h"
#include "../ptReportOverlay.h"

//==============================================================================

class MyWorker;

//==============================================================================

class ptImageView : public QGraphicsView
{
  Q_OBJECT
  public:
    /*! Creates a \c ptImageView instance.
      \param parent
        The image view’s parent widget.
    */
    explicit ptImageView(QWidget *parent = 0, ptFileMgrDM* DataModule = 0);
    ~ptImageView();

    void Display(const QString FileName);
  signals:

  public slots:

    void resizeEvent(QResizeEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);

  private:
    void ZoomTo(float factor);  // 1.0 means 100%
    int  ZoomToFit(const short withMsg = 1);  // fit complete image into viewport

    /*! This function performs the actual thumbnail generation. */
    void updateView();

  private slots:
    void startWorker();

  private:
    ptFileMgrDM*          m_DataModule;
    const float           MinZoom;
    const float           MaxZoom;
    QList<float>          ZoomFactors;   // steps for wheel zoom
    QGridLayout*          m_parentLayout;
    QGraphicsScene*       m_Scene;
    QGraphicsPixmapItem*  m_PixmapItem;
    QString               m_FileName;
    int                   m_ZoomMode;
    float                 m_ZoomFactor;
    int                   m_Zoom;
    QLine*                m_DragDelta;
    bool                  m_LeftMousePressed;
    ptReportOverlay*      m_ZoomSizeOverlay;
    bool                  m_NeedRun;
    MyWorker*             m_Worker;
};

//==============================================================================

typedef void (ptImageView::*updateView_ptr)();

class MyWorker: public QThread {
  public:
    void run();

    updateView_ptr myFct;
    ptImageView*   my_ImageView;
};

#endif // PTIMAGEVIEW_H
