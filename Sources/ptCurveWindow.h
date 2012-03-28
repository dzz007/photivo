/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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
  \class ptCurveWindow

  \brief ptCurveWindow is a Gui element showing a curve.
*/

#ifndef PTCURVEWINDOW_H
#define PTCURVEWINDOW_H

//==============================================================================

#include <QtGui>

#include "ptImage8.h"
#include "ptCurve.h"
#include "ptConstants.h"

//==============================================================================

class ptCurveWindow : public QWidget {
Q_OBJECT

public:
  ptCurveWindow(ptCurve*      ARelatedCurve,
                const short   AChannel,
                QWidget*      AParent,
                const QString &ACaption = "");
  ~ptCurveWindow();

  /*! NewRelatedCurve to associate anonter ptImage with this window.*/
  void UpdateView(ptCurve* NewRelatedCurve = NULL);

  /*! Calculate the curve into an Image8 */
  void CalculateCurve();

  void ContextMenu(QMouseEvent* event);

  ptCurve*       RelatedCurve;
  QTimer*        ResizeTimer; // To circumvent multi resize events.
  short          Channel;   // Beware, historical name. Read: “curve type”
  ptImage8*      Image8;

//------------------------------------------------------------------------------

protected:
  void changeEvent(QEvent* Event);
  void resizeEvent(QResizeEvent*);
  void paintEvent(QPaintEvent*);
  void mousePressEvent(QMouseEvent *Event);
  void wheelEvent(QWheelEvent *Event);
  void mouseMoveEvent(QMouseEvent *Event);
  void mouseReleaseEvent(QMouseEvent *Event);
  QSize sizeHint() const { return QSize(100,100); }
  QSize minimumSizeHint() const { return QSize(100,100); }
  int  heightForWidth(int w) const { return w;}

//------------------------------------------------------------------------------

private:
  void UpdateCurve();
  void SetBWGradient(ptImage8* Image);
  void SetBWGammaGradient(ptImage8* Image);
  void SetColorGradient(ptImage8* Image);
  void SetCurveState(const short state);
  short GetCurveState();


  QLabel*             FCaptionLabel;
  QTimer*             FWheelTimer;
  short               FXSpot[ptMaxAnchors];
  short               FYSpot[ptMaxAnchors];
  QPixmap*            FQPixmap;
  int32_t             FOverlayAnchorX;
  int32_t             FOverlayAnchorY;
  short               FMovingAnchor;
  short               FActiveAnchor;  // gets the wheel event, -1 neutral
  uint16_t            FMousePosX;
  uint16_t            FMousePosY;
  short               FBlockEvents;
  short               FRecalcNeeded;
  short               FCyclicCurve;
  // Saturaton Curve modes
  QAction*            FAtnAbsolute;
  QAction*            FAtnAdaptive;
  QActionGroup*       FSatModeGroup;
  QAction*            FAtnByLuma;
  QAction*            FAtnByChroma;
  QActionGroup*       FTypeGroup;
  // Interpolation Type
  QAction*            FAtnITLinear;
  QAction*            FAtnITSpline;
  QAction*            FAtnITCosine;
  QActionGroup*       FITGroup;

//------------------------------------------------------------------------------

private slots:
  void ResizeTimerExpired();
  void WheelTimerExpired();
  void SetSatMode();
  void SetType();
  void SetInterpolationType();


};
#endif // PTCURVEWINDOW_H
