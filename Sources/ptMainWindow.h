/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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
#include "ptVisibleToolsView.h"
#include "ptRepairSpotView.h"


////////////////////////////////////////////////////////////////////////////////
//
// ptMainWindow is the main gui element, showing all menus and controls.
//
////////////////////////////////////////////////////////////////////////////////

class ptMainWindow : public QMainWindow, public Ui::ptMainWindow {

Q_OBJECT


////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
////////////////////////////////////////////////////////////////////////////////

public:
  // Constructor.
  ptMainWindow(const QString         Title);
  // Destructor.
  ~ptMainWindow();

  // Populate Translations combobox
  void PopulateTranslationsCombobox(const QStringList UiLanguages, const int LangIdx);

  // Get the current active tab expressed in pt*Tab from Constants.h.
  // -1 on error.
  short GetCurrentTab();

  // Show or hide the toolboxes
  void UpdateToolBoxes();

  // Update some settings from the GuiSettings.
  // (after a dcraw calculation for instance).
  void UpdateSettings(void);

  // Update the file info GUI element on info tab
  void UpdateFilenameInfo(const QStringList FileNameList);

  // Update the exif info gui element.
  void UpdateExifInfo(Exiv2::ExifData ExifData);

  // Set visible/enabled states for crop tool widgets
  void UpdateCropToolUI();

  // Moved tools
  void ShowActiveTools();
  void ShowAllTools();
  void ShowFavouriteTools();
  void CleanUpMovedTools();

  // Set visible status for lensfun widgets
  void UpdateLfunDistUI();
  void UpdateLfunCAUI();
  void UpdateLfunVignetteUI();

  void UpdateLiquidRescaleUI();

  // Visible tools
  void ApplyVisibleTools();
  void UpdateVisibleTools();
  void LoadUISettings(const QString &fileName);
  void SaveUISettings(const QString &fileName) const;

  // Make and model are remembered because in
  // case of change the choice of white balances must be redone.
  QString              m_CameraMake;
  QString              m_CameraModel;

  // Resize timer
  QTimer* m_ResizeTimer;
  // Event0 timer (create event at t=0)
  QTimer* m_Event0Timer;
  QTimer* m_SearchInputTimer;

  // ToolBoxes
  QMap<QString, ptGroupBox*>* m_GroupBox;
  QList<QString>*             m_GroupBoxesOrdered;
  QList<QVBoxLayout*>*        m_TabLayouts;
  QList<ptGroupBox*>*         m_MovedTools;
  QIcon                       m_StatusIcon;
  QList<QWidget*>             m_ActiveTabs;

  // Desktop
  QDesktopWidget* m_DesktopWidget;

  QDockWidget* ControlsDockWidget;

  void OnToolBoxesEnabledTriggered(const bool Enabled);
public slots:
  void OtherInstanceMessage(const QString &msg);

////////////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
////////////////////////////////////////////////////////////////////////////////

protected:
  void closeEvent(QCloseEvent * Event);
  void resizeEvent(QResizeEvent * Event);
  void keyPressEvent(QKeyEvent * Event);
  void wheelEvent(QWheelEvent * Event);
  void dragEnterEvent(QDragEnterEvent* Event);
  void dropEvent(QDropEvent* Event);
  bool eventFilter(QObject *obj, QEvent *event);


////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
////////////////////////////////////////////////////////////////////////////////

private :
  QTabBar* Tabbar;
  QAction* m_AtnSavePipe;
  QAction* m_AtnSaveFull;
  QAction* m_AtnSaveSettings;
  QAction* m_AtnSaveJobfile;
  QAction* m_AtnGimpSavePipe;
  QAction* m_AtnGimpSaveFull;
  QAction* m_AtnMenuFullReset;
  QAction* m_AtnMenuUserReset;
  QAction* m_AtnMenuOpenPreset;
  QAction* m_AtnMenuOpenSettings;
  short    m_ContextMenuOnTab;
  QAction* m_AtnShowTools;

  ptVisibleToolsModel* m_VisibleToolsModel;
  ptRepairSpotModel* m_RepairSpotModel;

  void AnalyzeToolBoxStructure();
  void ShowMovedTools(const QString Title);
  void InitVisibleTools();
  void PopulateSpotRepairList();


////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
////////////////////////////////////////////////////////////////////////////////

private slots:
  void ResizeTimerExpired();
  void Event0TimerExpired();
  void SaveMenuPipe();
  void SaveMenuFull();
  void SaveMenuSettings();
  void SaveMenuJobfile();
  void GimpSaveMenuPipe();
  void GimpSaveMenuFull();
  void MenuFullReset();
  void MenuUserReset();
  void MenuOpenPreset();
  void MenuOpenSettings();
  void ShowToolsOnTab();
  void StartSearchTimer(QString);
  void Search();
  void OnTranslationChoiceChanged(int idx);

  // The generic catchall input change.
  //~ void OnTagsEditTextChanged();

  void OnInputChanged(QVariant Value);

  void on_ProcessingTabBook_currentChanged(const int Index);

  void OnToGimpButtonClicked();

  void OnPreviewModeButtonClicked();

  void OnSearchResetButtonClicked();
  void OnSearchActiveToolsButtonClicked();
  void OnSearchAllToolsButtonClicked();
  void OnSearchFavouriteToolsButtonClicked();

  void OnRunButtonClicked();
  void OnResetButtonClicked();

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

  void OnRotateLeftButtonClicked();
  void OnRotateRightButtonClicked();
  void OnRotateAngleButtonClicked();

  void OnMakeCropButtonClicked();
  void OnConfirmCropButtonClicked();
  void OnCancelCropButtonClicked();
  void OnCropOrientationButtonClicked();
  void OnCropCenterHorButtonClicked();
  void OnCropCenterVertButtonClicked();

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
  void OnCurveHueOpenButtonClicked();
  void OnCurveHueSaveButtonClicked();
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

  void OnTextureOverlayButtonClicked();
  void OnTextureOverlayClearButtonClicked();

  void OnGradualOverlay1ColorButtonClicked();
  void OnGradualOverlay2ColorButtonClicked();

  void OnOutputColorProfileResetButtonClicked();
  void OnWriteOutputButtonClicked();
  void OnWritePipeButtonClicked();

  void OnVisibleToolsDiscardButtonClicked();
  void OnVisibleToolsLoadButtonClicked();
  void OnVisibleToolsSaveButtonClicked();

};

#endif
