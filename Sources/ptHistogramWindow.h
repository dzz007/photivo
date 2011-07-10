/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

#ifndef DLHISTOGRAMWINDOW_H
#define DLHISTOGRAMWINDOW_H

#include <QtGui>

#include "ptImage.h"
#include "ptImage8.h"

////////////////////////////////////////////////////////////////////////////////
//
// ptHistogramWindow is a Gui element showing a histogram.
//
////////////////////////////////////////////////////////////////////////////////

class ptHistogramWindow : public QWidget {

Q_OBJECT

public :

const ptImage*      m_RelatedImage;
QTimer*             m_ResizeTimer; // To circumvent multi resize events.

// Constructor.
ptHistogramWindow(const ptImage* RelatedImage,
                        QWidget* Parent);
// Destructor.
~ptHistogramWindow();

// NewRelatedImage to associate anonter ptImage with this window.
void UpdateView(const ptImage* NewRelatedImage = NULL);
void FillLookUp();

protected:
void resizeEvent(QResizeEvent*);
void paintEvent(QPaintEvent*);
QSize sizeHint() const { return QSize(200,200); };
QSize minimumSizeHint() const { return QSize(100,100); };
int  heightForWidth(int w) const { return MIN(150,w);};
void contextMenuEvent(QContextMenuEvent *event);

private slots:
void ResizeTimerExpired();
void MenuLnX();
void MenuLnY();
void MenuCrop();
void MenuChannel();
void MenuMode();

private:
const ptImage8* m_Image8;
QPixmap*        m_QPixmap;
short           m_LogoActive;
short           m_RecalcNeeded;
uint32_t        m_HistoMax;
short           m_PreviousHistogramGamma;
short           m_PreviousHistogramLogX;
short           m_PreviousHistogramLogY;
QString         m_PreviousFileName;
QAction*        m_AtnLnX;
QAction*        m_AtnLnY;
QAction*        m_AtnCrop;
QActionGroup*   m_ModeGroup;
QAction*        m_AtnLinear;
QAction*        m_AtnPreview;
QAction*        m_AtnOutput;
QActionGroup*   m_ChannelGroup;
QAction*        m_AtnRGB;
QAction*        m_AtnR;
QAction*        m_AtnG;
QAction*        m_AtnB;
uint16_t*       m_LookUp;

void CalculateHistogram();
};

#endif

////////////////////////////////////////////////////////////////////////////////
