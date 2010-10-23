////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DLCURVEWINDOW_H
#define DLCURVEWINDOW_H

#include <QtGui>

#include "ptImage8.h"
#include "ptCurve.h"
#include "ptConstants.h"

////////////////////////////////////////////////////////////////////////////////
//
// ptCurveWindow is a Gui element showing a curve.
//
////////////////////////////////////////////////////////////////////////////////

class ptCurveWindow : public QWidget {

Q_OBJECT

public :

QWidget*       m_Parent;
ptCurve*       m_RelatedCurve;
QTimer*        m_ResizeTimer; // To circumvent multi resize events.
short          m_Channel;
ptImage8*      m_Image8;

// Constructor.
ptCurveWindow(ptCurve*     RelatedCurve,
              const short  Channel,   // RGB,R,G,B,L
              QWidget*     Parent);
// Destructor.
~ptCurveWindow();

// NewRelatedCurve to associate anonter ptImage with this window.
void UpdateView(ptCurve* NewRelatedCurve = NULL);

// Calculate the curve into an Image8
void CalculateCurve();

void ContextMenu(QMouseEvent* event);

protected:
void resizeEvent(QResizeEvent*);
void paintEvent(QPaintEvent*);
void mousePressEvent(QMouseEvent *Event);
void mouseMoveEvent(QMouseEvent *Event);
void mouseReleaseEvent(QMouseEvent *Event);
QSize sizeHint() const { return QSize(100,100); };
QSize minimumSizeHint() const { return QSize(100,100); };
int  heightForWidth(int w) const { return w;};

private slots:
void ResizeTimerExpired();
void SetSatMode();
void SetType();
void SetInterpolationType();

private:
void UpdateCurve();
void SetCurveState(const short state);
short GetCurveState();

short               m_XSpot[ptMaxAnchors];
short               m_YSpot[ptMaxAnchors];
QPixmap*            m_QPixmap;
int32_t             m_OverlayAnchorX;
int32_t             m_OverlayAnchorY;
short               m_MovingAnchor;
short               m_BlockEvents;
short               m_RecalcNeeded;
// Saturaton Curve modes
QAction*            m_AtnAbsolute;
QAction*            m_AtnAdaptive;
QActionGroup*       m_SatModeGroup;
QAction*            m_AtnByLuma;
QAction*            m_AtnByChroma;
QActionGroup*       m_TypeGroup;
// Interpolation Type
QAction*            m_AtnITLinear;
QAction*            m_AtnITSpline;
QActionGroup*       m_ITGroup;
};

#endif

////////////////////////////////////////////////////////////////////////////////
