////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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

#ifndef DLMAINWINDOW_H
#define DLMAINWINDOW_H

#include <QTimer>

#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>

#include "ui_ptMainWindow.h"

#include "ptInput.h"
#include "ptChoice.h"
#include "ptCheck.h"
#include "ptGroupBox.h"


////////////////////////////////////////////////////////////////////////////////
//
// ptMainWindow is the main gui element, showing all menus and controls.
//
////////////////////////////////////////////////////////////////////////////////

class ptMainWindow : public QMainWindow, public Ui::ptMainWindow {

Q_OBJECT

public:

// Constructor.
ptMainWindow(const QString         Title);
// Destructor.
~ptMainWindow();

// Get the current active tab expressed in pt*Tab from Constants.h.
// -1 on error.
short GetCurrentTab();

// Show or hide the toolboxes
void UpdateToolBoxes();

// Update some settings from the GuiSettings.
// (after a dcraw calculation for instance).
void UpdateSettings(void);

// Update the exif info gui element.
void UpdateExifInfo(Exiv2::ExifData ExifData);

// Make and model are remembered because in
// case of change the choice of white balances must be redone.
QString              m_CameraMake;
QString              m_CameraModel;

// Resize timer
QTimer* m_ResizeTimer;
// Event0 timer (create event at t=0)
QTimer* m_Event0Timer;

// ToolBoxes
QList<ptGroupBox *>*      m_ToolBoxes;
QIcon                     m_StatusIcon;
QList<QWidget *>          m_ActiveTabs;

// Desktop
QDesktopWidget* m_DesktopWidget;

QDockWidget* ControlsDockWidget;

void OnToolBoxesEnabledTriggered(const bool Enabled);

protected:
void closeEvent(QCloseEvent * Event);
void resizeEvent(QResizeEvent * Event);
void keyPressEvent(QKeyEvent * Event);
bool eventFilter(QObject *obj, QEvent *event);

private :
QTabBar* Tabbar;
QAction* m_AtnSavePipe;
QAction* m_AtnSaveFull;
QAction* m_AtnSaveSettings;
QAction* m_AtnSaveJobfile;

void AnalyzeToolBoxStructure();

private slots:
void ResizeTimerExpired();
void Event0TimerExpired();
void SaveMenuPipe();
void SaveMenuFull();
void SaveMenuSettings();
void SaveMenuJobfile();

// The generic catchall input change.
//~ void OnTagsEditTextChanged();

void OnInputChanged(QVariant Value);

void on_ProcessingTabBook_currentChanged(const int Index);

void OnToGimpButtonClicked();

void OnPreviewModeButtonClicked();

void OnRunButtonClicked();

void OnBackgroundColorButtonClicked();

void OnOpenFileButtonClicked();
void OnOpenSettingsFileButtonClicked();
void OnOpenPresetFileButtonClicked();
void OnSpotWBButtonClicked();

void OnZoomFitButtonClicked();
void OnZoomFullButtonClicked();
void OnFullScreenButtonClicked();
void OnLoadStyleButtonClicked();

void OnTabProcessingButtonClicked();
void OnTabSettingsButtonClicked();
void OnTabInfoButtonClicked();

void OnCameraColorProfileButtonClicked();
void OnPreviewColorProfileButtonClicked();
void OnOutputColorProfileButtonClicked();
void OnGimpExecCommandButtonClicked();

void OnStartupSettingsButtonClicked();

void OnMakeCropButtonClicked();

void OnChannelMixerOpenButtonClicked();
void OnChannelMixerSaveButtonClicked();

void OnCurveRGBOpenButtonClicked();
void OnCurveRGBSaveButtonClicked();
void OnCurveROpenButtonClicked();
void OnCurveRSaveButtonClicked();
void OnCurveGOpenButtonClicked();
void OnCurveGSaveButtonClicked();
void OnCurveBOpenButtonClicked();
void OnCurveBSaveButtonClicked();
void OnCurveLOpenButtonClicked();
void OnCurveLSaveButtonClicked();
void OnCurveaOpenButtonClicked();
void OnCurveaSaveButtonClicked();
void OnCurvebOpenButtonClicked();
void OnCurvebSaveButtonClicked();
void OnCurveLByHueOpenButtonClicked();
void OnCurveLByHueSaveButtonClicked();
void OnCurveTextureOpenButtonClicked();
void OnCurveTextureSaveButtonClicked();
void OnCurveShadowsHighlightsOpenButtonClicked();
void OnCurveShadowsHighlightsSaveButtonClicked();
void OnCurveDenoiseOpenButtonClicked();
void OnCurveDenoiseSaveButtonClicked();
void OnCurveSaturationOpenButtonClicked();
void OnCurveSaturationSaveButtonClicked();
void OnBaseCurveOpenButtonClicked();
void OnBaseCurveSaveButtonClicked();
void OnBaseCurve2OpenButtonClicked();
void OnBaseCurve2SaveButtonClicked();

void OnTone1ColorButtonClicked();
void OnTone2ColorButtonClicked();

void OnGradualOverlay1ColorButtonClicked();
void OnGradualOverlay2ColorButtonClicked();

void OnOutputColorProfileResetButtonClicked();
void OnWriteOutputButtonClicked();
void OnWritePipeButtonClicked();
};

#endif

////////////////////////////////////////////////////////////////////////////////
