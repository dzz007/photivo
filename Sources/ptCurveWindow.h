/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010-2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012-2013 Bernd Schoeler <brjohn@brother-john.net>
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
#ifndef PTCURVEWINDOW_H
#define PTCURVEWINDOW_H

#include "ptImage8.h"
#include "ptWidget.h"
#include "ptCurve.h"
#include <QPixmap>
#include <memory>
#include <vector>
#include <utility>

class ptFilterBase;
class QLabel;
class QActionGroup;

typedef std::pair<int, int>        TScreenAnchor;
typedef std::vector<TScreenAnchor> TScreenAnchorList;

class ptCurveWindow: public ptWidget {
Q_OBJECT
public:
  explicit ptCurveWindow(QWidget *AParent);
  ptCurveWindow(const ptCfgItem &ACfgItem, QWidget *AParent);
  ~ptCurveWindow();

  void init(const ptCfgItem &ACfgItem);
  void setValue(const QVariant &AValue);
  void setCaption(const QString &ACaption);
  void updateView();
  void updateView(const std::shared_ptr<ptCurve> ANewCurve);

protected:
  void    mousePressEvent(QMouseEvent *AEvent);
  void    mouseReleaseEvent(QMouseEvent *);
  void    mouseMoveEvent(QMouseEvent *AEvent);
  void    changeEvent(QEvent* Event);
  void    paintEvent(QPaintEvent*);
  void    resizeEvent(QResizeEvent*);

  QSize   sizeHint()                const { return QSize(100, 100); }
  QSize   minimumSizeHint()         const { return QSize(100, 100); }
  int     heightForWidth(int width) const { return width; }

private:
  enum TUserAction { NoAction, InsertAction, DeleteAction, DragAction, WheelAction };

private:
  void          calcCurveImage();
  void          setBWGradient(ptImage8* AImage);
  void          setBWGammaGradient(ptImage8* AImage);
  void          setColorGradient(ptImage8* AImage);
  void          setColorBlocks(const QColor &ATopLeftColor, const QColor &ABottomRightColor);
  TAnchor       clampMovingAnchor(const TAnchor &APoint, const QPoint &AMousePos);
  int           hasCaughtAnchor(const QPoint APos);
  inline bool   isCyclicCurve();
  void          requestPipeRun();

  ptImage8                  FCanvas;
  QLabel                   *FCaptionLabel;
  std::shared_ptr<ptCurve>  FCurve;
  TScreenAnchorList         FDisplayAnchors;
  QPixmap                   FDisplayImage;
  QTimer                   *FWheelTimer;
  TUserAction               FMouseAction;
  int                       FMovingAnchor;

  // context menu actions
  void createMenuActions();
  void execContextMenu(const QPoint APos);
  QAction*            FLinearIpolAction;
  QAction*            FSplineIpolAction;
  QAction*            FCosineIpolAction;
  QActionGroup*       FIpolGroup;
  QAction*            FByLumaAction;
  QAction*            FByChromaAction;
  QActionGroup*       FMaskGroup;
  QAction*            FOpenCurveAction;

private slots:
  void wheelTimerExpired();

  // context menu slots
  void setMaskType();
  void setInterpolationType();
  void openCurveFile();

};

#endif // PTCURVEWINDOW_H
