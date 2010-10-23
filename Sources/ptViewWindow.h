////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender
// Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

#ifndef DLVIEWWINDOW_H
#define DLVIEWWINDOW_H

#include <QtGui>

#include "ptImage.h"


class ptViewWindow : public QAbstractScrollArea {

Q_OBJECT

public :

// Constructor.
ptViewWindow(const ptImage*       RelatedImage,
                   QWidget*       Parent);
// Destructor.
~ptViewWindow();

// NewRelatedImage to associate anonter ptImage with this window.
void UpdateView(const ptImage* NewRelatedImage = NULL);


// Allow to select in the image. (push/drag/release events).
// Argument is 0 or 1.
// If FixedAspectRatio, then the selection box has the HOverW ratio.
void AllowSelection(const short  Allow,
                    const short  FixedAspectRatio = 0,
                    const double HOverW = 2.0/3,
        const short  RectangleMode = ptRectangleMode_None);

// Returns 1 if selection process is ongoing.
short SelectionOngoing();

// Results of selection.
// Expressed in terms of the RelatedImage.
uint16_t GetSelectionX();
uint16_t GetSelectionY();
uint16_t GetSelectionWidth();
uint16_t GetSelectionHeight();

// Grid
void Grid(const short Enabled, const short GridX, const short GridY);

// Zoom functions. Fit returns the factor in %.
short ZoomFit();
void  Zoom(const short Factor); // Expressed in %

// Status report
void StatusReport (short State);

const ptImage*       m_RelatedImage;

short                m_SelectionAllowed;
short                m_SelectionOngoing;
int16_t              m_StartDragX;
int16_t              m_StartDragY;
int16_t              m_EndDragX;
int16_t              m_EndDragY;
short                m_FixedAspectRatio;
double               m_HOverW;
uint16_t             m_StartX; // Offset of the shown part into the image.
uint16_t             m_StartY;
uint16_t             m_XOffsetInVP; // For images smaller than viewport
uint16_t             m_YOffsetInVP;
double               m_ZoomFactor;

// Order reflects also order into the pipe :
// The original->Zoom->Cut (to visible) ->pixmap for acceleration.
QImage*              m_QImage;
QImage*              m_QImageZoomed;
QImage*              m_QImageCut;

protected:
// overloaded virtual ones.
bool  viewportEvent(QEvent*);
void  scrollContentsBy ( int dx, int dy );

private slots:
void MenuExpIndicate();
void MenuExpIndOver();
void MenuExpIndUnder();
void MenuExpIndR();
void MenuExpIndG();
void MenuExpIndB();
void MenuExpIndSensor();
void MenuShowBottom();
void MenuShowTools();
void MenuFullScreen();
void MenuZoomFit();
void MenuZoom100();
void MenuMode();
void SizeReportTimerExpired();
void StatusReportTimerExpired();
void ResizeTimerExpired();

private:
void        RecalculateCut();
void        ContextMenu(QEvent* Event);

uint16_t    m_ZoomWidth;
uint16_t    m_ZoomHeight;
double      m_PreviousZoomFactor;
short       m_Grid;
short       m_GridX;
short       m_GridY;
short       m_DrawRectangle;
short       m_RectangleMode;

QAction*    m_AtnExpIndicate;
QAction*    m_AtnExpIndR;
QAction*    m_AtnExpIndG;
QAction*    m_AtnExpIndB;
QAction*    m_AtnExpIndOver;
QAction*    m_AtnExpIndUnder;
QAction*    m_AtnExpIndSensor;
QAction*    m_AtnShowBottom;
QAction*    m_AtnShowTools;
QAction*    m_AtnZoomFit;
QAction*    m_AtnZoom100;
QAction*    m_AtnModeRGB;
QAction*    m_AtnModeL;
QAction*    m_AtnModeA;
QAction*    m_AtnModeB;
QAction*    m_AtnModeGradient;
QAction*    m_AtnModeStructure;
QActionGroup*   m_ModeGroup;

QLabel*     m_SizeReport;
QString     m_SizeReportText;
int         m_SizeReportTimeOut;
QTimer*     m_SizeReportTimer;
QLabel*     m_StatusReport;
int         m_StatusReportTimeOut;
QTimer*     m_StatusReportTimer;
int         m_ResizeTimeOut;
QTimer*     m_ResizeTimer;
int         m_NewSize;

public:
QAction*    m_AtnFullScreen;
};

#endif

////////////////////////////////////////////////////////////////////////////////
