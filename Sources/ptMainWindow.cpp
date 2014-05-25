/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
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

#include <iomanip>
#include <iostream>

#include "ptDefines.h"
#include "ptChannelMixer.h"
#include "ptConfirmRequest.h"
#include "ptConstants.h"
#include "ptError.h"
#include "ptGuiOptions.h"
//#include "ptLensfun.h"    // TODO BJ: implement lensfun DB
#include "ptMainWindow.h"
#include "ptMessageBox.h"
#include "ptSettings.h"
#include "ptTheme.h"
#include "ptViewWindow.h"
#include "ptWhiteBalances.h"

#include "filters/ptFilterDM.h"
#include "filters/ptFilterBase.h"
#include "ptToolBox.h"

#ifdef Q_OS_WIN
  #include "ptEcWin7.h"
#endif

#include <QDesktopWidget>
#include <QFileDialog>

#include <cassert>

using namespace std;

extern ptTheme* Theme;
extern ptViewWindow* ViewWindow;
extern QString ImageFileToOpen;
extern QString PtsFileToOpen;
extern short ImageCleanUp;

void CB_MenuFileOpen(const short HaveFile);
void CB_OpenSettingsFile(QString SettingsFileName);
void CB_OpenFileButton();
void CB_ZoomStep(int direction);
void ReadSidecar(const QString& Sidecar);

// ATZ
void ptAddUndo();
void ptMakeUndo();
void ptMakeRedo();
void ptClearUndoRedo();
void ptMakeFullUndo();
void ptResetSettingsToDefault();
void ptCopySettingsToClipboard();
void ptPasteSettingsFromClipboard();
void ptSwitchAB();
// end ATZ


////////////////////////////////////////////////////////////////////////////////
//
// Here follow a bunch of macro's to avoid repetitive code for lots of
// the Gui elements.
//
////////////////////////////////////////////////////////////////////////////////

// Connect a menu action.

// Must be bool for the checkable menu items !
#define Macro_ConnectSomeMenu(Some)                                    \
  connect( action ## Some,SIGNAL(triggered(bool)),                     \
          this,SLOT(On ## Some ## Triggered(bool)));

// Connect a button.

#define Macro_ConnectSomeButton(Some)                                  \
  connect(Some ## Button,SIGNAL(clicked()),                            \
          this,SLOT(On ## Some ## ButtonClicked()));

// Callback into main for a menu item activated.

#define Macro_OnSomeMenuActivated(Some)                           \
void CB_Menu ## Some(const short Enabled);                        \
void ptMainWindow::On ## Some ## Triggered(const bool  Enabled) { \
  ::CB_Menu ## Some(Enabled);                                     \
}

// Leftover from removing the menu
void CB_MenuFileExit(const short);

// Prototype
void Update(short Phase,
            short SubPhase      = -1,
            short WithIdentify  = 1,
            short ProcessorMode = ptProcessorMode_Preview);

void Update(const QString GuiName);

//==============================================================================

ptMainWindow::ptMainWindow(const QString Title)
: QMainWindow(NULL)
{
  FUIState = uisNone;

  // Setup from the Gui builder.
  setupUi(this);
  setWindowTitle(Title);

  // Initialize Win 7 taskbar features
  #ifdef Q_OS_WIN
    ptEcWin7::CreateInstance(this);
  #endif

  // Move the fullscreen button in the zoom bar a bit to the left
  // to make space for the Mac window resize handle
  #ifdef Q_OS_MAC
    MacSpacer->setFixedWidth(16);
  #endif

  MainSplitter->setStretchFactor(1,1);
  ViewSplitter->setStretchFactor(0,1);

  // Some corrections that often go wrong in the manual
  // layout using the designer.

  QList <QVBoxLayout *> VBoxes = MainTabBook->findChildren <QVBoxLayout *> ();
  for (short i=0; i<VBoxes.size(); i++) {
    VBoxes[i]->setMargin(2);
    VBoxes[i]->setContentsMargins(2,2,2,2);
  }

  // Initial value. We remember locally for the case that a
  // a new photograph is from another camera. This is then detected
  // for other whitebalances.
  m_CameraMake  = Settings->GetString("CameraMake");
  m_CameraModel = Settings->GetString("CameraModel");

  // Go and construct all the input,choice .. gui elements.
  const QStringList Keys = Settings->GetKeys();
  for (int i=0; i<Keys.size(); i++) {
    const QString Key = Keys[i];
    //printf("(%s,%d) '%s'\n",__FILE__,__LINE__,Key.toLocal8Bit().data());
    switch (Settings->GetGuiType(Key)) {

      case ptGT_InputSlider :
      case ptGT_Input       :
        //printf("(%s,%d) Creating Input for '%s'\n",
        //       __FILE__,__LINE__,Key.toLocal8Bit().data());
        {
        QString ParentName = Key + "Widget";
        QString ObjectName = Key + "Input";
        ptInput* Input =
          new ptInput(this,
                      ObjectName,
                      ParentName,
                      Settings->GetGuiType(Key) == ptGT_InputSlider,
                      0,
                      Settings->GetHasDefaultValue(Key),
                      Settings->GetDefaultValue(Key),
                      Settings->GetMinimumValue(Key),
                      Settings->GetMaximumValue(Key),
                      Settings->GetStep(Key),
                      Settings->GetNrDecimals(Key),
                      Settings->GetLabel(Key),
                      Settings->GetToolTip(Key),
                      ptTimeout_Input);
        connect(Input,SIGNAL(valueChanged(QVariant)),
                this,SLOT(OnInputChanged(QVariant)));
        //m_GuiInputs[ObjectName] = Input;
        Settings->SetGuiInput(Key,Input);
        }
        break;
      case ptGT_InputSliderHue :
        //printf("(%s,%d) Creating Input for '%s'\n",
        //       __FILE__,__LINE__,Key.toLocal8Bit().data());
        {
        QString ParentName = Key + "Widget";
        QString ObjectName = Key + "Input";
        ptInput* Input =
          new ptInput(this,
                      ObjectName,
                      ParentName,
                      1,
                      1,
                      Settings->GetHasDefaultValue(Key),
                      Settings->GetDefaultValue(Key),
                      Settings->GetMinimumValue(Key),
                      Settings->GetMaximumValue(Key),
                      Settings->GetStep(Key),
                      Settings->GetNrDecimals(Key),
                      Settings->GetLabel(Key),
                      Settings->GetToolTip(Key),
                      ptTimeout_Input);
        connect(Input,SIGNAL(valueChanged(QVariant)),
                this,SLOT(OnInputChanged(QVariant)));
        //m_GuiInputs[ObjectName] = Input;
        Settings->SetGuiInput(Key,Input);
        }
        break;
      case ptGT_Choice :
        //printf("(%s,%d) Creating Choice for '%s'\n",
        //       __FILE__,__LINE__,Key.toLocal8Bit().data());
        {
        QString ParentName = Key + "Widget";
        QString ObjectName = Key + "Choice";
        ptChoice* Choice =
          new ptChoice(this,
                       ObjectName,
                       ParentName,
                       Settings->GetHasDefaultValue(Key),
                       Settings->GetDefaultValue(Key),
                       Settings->GetInitialOptions(Key),
                       Settings->GetToolTip(Key),
                       ptTimeout_Input);
        connect(Choice,SIGNAL(valueChanged(QVariant)),
                this,SLOT(OnInputChanged(QVariant)));
        //m_GuiChoices[ObjectName] = Choice;
        Settings->SetGuiChoice(Key,Choice);
        }
        break;

      case ptGT_Check :
        //printf("(%s,%d) Creating Check for '%s'\n",
        //       __FILE__,__LINE__,Key.toLocal8Bit().data());
        {
        QString ParentName = Key + "Widget";
        QString ObjectName = Key + "Check";
        ptCheck* Check =
          new ptCheck(this,
                      ObjectName,
                      ParentName,
                      Settings->GetDefaultValue(Key),
                      Settings->GetLabel(Key),
                      Settings->GetToolTip(Key));
        connect(Check,SIGNAL(valueChanged(QVariant)),
                this,SLOT(OnInputChanged(QVariant)));
        //m_GuiChecks[ObjectName] = Check;
        Settings->SetGuiCheck(Key,Check);
        }
        break;

      default :
        //printf("(%s,%d) No widget for '%s'\n",
        //       __FILE__,__LINE__,Key.toLocal8Bit().data());
        continue;
    };
    // To sync the state of the now created gui element we have
    // to set the value. As the setting now has the gui element
    // attached, it will be updated.
    Settings->SetValue(Key,Settings->GetValue(Key));
  }

  //
  // Menu structure related.
  //

  // menu removed ;-)

  // Leftover after menu was removed

  m_GroupBox = NULL;
  m_GroupBoxesOrdered = NULL;
  m_TabLayouts = NULL;
  m_MovedTools = new QList<QWidget*>;

  // NOTE: Following function does the UI replacing and ptGroupBox creation.
  // It also initializes new-style filters.
  OnToolBoxesEnabledTriggered(0);

  // Inserting new-style filters postponed to here because a fully functional m_TabLayouts
  // is needed.
  for (ptFilterBase *hFilter: *GFilterDM) {
    m_TabLayouts->at(hFilter->parentTabIdx())->insertWidget(hFilter->idxInParentTab(),
                                                            hFilter->gui());
  }

  //
  // Run pipe related
  //

  Macro_ConnectSomeButton(PreviewMode);
  PreviewModeButton->setChecked(Settings->GetInt("PreviewMode"));

  Macro_ConnectSomeButton(Run);
  RunButton->setEnabled(Settings->GetInt("RunMode"));

  Macro_ConnectSomeButton(TabProcessing);
  Macro_ConnectSomeButton(TabSettings);
  Macro_ConnectSomeButton(TabInfo);

  //
  // Zoom related
  //

  BottomContainer->setVisible(Settings->GetInt("ShowBottomContainer"));

  Macro_ConnectSomeButton(ZoomFit);
  Macro_ConnectSomeButton(ZoomIn);
  Macro_ConnectSomeButton(ZoomOut);
  Macro_ConnectSomeButton(ZoomFull);
  Macro_ConnectSomeButton(Batch);
  Macro_ConnectSomeButton(FileMgr);
  Macro_ConnectSomeButton(FullScreen);
  FullScreenButton->setChecked(0);
  Macro_ConnectSomeButton(LoadStyle);

// ATZ
  StarRating1 = new ptStarRating(StarRatingWidget);
  StarRating1->setToolTip(tr("Rating"));
  ColorLabel1 = new ptColorLabel(ColorLabelWidget);
  ColorLabel1->setToolTip(tr("Color label"));
  connect(StarRating1,SIGNAL(valueChanged()),this,SLOT(Form_2_Settings()));
  connect(ColorLabel1,SIGNAL(valueChanged()),this,SLOT(Form_2_Settings()));
  Macro_ConnectSomeButton(PreviousImage);
  Macro_ConnectSomeButton(NextImage);
  Macro_ConnectSomeButton(DeleteImage);
// end ATZ


  //
  // Connect Export button (not only for Gimp like the name suggests)
  //
  Macro_ConnectSomeButton(ToGimp);

  //
  // Settings related
  //

  Macro_ConnectSomeButton(BackgroundColor);
  QPixmap BgPix(80, 14);
  QColor  BgColor;
  BgColor.setRed(Settings->GetInt("BackgroundRed"));
  BgColor.setGreen(Settings->GetInt("BackgroundGreen"));
  BgColor.setBlue(Settings->GetInt("BackgroundBlue"));
  BgPix.fill(BgColor);
  BackgroundColorButton->setIcon(BgPix);

  Macro_ConnectSomeButton(Reset);

  //
  // TAB : Generic
  //

  Macro_ConnectSomeButton(CameraColorProfile);

  Macro_ConnectSomeButton(PreviewColorProfile);

  Macro_ConnectSomeButton(OutputColorProfile);

  Macro_ConnectSomeButton(GimpExecCommand);

  Macro_ConnectSomeButton(StartupSettings);

  //
  // TAB : Camera
  //

  Macro_ConnectSomeButton(OpenFile);
  Macro_ConnectSomeButton(OpenSettingsFile);
  Macro_ConnectSomeButton(OpenPresetFile);
  Macro_ConnectSomeButton(SpotWB);

  //
  // TAB : Geometry
  //

  // TODO: BJ Unhide when lensfun implementation has grown far enough
  widget_158->setVisible(false);  //Camera
  widget_159->setVisible(false);  //Lens

  Macro_ConnectSomeButton(RotateLeft);
  Macro_ConnectSomeButton(RotateRight);
  Macro_ConnectSomeButton(RotateAngle);
  Macro_ConnectSomeButton(MakeCrop);
  Macro_ConnectSomeButton(ConfirmCrop);
  Macro_ConnectSomeButton(CancelCrop);
  Macro_ConnectSomeButton(CropOrientation);
  Macro_ConnectSomeButton(CropCenterHor);
  Macro_ConnectSomeButton(CropCenterVert);

  //
  // TAB : RGB
  //

  Macro_ConnectSomeButton(ChannelMixerOpen);
  Macro_ConnectSomeButton(ChannelMixerSave);

  if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Auto) {
    Settings->SetEnabled("WhiteFraction",1);
    Settings->SetEnabled("WhiteLevel",1);
  } else {
    Settings->SetEnabled("WhiteFraction",0);
    Settings->SetEnabled("WhiteLevel",0);
  }



  //
  // TAB : EyeCandy
  //

  Macro_ConnectSomeButton(Tone1Color);
  Macro_ConnectSomeButton(Tone2Color);

  Macro_ConnectSomeButton(TextureOverlay);
  Macro_ConnectSomeButton(TextureOverlayClear);
  Macro_ConnectSomeButton(TextureOverlay2);
  Macro_ConnectSomeButton(TextureOverlay2Clear);

  Macro_ConnectSomeButton(GradualOverlay1Color);
  Macro_ConnectSomeButton(GradualOverlay2Color);

  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("Tone1ColorRed"));
  Color.setGreen(Settings->GetInt("Tone1ColorGreen"));
  Color.setBlue(Settings->GetInt("Tone1ColorBlue"));
  Pix.fill(Color);
  Tone1ColorButton->setIcon(Pix);
  Color.setRed(Settings->GetInt("Tone2ColorRed"));
  Color.setGreen(Settings->GetInt("Tone2ColorGreen"));
  Color.setBlue(Settings->GetInt("Tone2ColorBlue"));
  Pix.fill(Color);
  Tone2ColorButton->setIcon(Pix);
  Color.setRed(Settings->GetInt("GradualOverlay1ColorRed"));
  Color.setGreen(Settings->GetInt("GradualOverlay1ColorGreen"));
  Color.setBlue(Settings->GetInt("GradualOverlay1ColorBlue"));
  Pix.fill(Color);
  GradualOverlay1ColorButton->setIcon(Pix);
  Color.setRed(Settings->GetInt("GradualOverlay2ColorRed"));
  Color.setGreen(Settings->GetInt("GradualOverlay2ColorGreen"));
  Color.setBlue(Settings->GetInt("GradualOverlay2ColorBlue"));
  Pix.fill(Color);
  GradualOverlay2ColorButton->setIcon(Pix);

  //
  // TAB : Output
  //

  connect(TagsEditWidget,  SIGNAL(textChanged()),     this, SLOT(OnTagsEditTextChanged()));
  connect(edtOutputSuffix, SIGNAL(editingFinished()), this, SLOT(Form_2_Settings()));
  connect(edtImageTitle,   SIGNAL(editingFinished()), this, SLOT(Form_2_Settings()));
  connect(edtCopyright,    SIGNAL(editingFinished()), this, SLOT(Form_2_Settings()));


  Macro_ConnectSomeButton(OutputColorProfileReset);

  Macro_ConnectSomeButton(WritePipe);

  Settings->SetEnabled("OutputGamma",Settings->GetInt("OutputGammaCompensation"));
  Settings->SetEnabled("OutputLinearity",Settings->GetInt("OutputGammaCompensation"));

  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG)
    Settings->SetEnabled("SaveSampling",1);
  else
    Settings->SetEnabled("SaveSampling",0);

  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG ||
      Settings->GetInt("SaveFormat")==ptSaveFormat_PNG||
      Settings->GetInt("SaveFormat")==ptSaveFormat_PNG16)
    Settings->SetEnabled("SaveQuality",1);
  else
    Settings->SetEnabled("SaveQuality",0);

  // hide some stuff instead of removing from the code
  findChild<ptGroupBox *>(QString("TabToolBoxControl"))->setVisible(0);
  findChild<ptGroupBox *>(QString("TabMemoryTest"))->setVisible(0);
  findChild<ptGroupBox *>(QString("TabRememberSettings"))->setVisible(0);

  UpdateToolBoxes();

  // Set help pages
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabWhiteBalance"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/camera#white_balance");

  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunLensParameters"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#lens_parameters");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunCA"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#chromatic_aberration");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunVignette"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#vignetting");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunDistortion"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#lens_distortion");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunGeometry"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#geometry_conversion");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabDefish"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#defish");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabRotation"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#rotation_and_perspective");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabCrop"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#crop");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabLiquidRescale"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#seam_carving");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabResize"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#resize");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabFlip"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#flip");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabBlock"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#block");

  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabBW"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/eyecandy#black_and_white");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabGradualBlur1"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/eyecandy#gradual_blur");
  dynamic_cast<ptGroupBox*>(m_GroupBox->value("TabGradualBlur2"))->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/eyecandy#gradual_blur");

  m_ActiveTabs.append(LocalTab);
  m_ActiveTabs.append(GeometryTab);
  m_ActiveTabs.append(RGBTab);
  m_ActiveTabs.append(LabCCTab);
  m_ActiveTabs.append(LabSNTab);
  m_ActiveTabs.append(LabEyeCandyTab);
  m_ActiveTabs.append(EyeCandyTab);
  m_ActiveTabs.append(OutTab);

  m_StatusIcon = QIcon(QString::fromUtf8(":/dark/ui-graphics/indicator-active-tab.png"));

  // Profiles
  QFileInfo PathInfo(Settings->GetString("CameraColorProfile"));
  QString ShortFileName = PathInfo.fileName();
  CameraColorProfileText->setText(ShortFileName);

  PathInfo.setFile(Settings->GetString("PreviewColorProfile"));
  ShortFileName = PathInfo.fileName();
  PreviewColorProfileText->setText(ShortFileName);

  PathInfo.setFile(Settings->GetString("OutputColorProfile"));
  ShortFileName = PathInfo.fileName();
  OutputColorProfileText->setText(ShortFileName);

  PathInfo.setFile(Settings->GetString("GimpExecCommand"));
  ShortFileName = PathInfo.fileName();
  GimpExecCommandText->setText(ShortFileName);

  // Photivo version string on info tab.
  // Character replacements are hacks to avoid problems with make and stringify
  // and certain special characters.
  QString Temp(TOSTRING(APPVERSION));
  AppVersionLabel->setText(Temp);
  AppVersion2Label->setText(Temp);

  Tabbar = ProcessingTabBook->findChild<QTabBar*>();
  Tabbar->installEventFilter(this);
  WritePipeButton->installEventFilter(this);
  ToGimpButton->installEventFilter(this);
  ResetButton->installEventFilter(this);
  LogoLabel->installEventFilter(this);

  m_ContextMenuOnTab = -1;

  // context menu for save button
  m_AtnSavePipe = new QAction(tr("Save current pipe"), this);
  connect(m_AtnSavePipe, SIGNAL(triggered()), this, SLOT(SaveMenuPipe()));
  m_AtnSaveFull = new QAction(tr("Save full size"), this);
  connect(m_AtnSaveFull, SIGNAL(triggered()), this, SLOT(SaveMenuFull()));
  m_AtnSaveSettings = new QAction(tr("Save settings file"), this);
  connect(m_AtnSaveSettings, SIGNAL(triggered()), this, SLOT(SaveMenuSettings()));
  m_AtnSaveJobfile = new QAction(tr("Save job file"), this);
  connect(m_AtnSaveJobfile, SIGNAL(triggered()), this, SLOT(SaveMenuJobfile()));
  m_AtnSendToBatch = new QAction(tr("Send to batch"), this);
  connect(m_AtnSendToBatch, SIGNAL(triggered()), this, SLOT(SaveMenuBatch()));

  // context menu for gimp button
  m_AtnGimpSavePipe = new QAction(tr("Export current pipe"), this);
  connect(m_AtnGimpSavePipe, SIGNAL(triggered()), this, SLOT(GimpSaveMenuPipe()));
  m_AtnGimpSaveFull = new QAction(tr("Export full size"), this);
  connect(m_AtnGimpSaveFull, SIGNAL(triggered()), this, SLOT(GimpSaveMenuFull()));

  // context menu for reset button
  m_AtnMenuFullReset = new QAction(tr("Neutral reset"), this);
  connect(m_AtnMenuFullReset, SIGNAL(triggered()), this, SLOT(MenuFullReset()));
  m_AtnMenuUserReset = new QAction(tr("User reset"), this);
  connect(m_AtnMenuUserReset, SIGNAL(triggered()), this, SLOT(MenuUserReset()));
  m_AtnMenuOpenPreset = new QAction(tr("Open preset"), this);
  connect(m_AtnMenuOpenPreset, SIGNAL(triggered()), this, SLOT(MenuOpenPreset()));
  m_AtnMenuOpenSettings = new QAction(tr("Open settings"), this);
  connect(m_AtnMenuOpenSettings, SIGNAL(triggered()), this, SLOT(MenuOpenSettings()));

  // context menu to show tools
  m_AtnShowTools = new QAction(tr("&Show hidden tools"), this);
  connect(m_AtnShowTools, SIGNAL(triggered()), this, SLOT(ShowToolsOnTab()));
  m_AtnShowTools->setIcon(QIcon(*Theme->ptIconCheckGreen));
  m_AtnShowTools->setIconVisibleInMenu(true);

  // Search bar
  Macro_ConnectSomeButton(SearchReset);
  Macro_ConnectSomeButton(SearchActiveTools);
  Macro_ConnectSomeButton(SearchAllTools);
  Macro_ConnectSomeButton(SearchFavouriteTools);
  connect(SearchInputWidget, SIGNAL(textEdited(QString)), this, SLOT(StartSearchTimer(QString)));
  SearchWidget->setVisible(Settings->GetInt("SearchBarEnable"));
#if (QT_VERSION >= 0x40700)
  SearchInputWidget->setPlaceholderText(tr("Search"));
#endif
  m_SearchInputTimer = new QTimer(this);
  m_SearchInputTimer->setSingleShot(1);
  connect(m_SearchInputTimer, SIGNAL(timeout()), this, SLOT(Search()));

  // Set the toolpane to show the pipe controls ...
  MainTabBook->blockSignals(1);
  MainTabBook->setCurrentWidget(TabProcessing);
  MainTabBook->blockSignals(0);

  // ... and open the first pipe controls tab
  ProcessingTabBook->blockSignals(1);
  ProcessingTabBook->setCurrentIndex(0);
  ProcessingTabBook->blockSignals(0);

  setAcceptDrops(true);     // Drag and drop

  // Timer to delay on resize operations.
  // (avoiding excessive calculations and loops in the ZoomFit approach.)
  m_ResizeTimer = new QTimer(this);
  m_ResizeTimer->setSingleShot(1);
  connect(m_ResizeTimer,
          SIGNAL(timeout()),
          this,
          SLOT(ResizeTimerExpired()));
  // This qualifies for an ugly hack :)
  // Need an event at t=0 at toplevel.
  m_Event0Timer = new QTimer(this);
  m_Event0Timer->setSingleShot(1);
  m_Event0Timer->start(0);
  connect(m_Event0Timer,
          SIGNAL(timeout()),
          this,
          SLOT(Event0TimerExpired()));

  FileMgrThumbMaxRowColWidget->setEnabled(Settings->GetInt("FileMgrUseThumbMaxRowCol"));

  UpdateCropToolUI();
  UpdateLfunDistUI();
  UpdateLfunCAUI();
  UpdateLfunVignetteUI();
  UpdateLiquidRescaleUI();
  UpdateGradualBlurUI();
  InitVisibleTools();
  Settings->SetValue("PipeSize", Settings->GetValue("StartupPipeSize"));
  if (Settings->GetInt("StartupUIMode") == ptStartupUIMode_Favourite)
    ShowFavouriteTools();
  if (Settings->GetInt("StartupUIMode") == ptStartupUIMode_AllTools)
    ShowAllTools();

  // Show the file manager if no image loaded at startup, the image editor otherwise.
  // Do this last in the constructor because it triggers thumbnail reading.
#ifndef PT_WITHOUT_FILEMGR
  if (ImageFileToOpen == "" &&
      Settings->GetInt("FileMgrStartupOpen") &&
      !Settings->GetInt("PreventFileMgrStartup"))
  {
    SwitchUIState(uisFileMgr);
  } else {
    SwitchUIState(uisProcessing);
  }
#else
  SwitchUIState(uisProcessing);
  findChild<ptGroupBox *>(QString("TabFileMgrSettings"))->setVisible(0);
#endif

  AutosaveSettingsWidget->setDisabled(Settings->GetInt("SaveConfirmation"));
}

//==============================================================================

void CB_Event0();
void ptMainWindow::Event0TimerExpired() {
  ::CB_Event0();
}

//==============================================================================

// Setup the UI language combobox. Only done once after constructing MainWindow.
// The languages combobox is a regular QComboBox because its dynamic content (depends on
// available translation files) doesn't fit the logic of ptChoice (static, pre-defined lists).
void ptMainWindow::PopulateTranslationsCombobox(const QStringList UiLanguages, const int LangIdx) {
  TranslationChoice->setFixedHeight(24);
  TranslationChoice->addItem(tr("English (Default)"));
  TranslationChoice->addItems(UiLanguages);
  TranslationChoice->setCurrentIndex(LangIdx==-1 ? 0 : LangIdx+1);
  connect(TranslationChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(OnTranslationChoiceChanged(int)));
}

//==============================================================================

void ptMainWindow::OnTranslationChoiceChanged(int idx) {
  TranslationLabel->setText(tr("Restart Photivo to change the language."));
  if (idx == 0) {   // no translation
    Settings->SetValue("TranslationMode", 0);
    Settings->SetValue("UiLanguage", "");
  } else {
    Settings->SetValue("TranslationMode", 1);
    Settings->SetValue("UiLanguage", TranslationChoice->itemText(idx));
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// Event filter
//
////////////////////////////////////////////////////////////////////////////////

bool ptMainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::ContextMenu) {
    if (obj == Tabbar) {
      // compute the tab number
      QContextMenuEvent *mouseEvent = static_cast<QContextMenuEvent *>(event);
      QPoint position = mouseEvent->pos();
      int c = Tabbar->count();
      int clickedItem = -1;

      for (int i=0; i<c; i++) {
        if ( Tabbar->tabRect(i).contains( position ) ) {
          clickedItem = i;
          break;
        }
      }
      // tools on this tab hidden?
      short HaveHiddenTools = 0;
      QStringList TempList = Settings->GetStringList("HiddenTools");
      foreach (ptTempFilterBase* GroupBox, *m_GroupBox) {
        if (TempList.contains(GroupBox->objectName())) {
          short Tab = GroupBox->parentTabIdx();
          if (Tab == clickedItem) HaveHiddenTools = 1;
        }
      }
      if (clickedItem !=0 && HaveHiddenTools == 1) { // no hidden tools on camera tab
        m_ContextMenuOnTab = clickedItem;
        QMenu Menu(NULL);
        Menu.setStyle(Theme->style());
        Menu.setPalette(Theme->menuPalette());
        Menu.addAction(m_AtnShowTools);
        Menu.exec(static_cast<QContextMenuEvent *>(event)->globalPos());
      }
    } else if (obj == WritePipeButton) {
      QMenu Menu(NULL);
      Menu.setStyle(Theme->style());
      Menu.setPalette(Theme->menuPalette());
      Menu.addAction(m_AtnSavePipe);
      Menu.addAction(m_AtnSaveFull);
      Menu.addAction(m_AtnSaveSettings);
      Menu.addAction(m_AtnSaveJobfile);
      Menu.addAction(m_AtnSendToBatch);
      Menu.exec(static_cast<QContextMenuEvent *>(event)->globalPos());
    } else if (obj == ToGimpButton) {
      QMenu Menu(NULL);
      Menu.setStyle(Theme->style());
      Menu.setPalette(Theme->menuPalette());
      Menu.addAction(m_AtnGimpSavePipe);
      Menu.addAction(m_AtnGimpSaveFull);
      Menu.exec(static_cast<QContextMenuEvent *>(event)->globalPos());
    } else if (obj == ResetButton) {
      QMenu Menu(NULL);
      Menu.setStyle(Theme->style());
      Menu.setPalette(Theme->menuPalette());
      Menu.addAction(m_AtnMenuUserReset);
      Menu.addAction(m_AtnMenuFullReset);
      Menu.addSeparator();
      Menu.addAction(m_AtnMenuOpenPreset);
      Menu.addAction(m_AtnMenuOpenSettings);
      Menu.exec(static_cast<QContextMenuEvent *>(event)->globalPos());
    }
    return QObject::eventFilter(obj, event);
  } else if (obj == LogoLabel &&
             event->type() == QEvent::MouseButtonPress &&
             static_cast <QMouseEvent*>(event)->button() == Qt::LeftButton) {
    ::CB_OpenFileButton();
    return 0;
  } else {
    // pass the event on to the parent class
    return QObject::eventFilter(obj, event);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Slots for context menu on save button
//
////////////////////////////////////////////////////////////////////////////////

extern void SaveOutput(const short mode);

void ptMainWindow::SaveMenuPipe() {
  SaveOutput(ptOutputMode_Pipe);
}
void ptMainWindow::SaveMenuFull() {
  SaveOutput(ptOutputMode_Full);
}
void ptMainWindow::SaveMenuSettings() {
  SaveOutput(ptOutputMode_Settingsfile);
}
void ptMainWindow::SaveMenuJobfile() {
  SaveOutput(ptOutputMode_Jobfile);
}
void ptMainWindow::SaveMenuBatch() {
  SaveOutput(ptOutputMode_Batch);
}

////////////////////////////////////////////////////////////////////////////////
//
// Slots for context menu on gimp button
//
////////////////////////////////////////////////////////////////////////////////

extern void Export(const short mode);

void ptMainWindow::GimpSaveMenuPipe() {
  Export(ptExportMode_GimpPipe);
}
void ptMainWindow::GimpSaveMenuFull() {
  Export(ptExportMode_GimpFull);
}

////////////////////////////////////////////////////////////////////////////////
//
// Slots for context menu on reset button
//
////////////////////////////////////////////////////////////////////////////////

extern void ResetButtonHandler(const short mode);

void ptMainWindow::MenuFullReset() {
  ResetButtonHandler(ptResetMode_Full);
}

void ptMainWindow::MenuUserReset() {
  ResetButtonHandler(ptResetMode_User);
}

void ptMainWindow::MenuOpenPreset() {
  ResetButtonHandler(ptResetMode_OpenPreset);
}

void ptMainWindow::MenuOpenSettings() {
  ResetButtonHandler(ptResetMode_OpenSettings);
}

////////////////////////////////////////////////////////////////////////////////
//
// Slots for context menu on tabs
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::ShowToolsOnTab() {
  // show hidden tools on tab m_ContextMenuOnTab
  QString ActiveTool= "";
  QString TempString = "";
  QStringList TempList = Settings->GetStringList("HiddenTools");
  TempList.removeDuplicates();

  foreach (ptTempFilterBase* GroupBox, *m_GroupBox) {
    TempString = GroupBox->objectName();

    if (TempList.contains(TempString)) {
      short Tab = GroupBox->parentTabIdx();

      if (m_ContextMenuOnTab==Tab) {
        GroupBox->guiWidget()->show();
        TempList.removeOne(TempString);
        Settings->SetValue("HiddenTools", TempList);

        if (GroupBox->isActive()) {
          ActiveTool = TempString;
        }
      }
    }
  }
  Settings->SetValue("HiddenTools", TempList);
  // run processor if needed
  if (ActiveTool != "") Update(ActiveTool);
}


////////////////////////////////////////////////////////////////////////////////
//
// Added slot for messages to the single instance
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::OtherInstanceMessage(const QString &msg) {
  // Settings file loaded via cli
  if (msg.startsWith("::pts::")) {
    PtsFileToOpen = msg;
    PtsFileToOpen.remove(0,7);

    if (ptConfirmRequest::loadConfig(lcmSettingsFile, PtsFileToOpen)) {
      CB_OpenSettingsFile(PtsFileToOpen);
    }

  // Image file loaded via cli
  } else if (msg.startsWith("::img::")) {
    ImageFileToOpen = msg;
    ImageFileToOpen.remove(0,7);
    CB_MenuFileOpen(1);

  // image incoming from Gimp
  } else if (msg.startsWith("::tmp::")) {
    ImageFileToOpen = msg;
    ImageFileToOpen.remove(0,7);
    ImageCleanUp++;
    CB_MenuFileOpen(1);
  } else if (msg.startsWith("::sidecar::")) {
    QString Sidecar = msg;
    Sidecar.remove(0,11);
    ReadSidecar(Sidecar);
    Settings->SetValue("Sidecar", Sidecar);
  }
  
}

//==============================================================================

////////////////////////////////////////////////////////////////////////////////
//
// GetCurrentTab
//
// Determine the current tab selected.
//
////////////////////////////////////////////////////////////////////////////////

short ptMainWindow::GetCurrentTab() {
  // I opt for matching onto widget name, rather than on
  // index, as I feel that this is more robust for change.
  if      (ProcessingTabBook->currentWidget() == CameraTab) return ptCameraTab;
  else if (ProcessingTabBook->currentWidget() == LocalTab) return ptLocalTab;
  else if (ProcessingTabBook->currentWidget() == GeometryTab) return ptGeometryTab;
  else if (ProcessingTabBook->currentWidget() == RGBTab) return ptRGBTab;
  else if (ProcessingTabBook->currentWidget() == LabCCTab) return ptLabCCTab;
  else if (ProcessingTabBook->currentWidget() == LabSNTab) return ptLabSNTab;
  else if (ProcessingTabBook->currentWidget() == LabEyeCandyTab) return ptLabEyeCandyTab;
  else if (ProcessingTabBook->currentWidget() == EyeCandyTab) return ptEyeCandyTab;
  else if (ProcessingTabBook->currentWidget() == OutTab) return ptOutTab;
  else {
     ptLogError(ptError_Argument,"Unforeseen tab.");
     assert(0);
     return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// All kind of Gui events.
// Translated back to a CB_ function into ptMain
//
////////////////////////////////////////////////////////////////////////////////

//==============================================================================
// Show/hide file manager window

void ptMainWindow::OpenBatchWindow() {
  SwitchUIState(uisBatch);
}

void ptMainWindow::CloseBatchWindow() {
  SwitchUIState(uisProcessing);
}

void ptMainWindow::OpenFileMgrWindow() {
  SwitchUIState(uisFileMgr);
}

void ptMainWindow::CloseFileMgrWindow() {
  SwitchUIState(uisProcessing);
}

//==============================================================================
// Tabbook switching

void CB_Tabs(const short Index);
void ptMainWindow::on_ProcessingTabBook_currentChanged(const int Index) {
  ::CB_Tabs(Index);
}

//==============================================================================

struct sToolBoxStructure {
  QToolBox*             ToolBox;
  QWidget*              Parent;
  QVBoxLayout*          ParentLayout;
  // The standard layout in toolbox pages.
  QList <QWidget *>     Pages;
  QList <QVBoxLayout *> PageLayouts;
  // The alternative layout in groupboxes.
  QList <ptGroupBox *>   GroupBoxes;
  QList <QVBoxLayout *> GroupBoxLayouts;
  // The widgets we foresaw moving.
  QList <QWidget *>     Widgets;
};

QList <sToolBoxStructure*> ToolBoxStructureList;

void ptMainWindow::AnalyzeToolBoxStructure() {
  // QMap for all processing group boxes (without settings and info!)
  // QMap Name -> ptGroupBox*
  if (m_GroupBox) delete m_GroupBox;
  m_GroupBox = new QMap<QString, ptTempFilterBase*>;
  if (m_GroupBoxesOrdered) delete m_GroupBoxesOrdered;
  m_GroupBoxesOrdered = new QList<QString>;
  if (m_TabLayouts) delete m_TabLayouts;
  m_TabLayouts = new QList<QVBoxLayout*>;

  // Each processing tab has one QToolBox containing a list of QWidgets as direct children.
  // Each of those is one filter and will become one groupbox.
  // The settings page and info page are represented by QToolBox "SettingsToolBox" and
  // "InfoToolBox". They are excluded from the m_GroupBox and m_GrouBoxesOrdered lists. I.e.
  // those lists only contain the filters on the proccessing page.
  QList <QToolBox *> ToolBoxes;
  short NumberOfTabs = ProcessingTabBook->count();
  for (short i=0; i<NumberOfTabs; i++) {
    ToolBoxes.append(ProcessingTabBook->widget(i)->findChildren <QToolBox *> ());
  }
  ToolBoxes.append(SettingsToolBox);
  ToolBoxes.append(InfoToolBox);

  // iterate over the QToolBoxes
  for (short itTab=0; itTab<ToolBoxes.size(); itTab++) {
    sToolBoxStructure* ToolBoxStructure = new sToolBoxStructure;
    ToolBoxStructureList.append(ToolBoxStructure);
    ToolBoxStructure->ToolBox = ToolBoxes[itTab];
    ToolBoxStructure->Parent =
      qobject_cast <QWidget*>(ToolBoxes[itTab]->parent());
    ToolBoxStructure->ParentLayout =
      qobject_cast <QVBoxLayout*>(ToolBoxStructure->Parent->layout());
    assert (ToolBoxStructure->ParentLayout);
    if (ToolBoxStructure->ToolBox != SettingsToolBox &&
        ToolBoxStructure->ToolBox != InfoToolBox) {
      m_TabLayouts->append(ToolBoxStructure->ParentLayout);
    }

    // iterate over the the QToolBox child widgets (i.e. the individual filters)
    for (short itIdx=0; itIdx<ToolBoxes[itTab]->count(); itIdx++) {
      // handle dummy entries for new-style filters
      auto hNewFilter = GFilterDM->GetFilterFromName(ToolBoxes[itTab]->itemText(itIdx), false);
      if (hNewFilter) {
        hNewFilter->setPos(itTab, itIdx);
        m_GroupBoxesOrdered->append(hNewFilter->uniqueName());
        m_GroupBox->insert(hNewFilter->uniqueName(), hNewFilter);
        continue;
      }

      // create groupboxes etc. for old-style filters
      QWidget* Page = ToolBoxes[itTab]->widget(itIdx);
      QVBoxLayout* PageLayout = qobject_cast <QVBoxLayout*>(Page->layout());
      ToolBoxStructure->Pages.append(Page);
      ToolBoxStructure->PageLayouts.append(PageLayout);
      // Alternative groupboxes. The layout of which still needs to be created.
      ptGroupBox* GroupBox =
        new ptGroupBox(ToolBoxStructure->ToolBox->itemText(itIdx),
                       this,
                       ToolBoxStructure->ToolBox->widget(itIdx)->objectName(),
                       itTab<NumberOfTabs?ProcessingTabBook->widget(itTab)->objectName():"",
                       itTab,
                       itIdx);

      if (ToolBoxStructure->ToolBox != SettingsToolBox &&
          ToolBoxStructure->ToolBox != InfoToolBox) {
        m_GroupBox->insert(ToolBoxStructure->ToolBox->widget(itIdx)->objectName(),
                           GroupBox);
        m_GroupBoxesOrdered->append(ToolBoxStructure->ToolBox->widget(itIdx)->objectName());
      }

      QVBoxLayout* GroupBoxLayout = new QVBoxLayout(GroupBox->m_Widget);
      GroupBoxLayout->setSpacing(0);
      GroupBoxLayout->setMargin(0);
      ToolBoxStructure->GroupBoxes.append(GroupBox);
      ToolBoxStructure->GroupBoxLayouts.append(GroupBoxLayout);
      // The list of childwidgets.
      QList <QWidget *> DirectChildrenOfPage = findChildren <QWidget *> ();
      // findChildren is always recursive. Remove lots.
      for (short k=0; k<DirectChildrenOfPage.size();) {
        if (DirectChildrenOfPage[k]->parent() != Page) {
          DirectChildrenOfPage.removeAt(k);
        } else {
          k++;
        }
      }
      GInfo->Assert(DirectChildrenOfPage.size() == 1,
                    "Failed analyzing filter named " + ToolBoxes[itTab]->itemText(itIdx) +
                    ". If you see a FUID as the name you probably forgot to create the "
                    "corresponding new-style filter in ptMain::CreateAllFilters().", AT);
      ToolBoxStructure->Widgets.append(DirectChildrenOfPage[0]);
    }
  }
}

//==============================================================================

void ptMainWindow::OnToolBoxesEnabledTriggered(const bool Enabled) {
  if (ToolBoxStructureList.size() == 0) {
    AnalyzeToolBoxStructure();
  }

  // Empty all involved layouts.
  for (short Idx=0; Idx<ToolBoxStructureList.size(); Idx++) {
    sToolBoxStructure* ToolBoxStructure = ToolBoxStructureList[Idx];
    // 1. Parent.
    while (ToolBoxStructure->ParentLayout->count()) {
      ToolBoxStructure->ParentLayout->
        removeItem(ToolBoxStructure->ParentLayout->itemAt(0));
    }
    for (short j=0; j<ToolBoxStructure->Widgets.size(); j++) {
      // 2. Groupboxes.
      while (ToolBoxStructure->GroupBoxLayouts[j]->count()) {
        ToolBoxStructure->GroupBoxLayouts[j]->
          removeItem(ToolBoxStructure->GroupBoxLayouts[j]->itemAt(0));
      }
      // 3. Pages.
      while (ToolBoxStructure->PageLayouts[j]->count()) {
        ToolBoxStructure->PageLayouts[j]->
          removeItem(ToolBoxStructure->PageLayouts[j]->itemAt(0));
      }
    }
  }

  // Orphan widgets (TODO : maybe not really needed ..)
  for (short Idx=0; Idx<ToolBoxStructureList.size(); Idx++) {
    sToolBoxStructure* ToolBoxStructure = ToolBoxStructureList[Idx];
    for (short j=0; j<ToolBoxStructure->Widgets.size(); j++) {
      ToolBoxStructure->Widgets[j]->setParent(NULL);
      ToolBoxStructure->GroupBoxes[j]->setParent(NULL);
    }
    ToolBoxStructure->ToolBox->setParent(NULL);
  }

  // Now start relayouting.

  // Layout for groupbox mode.
  if (!Enabled) {
    for (short Idx=0; Idx<ToolBoxStructureList.size(); Idx++) {
      sToolBoxStructure* ToolBoxStructure = ToolBoxStructureList[Idx];
      for (short j=0; j<ToolBoxStructure->Widgets.size(); j++) {
        ToolBoxStructure->Widgets[j]->
          setParent(ToolBoxStructure->GroupBoxes[j]);
        ToolBoxStructure->GroupBoxLayouts[j]->
          addWidget(ToolBoxStructure->Widgets[j]);
        ToolBoxStructure->Widgets[j]->layout()->setSpacing(4);
        ToolBoxStructure->GroupBoxes[j]->setParent(ToolBoxStructure->Parent);
        ToolBoxStructure->ParentLayout->
          addWidget(ToolBoxStructure->GroupBoxes[j]);
      }
      ToolBoxStructure->ParentLayout->addStretch();
      ToolBoxStructure->ParentLayout->setSpacing(0);
      ToolBoxStructure->ParentLayout->setContentsMargins(0,0,0,0);
      ToolBoxStructure->ParentLayout->setMargin(0);
      ToolBoxStructure->Parent->show();
    }
  }

  // Layout for toolbox mode.
  if (Enabled) {
    GInfo->Raise("Toolbox mode is ancient and dysfunctional!", AT);
  }
}


//
// Gimp
//

void ptMainWindow::OnToGimpButtonClicked() {
#ifdef DLRAW_GIMP_PLUGIN
  ::CB_MenuFileExit(1);
#endif
  GimpSaveMenuPipe();
}

//
// PreviewMode
//

void CB_PreviewModeButton(const QVariant State);
void ptMainWindow::OnPreviewModeButtonClicked() {
  if (PreviewModeButton->isChecked())
    ::CB_PreviewModeButton(1);
  else
    ::CB_PreviewModeButton(0);
}

//
// Run
//

void CB_RunButton();
void ptMainWindow::OnRunButtonClicked() {
  ::CB_RunButton();
}

void CB_ResetButton();
void ptMainWindow::OnResetButtonClicked() {
  ::CB_ResetButton();
}

//
// Zoom
//

void CB_ZoomFitButton();
void ptMainWindow::OnZoomFitButtonClicked() {
  ::CB_ZoomFitButton();
}

void CB_ZoomFullButton();
void ptMainWindow::OnZoomFullButtonClicked() {
  ::CB_ZoomFullButton();
}

void ptMainWindow::OnZoomInButtonClicked() {
  ViewWindow->ZoomStep(1);
}

void ptMainWindow::OnZoomOutButtonClicked() {
  ViewWindow->ZoomStep(-1);
}


void CB_InputChanged(const QString ObjectName, const QVariant Value);
void ptMainWindow::OnInputChanged(const QVariant Value) {
  QObject* Sender = sender();
  printf("(%s,%d) Sender : '%s'\n",
         __FILE__,__LINE__,Sender->objectName().toLocal8Bit().data());
  CB_InputChanged(Sender->objectName(),Value);

}


// ATZ
void CB_PreviousImageButton();
void ptMainWindow::OnPreviousImageButtonClicked() {
  ::CB_PreviousImageButton();
}

void CB_NextImageButton();
void ptMainWindow::OnNextImageButtonClicked() {
  ::CB_NextImageButton();
}

void CB_DeleteImageButton();
void ptMainWindow::OnDeleteImageButtonClicked() {
  ::CB_DeleteImageButton();
}
// end ATZ

void CB_BatchButton();
void ptMainWindow::OnBatchButtonClicked() {
  ::CB_BatchButton();
}

void CB_FileMgrButton();
void ptMainWindow::OnFileMgrButtonClicked() {
  ::CB_FileMgrButton();
}

void CB_FullScreenButton(const int State);
void ptMainWindow::OnFullScreenButtonClicked() {
  if (FullScreenButton->isChecked())
    ::CB_FullScreenButton(1);
  else
    ::CB_FullScreenButton(0);
}

void CB_LoadStyleButton();
void ptMainWindow::OnLoadStyleButtonClicked() {
  ::CB_LoadStyleButton();
}

void ptMainWindow::OnTabProcessingButtonClicked() {
  // clean up first!
  if (m_MovedTools->size()>0) CleanUpMovedTools();
  SearchInputWidget->setText("");
  ViewWindow->setFocus();

  // apply changes at VisibleTools box
  if (MainTabBook->currentWidget() == TabSetting)
    ApplyVisibleTools();

  MainTabBook->setCurrentWidget(TabProcessing);
}

void ptMainWindow::OnTabSettingsButtonClicked() {
  // clean up first!
  if (m_MovedTools->size()>0) CleanUpMovedTools();
  SearchInputWidget->setText("");
  ViewWindow->setFocus();

  if (MainTabBook->currentWidget() == TabSetting) {
    // apply changes at VisibleTools box
    ApplyVisibleTools();
    MainTabBook->setCurrentWidget(TabProcessing);
  }
  else {
    // update VisibleTools box
    UpdateVisibleTools();
    MainTabBook->setCurrentWidget(TabSetting);
  }
}

void ptMainWindow::OnTabInfoButtonClicked() {
  // clean up first!
  if (m_MovedTools->size()>0) CleanUpMovedTools();
  SearchInputWidget->setText("");
  ViewWindow->setFocus();

  // apply changes at VisibleTools box
  if (MainTabBook->currentWidget() == TabSetting)
    ApplyVisibleTools();

  if (MainTabBook->currentWidget() == TabInfo)
    MainTabBook->setCurrentWidget(TabProcessing);
  else {
    UpdateSettings();
    MainTabBook->setCurrentWidget(TabInfo);
  }
}

//
// Settings
//

void CB_BackgroundColorButton();
void ptMainWindow::OnBackgroundColorButtonClicked() {
  ::CB_BackgroundColorButton();
}

//
// Tab Generic
//

void CB_CameraColorProfileButton();
void ptMainWindow::OnCameraColorProfileButtonClicked() {
  ::CB_CameraColorProfileButton();
}

void CB_PreviewColorProfileButton();
void ptMainWindow::OnPreviewColorProfileButtonClicked() {
  ::CB_PreviewColorProfileButton();
}

void CB_OutputColorProfileButton();
void ptMainWindow::OnOutputColorProfileButtonClicked() {
  ::CB_OutputColorProfileButton();
}

void CB_GimpExecCommandButton();
void ptMainWindow::OnGimpExecCommandButtonClicked() {
  ::CB_GimpExecCommandButton();
}

void CB_StartupSettingsButton();
void ptMainWindow::OnStartupSettingsButtonClicked() {
  ::CB_StartupSettingsButton();
}


//
// Tab : Camera
//

void ptMainWindow::OnOpenFileButtonClicked() {
  ::CB_OpenFileButton();
}
void CB_OpenSettingsFileButton();
void ptMainWindow::OnOpenSettingsFileButtonClicked() {
  ::CB_OpenSettingsFileButton();
}
void CB_OpenPresetFileButton();
void ptMainWindow::OnOpenPresetFileButtonClicked() {
  ::CB_OpenPresetFileButton();
}

void CB_SpotWBButton();
void ptMainWindow::OnSpotWBButtonClicked() {
  ::CB_SpotWBButton();
}

//
// Tab : Geometry
//


void CB_RotateLeftButton();
void ptMainWindow::OnRotateLeftButtonClicked() {
  ::CB_RotateLeftButton();
}

void CB_RotateRightButton();
void ptMainWindow::OnRotateRightButtonClicked() {
  ::CB_RotateRightButton();
}

void CB_RotateAngleButton();
void ptMainWindow::OnRotateAngleButtonClicked() {
  ::CB_RotateAngleButton();
}

void CB_MakeCropButton();
void ptMainWindow::OnMakeCropButtonClicked() {
  ::CB_MakeCropButton();
}

void CB_ConfirmCropButton();
void ptMainWindow::OnConfirmCropButtonClicked() {
  ::CB_ConfirmCropButton();
}

void CB_CancelCropButton();
void ptMainWindow::OnCancelCropButtonClicked() {
  ::CB_CancelCropButton();
}

void CB_CropOrientationButton();
void ptMainWindow::OnCropOrientationButtonClicked() {
  ::CB_CropOrientationButton();
}

void CB_CropCenterHorButton();
void ptMainWindow::OnCropCenterHorButtonClicked() {
  ::CB_CropCenterHorButton();
}

void CB_CropCenterVertButton();
void ptMainWindow::OnCropCenterVertButtonClicked() {
  ::CB_CropCenterVertButton();
}


//
// Tab : RGB
//


void CB_ChannelMixerOpenButton();
void ptMainWindow::OnChannelMixerOpenButtonClicked() {
  ::CB_ChannelMixerOpenButton();
}
void CB_ChannelMixerSaveButton();
void ptMainWindow::OnChannelMixerSaveButtonClicked() {
  ::CB_ChannelMixerSaveButton();
}


// Tab : EyeCandy

void CB_Tone1ColorButton();
void ptMainWindow::OnTone1ColorButtonClicked() {
  ::CB_Tone1ColorButton();
}

void CB_Tone2ColorButton();
void ptMainWindow::OnTone2ColorButtonClicked() {
  ::CB_Tone2ColorButton();
}


void CB_TextureOverlayButton();
void ptMainWindow::OnTextureOverlayButtonClicked() {
  ::CB_TextureOverlayButton();
}

void CB_TextureOverlayClearButton();
void ptMainWindow::OnTextureOverlayClearButtonClicked() {
  ::CB_TextureOverlayClearButton();
}

void CB_TextureOverlay2Button();
void ptMainWindow::OnTextureOverlay2ButtonClicked() {
  ::CB_TextureOverlay2Button();
}

void CB_TextureOverlay2ClearButton();
void ptMainWindow::OnTextureOverlay2ClearButtonClicked() {
  ::CB_TextureOverlay2ClearButton();
}


void CB_GradualOverlay1ColorButton();
void ptMainWindow::OnGradualOverlay1ColorButtonClicked() {
  ::CB_GradualOverlay1ColorButton();
}

void CB_GradualOverlay2ColorButton();
void ptMainWindow::OnGradualOverlay2ColorButtonClicked() {
  ::CB_GradualOverlay2ColorButton();
}

// Tab : Output

void CB_OutputColorProfileResetButton();
void ptMainWindow::OnOutputColorProfileResetButtonClicked() {
  ::CB_OutputColorProfileResetButton();
}

void CB_WritePipeButton();
void ptMainWindow::OnWritePipeButtonClicked() {
  ::CB_WritePipeButton();
}

void PrepareTags(const QString TagsInput);
void ptMainWindow::OnTagsEditTextChanged() {
  PrepareTags(TagsEditWidget->toPlainText());
}

// Intercept close event and translate to a FileExit.
void ptMainWindow::closeEvent(QCloseEvent *Event) {
  Event->ignore();
  ::CB_MenuFileExit(1);
}

// Fit the image again after a resize event.

void ptMainWindow::resizeEvent(QResizeEvent*) {
  // Avoiding excessive calculations and ZoomFit loops.
  m_ResizeTimer->start(500); // 500 ms.
}

void ptMainWindow::ResizeTimerExpired() {
  // 250 would be the size for the progress label then.
  if (Settings->GetInt("ZoomMode") == ptZoomMode_Fit) {
    ::CB_ZoomFitButton();
  }
}

////////////////////////////////////////////////////////////////////////
//
// dragEnterEvent
//
////////////////////////////////////////////////////////////////////////

void ptMainWindow::dragEnterEvent(QDragEnterEvent* Event) {
  if (ViewWindow->interaction() != iaNone) {
    return;
  }

  // accept just text/uri-list mime format
  if (Event->mimeData()->hasFormat("text/uri-list")) {
    Event->acceptProposedAction();
  }
}


////////////////////////////////////////////////////////////////////////
//
// dropEvent
//
////////////////////////////////////////////////////////////////////////

void ptMainWindow::dropEvent(QDropEvent* Event) {
  if (ViewWindow->interaction() != iaNone) {
    return;
  }

  QList<QUrl> UrlList;
  QString DropName;
  QFileInfo DropInfo;

  if (Event->mimeData()->hasUrls())
  {
    UrlList = Event->mimeData()->urls(); // returns list of QUrls

    // if just text was dropped, urlList is empty (size == 0)
    if ( UrlList.size() > 0) // if at least one QUrl is present in list
    {
      DropName = UrlList[0].toLocalFile(); // convert first QUrl to local path
      DropInfo.setFile( DropName ); // information about file
      if ( DropInfo.isFile() ) { // if is file
        if (DropInfo.completeSuffix()!="pts" && DropInfo.completeSuffix()!="ptj" &&
            DropInfo.completeSuffix()!="dls" && DropInfo.completeSuffix()!="dlj")
        {
          if (Settings->GetInt("FileMgrIsOpen"))
            CloseFileMgrWindow();
          ImageFileToOpen = DropName;
          CB_MenuFileOpen(1);

        } else {
          if (!Settings->GetInt("FileMgrIsOpen")) {
            if (ptConfirmRequest::loadConfig(lcmSettingsFile, DropName)) {
              CB_OpenSettingsFile(DropName);
            }
          }
        }
      }
    }
  }
  Event->acceptProposedAction();
} 

void CB_FullScreenButton(const int State);
void UpdateSettings();
void CB_RunModeCheck(const QVariant Check);
void CB_OpenPresetFileButton();
void CB_InputChanged(const QString,const QVariant);
void CB_ZoomFitButton();
void CB_WritePipeButton();
void CB_SpecialPreviewChoice(const QVariant Choice);
void CB_MenuFileExit(const short);
void ViewWindowShowStatus(short State);
void CB_SearchBarEnableCheck(const QVariant State);

// Catch keyboard shortcuts
void ptMainWindow::keyPressEvent(QKeyEvent *Event) {
  if (Event->key()==Qt::Key_Escape) {// back to used view
    if (Settings->GetInt("SpecialPreview") != ptSpecialPreview_RGB) {
        CB_SpecialPreviewChoice(ptSpecialPreview_RGB);
        return;
    } else if (SearchInputWidget->hasFocus()) {
      OnTabProcessingButtonClicked();
      return;
    } else if (Settings->GetInt("ShowToolContainer") == 0) {
        Settings->SetValue("ShowToolContainer", 1);
        UpdateSettings();
        return;
    } else if (Settings->GetInt("FullscreenActive") == 1) {
      ::CB_FullScreenButton(0);
      return;
    } else {
      if (Settings->GetInt("EscToExit") == 1) {
        ::CB_MenuFileExit(1);
      }
      return;
    }
  }

  if (SearchInputWidget->hasFocus() &&
      (Event->key()==Qt::Key_Return ||
       Event->key()==Qt::Key_Enter))
  {
    ViewWindow->setFocus();
    return;
  }

  if (Settings->GetInt("BlockTools") == 0 || ViewWindow->interaction() == iaCrop) {
    if (Event->key()==Qt::Key_F11) { // toggle full screen
      ::CB_FullScreenButton(!isFullScreen());
    } else if (Event->key()==Qt::Key_1 && Event->modifiers()==Qt::NoModifier) {
      CB_ZoomStep(1);
    } else if (Event->key()==Qt::Key_2 && Event->modifiers()==Qt::NoModifier) {
      CB_InputChanged("ZoomInput",100);
    } else if (Event->key()==Qt::Key_3 && Event->modifiers()==Qt::NoModifier) {
      CB_ZoomStep(-1);
    } else if (Event->key()==Qt::Key_4 && Event->modifiers()==Qt::NoModifier) {
      CB_ZoomFitButton();
    } else if (Event->key()==Qt::Key_Space) {
      Settings->SetValue("ShowToolContainer",1-Settings->GetInt("ShowToolContainer"));
      UpdateSettings();
      if (Settings->GetInt("ZoomMode") == ptZoomMode_Fit) {
        ViewWindow->ZoomToFit(0);
      }
    }
  }

  // most shortcuts should only work when we are not in special state like cropping
  if (Settings->GetInt("BlockTools")==0) {
    if (Event->key()==Qt::Key_F1) {
      QDesktopServices::openUrl(QString("http://photivo.org/photivo/manual"));
    } else if (Event->key()==Qt::Key_P && Event->modifiers()==Qt::NoModifier) {
      OnTabProcessingButtonClicked();
    } else if (Event->key()==Qt::Key_S && Event->modifiers()==Qt::NoModifier) {
      OnTabSettingsButtonClicked();
    } else if (Event->key()==Qt::Key_I && Event->modifiers()==Qt::NoModifier) {
      OnTabInfoButtonClicked();
    } else if (Event->key()==Qt::Key_M && Event->modifiers()==Qt::NoModifier) {
      ::CB_RunModeCheck(1-Settings->GetInt("RunMode"));
    } else if (Event->key()==Qt::Key_F5) {
      OnRunButtonClicked();
    } else if (Event->key()==Qt::Key_F3) {
      CB_SearchBarEnableCheck(1);
      if (!SearchInputWidget->hasFocus()) {
        SearchInputWidget->setText("");
        SearchInputWidget->setFocus();
      }
    } else if (Event->key() == Qt::Key_M && Event->modifiers() == Qt::ControlModifier) {
      SwitchUIState(uisFileMgr);
    } else if (Event->key() == Qt::Key_B && Event->modifiers() == Qt::ControlModifier) {
      SwitchUIState(uisBatch);
    } else if (Event->key()==Qt::Key_P && Event->modifiers()==Qt::ControlModifier) {
      CB_OpenPresetFileButton();
    } else if (Event->key()==Qt::Key_Q && Event->modifiers()==Qt::ControlModifier) {
      CB_MenuFileExit(1);
    } else if (Event->key()==Qt::Key_O && Event->modifiers()==Qt::ControlModifier) {
      CB_MenuFileOpen(0);
    } else if (Event->key()==Qt::Key_S && Event->modifiers()==Qt::ControlModifier) {
      CB_WritePipeButton();
    } else if (Event->key()==Qt::Key_G && Event->modifiers()==Qt::ControlModifier) {
      Update(ptProcessorPhase_ToGimp);
    } else if (Event->key()==Qt::Key_R && Event->modifiers()==Qt::ControlModifier) {
      resize(QSize(1200,900));
      QRect DesktopRect = (QApplication::desktop())->screenGeometry(this);
      move(QPoint(MAX((DesktopRect.width()-1200),0)/2,MAX((DesktopRect.height()-900)/2-20,0)));
    } else if (Event->key()==Qt::Key_1 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(0);
    } else if (Event->key()==Qt::Key_2 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(1);
    } else if (Event->key()==Qt::Key_3 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(2);
    } else if (Event->key()==Qt::Key_4 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(3);
    } else if (Event->key()==Qt::Key_5 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(4);
    } else if (Event->key()==Qt::Key_6 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(5);
    } else if (Event->key()==Qt::Key_7 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(6);
    } else if (Event->key()==Qt::Key_8 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(7);
    } else if (Event->key()==Qt::Key_9 && Event->modifiers()==Qt::AltModifier) {
      ProcessingTabBook->setCurrentIndex(8);
    } else if (Event->key()==Qt::Key_Period && Event->modifiers()==Qt::NoModifier) {
      ProcessingTabBook->setCurrentIndex(MIN(ProcessingTabBook->currentIndex()+1,ProcessingTabBook->count()));
    } else if (Event->key()==Qt::Key_Comma && Event->modifiers()==Qt::NoModifier) {
      ProcessingTabBook->setCurrentIndex(MAX(ProcessingTabBook->currentIndex()-1,0));
    } else if (Event->key()==Qt::Key_T && Event->modifiers()==Qt::NoModifier) {
      CB_PreviewModeButton(1-Settings->GetInt("PreviewTabMode"));
    } else if (Event->key()==Qt::Key_0 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_RGB)
        CB_SpecialPreviewChoice(ptSpecialPreview_RGB);
      else ViewWindowShowStatus(ptStatus_Done);
    } else if (Event->key()==Qt::Key_9 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_Structure)
        CB_SpecialPreviewChoice(ptSpecialPreview_Structure);
      else ViewWindowShowStatus(ptStatus_Done);
    } else if (Event->key()==Qt::Key_8 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_L)
        CB_SpecialPreviewChoice(ptSpecialPreview_L);
      else ViewWindowShowStatus(ptStatus_Done);
    } else if (Event->key()==Qt::Key_7 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_A)
        CB_SpecialPreviewChoice(ptSpecialPreview_A);
      else ViewWindowShowStatus(ptStatus_Done);
    } else if (Event->key()==Qt::Key_6 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_B)
        CB_SpecialPreviewChoice(ptSpecialPreview_B);
      else ViewWindowShowStatus(ptStatus_Done);
    } else if (Event->key()==Qt::Key_5 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_Gradient)
        CB_SpecialPreviewChoice(ptSpecialPreview_Gradient);
      else ViewWindowShowStatus(ptStatus_Done);
    } else if (Event->key()==Qt::Key_C && Event->modifiers()==Qt::NoModifier) {
      Settings->SetValue("ExposureIndicator",1-Settings->GetInt("ExposureIndicator"));
      Update(ptProcessorPhase_Preview);
    // hidden tools, needs GUI implementation
    } else if (Event->key()==Qt::Key_H && Event->modifiers()==Qt::NoModifier) {
      QString Tools = "";
      QString Tab = "";
      foreach (ptTempFilterBase* GroupBox, *m_GroupBox) {
        if (Settings->ToolIsHidden(GroupBox->objectName())) {
          Tab = ProcessingTabBook->tabText(GroupBox->parentTabIdx());
          Tools += Tab + ": " + GroupBox->caption() + "\n";
        }
      }
      if (Tools == "") Tools = tr("No tools hidden!");
      ptMessageBox::information(this,tr("Hidden tools"),Tools);
      /*findChild<QWidget *>(QString("TabGenCorrections"))->
        setVisible(1-findChild<QWidget *>(QString("TabGenCorrections"))->isVisible()); */
    } else if (Event->key()==Qt::Key_U && Event->modifiers()==Qt::NoModifier) {
      // show hidden tools on current tab
      m_ContextMenuOnTab = ProcessingTabBook->currentIndex();
      ShowToolsOnTab();
    } else if (Event->key()==Qt::Key_A && Event->modifiers()==Qt::NoModifier) {
      ShowActiveTools();
    } else if (Event->key()==Qt::Key_B && Event->modifiers()==Qt::NoModifier) {
      QString Tools = "";
      QString Tab = "";
      foreach (ptTempFilterBase* GroupBox, *m_GroupBox) {
        if (GroupBox->isBlocked()) {
          Tab = ProcessingTabBook->tabText(GroupBox->parentTabIdx());
          Tools += Tab + ": " + GroupBox->caption() + "\n";
        }
      }
      if (Tools == "") Tools = tr("No tools blocked!");
      ptMessageBox::information(this,tr("Blocked tools"),Tools);
// ATZ
    } else if (Event->key()==Qt::Key_R && Event->modifiers()==(Qt::ControlModifier | Qt::ShiftModifier)) {
      // Ctrl+Shift+R resets to default settings
      ptResetSettingsToDefault();
    } else if (Event->key()==Qt::Key_U && Event->modifiers()==(Qt::ControlModifier | Qt::ShiftModifier)) {
      // Ctrl+Shift+U resets to the last saved user settings
      ptMakeFullUndo();
    } else if (Event->key()==Qt::Key_Z && Event->modifiers()==Qt::ControlModifier) {
      // Ctrl+Z undo
      ptMakeUndo();
    } else if (Event->key()==Qt::Key_Y && Event->modifiers()==Qt::ControlModifier) {
      // Ctrl+Y redo
      ptMakeRedo();
    } else if (Event->key()==Qt::Key_Z && Event->modifiers()==(Qt::ControlModifier | Qt::ShiftModifier)) {
      // Ctrl+Shift+Z redo
      ptMakeRedo();
    } else if (Event->key()==Qt::Key_C && Event->modifiers()==(Qt::ControlModifier | Qt::ShiftModifier)) {
      // Ctrl+Shift+C copy settings
      ptCopySettingsToClipboard();
    } else if (Event->key()==Qt::Key_V && Event->modifiers()==(Qt::ControlModifier | Qt::ShiftModifier)) {
      // Ctrl+Shift+V paste settings
      ptPasteSettingsFromClipboard();
    } else if (Event->key()==Qt::Key_Left && Event->modifiers()==Qt::NoModifier) {
      // LeftArrow go to previous image
      CB_PreviousImageButton();
    } else if (Event->key()==Qt::Key_Right && Event->modifiers()==Qt::NoModifier) {
      // RightArrow go to next image
      CB_NextImageButton();
    } else if (Event->key()==Qt::Key_Plus) {
      // + increase exposure +0.3Ev
      ptAddUndo();
      Settings->SetValue("Exposure", Settings->GetDouble("Exposure") + 0.3);
      Update(ptProcessorPhase_RGB);
    } else if (Event->key()==Qt::Key_Minus) {
      // - decrease exposure -0.3Ev
      ptAddUndo();
      Settings->SetValue("Exposure", Settings->GetDouble("Exposure") - 0.3);
      Update(ptProcessorPhase_RGB);
    } else if (Event->key()==Qt::Key_Asterisk) {
      ptSwitchAB();
// end ATZ
    }
  }
}

void ptMainWindow::wheelEvent(QWheelEvent * Event) {
  if (Event->delta() < 0 && ((QMouseEvent*)Event)->modifiers()==Qt::AltModifier) {
    ProcessingTabBook->setCurrentIndex(MIN(ProcessingTabBook->currentIndex()+1,ProcessingTabBook->count()));
  } else if (Event->delta() > 0 && ((QMouseEvent*)Event)->modifiers()==Qt::AltModifier) {
    ProcessingTabBook->setCurrentIndex(MAX(ProcessingTabBook->currentIndex()-1,0));
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Search and moved tools
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::OnSearchResetButtonClicked() {
  OnTabProcessingButtonClicked();
}

//==============================================================================

void ptMainWindow::OnSearchActiveToolsButtonClicked() {
  ShowActiveTools();
}

//==============================================================================

void ptMainWindow::OnSearchAllToolsButtonClicked() {
  ShowAllTools();
}

//==============================================================================

void ptMainWindow::OnSearchFavouriteToolsButtonClicked() {
  ShowFavouriteTools();
}

//==============================================================================

void ptMainWindow::StartSearchTimer(QString) {
  m_SearchInputTimer->start(50);
}

//==============================================================================

void ptMainWindow::Search() {
  const QString hSearchString = SearchInputWidget->text();

  if (hSearchString == "") {
    OnTabProcessingButtonClicked();
    return;
  }

  // apply changes at VisibleTools box
  if (MainTabBook->currentWidget() == TabSetting)
    ApplyVisibleTools();

  // clean up first!
  if (m_MovedTools->size()>0)
    CleanUpMovedTools();

  // iterate through toolboxes and remember those whose caption matches the search text
  for (QString hBoxName: *m_GroupBoxesOrdered) {
    ptTempFilterBase *hBox = m_GroupBox->value(hBoxName);

    // remember matching toolboxes
    if (hBox->caption().contains(hSearchString, Qt::CaseInsensitive)) {
      m_MovedTools->append(hBox->guiWidget());
    }
  }

  if (m_MovedTools->size() > 0)
    ShowMovedTools(tr("Search results:"));
}

//==============================================================================

void ptMainWindow::ShowActiveTools() {
  // apply changes at VisibleTools box
  if (MainTabBook->currentWidget() == TabSetting)
    ApplyVisibleTools();

  // clean up first!
  if (m_MovedTools->size()>0) CleanUpMovedTools();
  SearchInputWidget->setText("");

  for (QString hBoxName: *m_GroupBoxesOrdered) {
    ptTempFilterBase *hBox = m_GroupBox->value(hBoxName);

    if (hBox->isActive())
      m_MovedTools->append(hBox->guiWidget());
  }

  if (m_MovedTools->size() == 0) {
    ShowMovedTools(tr("No tools active!"));
  } else {
    ShowMovedTools(tr("Active tools:"));
  }
}

//==============================================================================

void ptMainWindow::ShowAllTools() {
  // apply changes at VisibleTools box
  if (MainTabBook->currentWidget() == TabSetting)
    ApplyVisibleTools();

  // clean up first!
  if (m_MovedTools->size()>0) CleanUpMovedTools();
  SearchInputWidget->setText("");

  auto hHiddenTools = Settings->GetStringList("HiddenTools");
  for (QString hBoxName: *m_GroupBoxesOrdered) {
    if (!hHiddenTools.contains(hBoxName)) {
      m_MovedTools->append(m_GroupBox->value(hBoxName)->guiWidget());
    }
  }

  if (m_MovedTools->size() == 0) {
    ShowMovedTools(tr("No tools visible!"));
  } else {
    ShowMovedTools(tr("All visible tools:"));
  }
}

//==============================================================================

void ptMainWindow::ShowFavouriteTools() {
  // apply changes at VisibleTools box
  if (MainTabBook->currentWidget() == TabSetting)
    ApplyVisibleTools();

  // clean up first!
  if (m_MovedTools->size()>0) CleanUpMovedTools();
  SearchInputWidget->setText("");

  auto hFavTools = Settings->GetStringList("FavouriteTools");
  for (QString hBoxName: *m_GroupBoxesOrdered) {
    if (hFavTools.contains(hBoxName)) {
      m_MovedTools->append(m_GroupBox->value(hBoxName)->guiWidget());
    }
  }

  if (m_MovedTools->size() == 0) {
    ptMessageBox::information(this,tr("Favourite tools"),tr("No favourite tools!"));
    return;
  }

  ShowMovedTools(tr("Favourite tools:"));
}

//==============================================================================

void ptMainWindow::ShowMovedTools(const QString ATitle) {
  ToolContainerLabel->setText(ATitle);
  MainTabBook->setCurrentWidget(TabMovedTools);

  // clear previous contents from the moved tools page
  while (ToolContainer->layout()->count()!=0) {
    ToolContainer->layout()->takeAt(0);
  }

  // add new tools from moved tools list
  for (short i=0; i<m_MovedTools->size(); i++) {
    ToolContainer->layout()->addWidget(m_MovedTools->at(i));
  }

  static_cast<QVBoxLayout*>(ToolContainer->layout())->addStretch();
  static_cast<QVBoxLayout*>(ToolContainer->layout())->setSpacing(0);
  static_cast<QVBoxLayout*>(ToolContainer->layout())->setContentsMargins(0,0,0,0);
}

//==============================================================================

void ptMainWindow::CleanUpMovedTools() {
  for (QWidget *hToolBox: *m_MovedTools) {
    int hTabIdx   = -1;
    int hIdxInTab = -1;
    if (QString(hToolBox->metaObject()->className()) == "ptToolBox") {
      auto hFilt = GFilterDM->GetFilterFromName(hToolBox->objectName());
      hTabIdx    = hFilt->parentTabIdx();
      hIdxInTab  = hFilt->idxInParentTab();
    } else if (QString(hToolBox->metaObject()->className()) == "ptGroupBox") {
      hTabIdx   = ((ptGroupBox*)hToolBox)->GetTabNumber();
      hIdxInTab = ((ptGroupBox*)hToolBox)->GetIndexInTab();
    }

    m_TabLayouts->at(hTabIdx)->insertWidget(hIdxInTab, hToolBox);
  }

  m_MovedTools->clear();
}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateToolBoxes
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateToolBoxes() {
  foreach (ptTempFilterBase* GroupBox, *m_GroupBox) {
    auto hGroupBox = qobject_cast<ptGroupBox*>(GroupBox);
    if (!hGroupBox) continue;

    if (Settings->ToolIsHidden(hGroupBox->objectName())) {
      hGroupBox->hide();
    } else {
      hGroupBox->show();
    }
    hGroupBox->Update();
  }

  // disable Raw tools when we have a bitmap
  QList<ptGroupBox *> m_RawTools;
  m_RawTools << static_cast<ptGroupBox*>(m_GroupBox->value("TabCameraColorSpace"))
             << static_cast<ptGroupBox*>(m_GroupBox->value("TabGenCorrections"))
             << static_cast<ptGroupBox*>(m_GroupBox->value("TabWhiteBalance"))
             << static_cast<ptGroupBox*>(m_GroupBox->value("TabDemosaicing"))
             << static_cast<ptGroupBox*>(m_GroupBox->value("TabHighlightRecovery"));
  bool hTemp = Settings->useRAWHandling();
  for (int i = 0; i < m_RawTools.size(); i++) {
    m_RawTools.at(i)->SetEnabled(hTemp);
  }

  // disable tools when we are in detail view
  QList<ptGroupBox *> m_DetailViewTools;
  m_DetailViewTools << static_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunCA"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunVignette"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunDistortion"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabLensfunGeometry"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabDefish"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabCrop"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabLiquidRescale"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabResize"))
                    << static_cast<ptGroupBox*>(m_GroupBox->value("TabWebResize"));

  hTemp = !(Settings->GetInt("DetailViewActive") == 1);
  for (int i = 0; i < m_DetailViewTools.size(); i++) {
    m_DetailViewTools.at(i)->SetEnabled(hTemp);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateSettings()
//
// Adapt the Gui elements according to GuiSettings
// (for those gui elements that should be updated after a programmatic
// recalculation)
//
////////////////////////////////////////////////////////////////////////////////

// #define SHOW_BT
#ifdef SHOW_BT
extern "C" {
  #include <execinfo.h>
}
#endif

void ptMainWindow::UpdateSettings() {

  #ifdef SHOW_BT

    {
    void *array[10];
    size_t size;
    char **strings;
    size_t i;
    size = backtrace(array,10);
    strings = backtrace_symbols (array, size);
    printf ("Obtained %zd stack frames.\n", size);
    for (i = 0; i < size; i++)
      printf ("%s\n", strings[i]);
    free (strings);
    }
  #endif

  // State of Groupboxes
  foreach (ptTempFilterBase* GroupBox, *m_GroupBox) {
    auto hGroupBox = qobject_cast<ptGroupBox*>(GroupBox);
    if (hGroupBox)
      hGroupBox->SetActive(Settings->ToolIsActive(hGroupBox->objectName()));
  }

  // Status LED on tabs
  if(Settings->GetInt("TabStatusIndicator")) {
    ProcessingTabBook->setIconSize(QSize(Settings->GetInt("TabStatusIndicator"),
                                         Settings->GetInt("TabStatusIndicator")));
    QList <ptGroupBox *> Temp;
    for (int j=0; j<m_ActiveTabs.size();j++) {
      bool hIsActiveTab = GFilterDM->isActiveCacheGroup(j+1);  // +1 because Camera tab is not
      if (!hIsActiveTab) {                                     // in m_ActiveTabs
        Temp = m_ActiveTabs.at(j)->findChildren <ptGroupBox *> ();
        for (int i=0; i<Temp.size();i++) {
          if(Settings->ToolIsActive(Temp.at(i)->objectName())) {
            hIsActiveTab = true;
            break;
          }
        }
      }

      if (hIsActiveTab)
        ProcessingTabBook->setTabIcon(ProcessingTabBook->indexOf(m_ActiveTabs.at(j)),m_StatusIcon);
      else
        ProcessingTabBook->setTabIcon(ProcessingTabBook->indexOf(m_ActiveTabs.at(j)),QIcon());
    }
  } else {
    for (int j=0; j<m_ActiveTabs.size();j++)
      ProcessingTabBook->setTabIcon(ProcessingTabBook->indexOf(m_ActiveTabs.at(j)),QIcon());
  }


  // New Camera Color Space or profile ?
  int TmpCameraColor = Settings->GetInt("CameraColor");
  if (TmpCameraColor == ptCameraColor_Adobe_Profile) {
    Settings->SetEnabled("CameraColorProfileIntent",1);
    Settings->SetEnabled("CameraColorGamma",0);
    CameraColorProfileText->setEnabled(1);
  } else if (TmpCameraColor == ptCameraColor_Profile) {
    Settings->SetEnabled("CameraColorProfileIntent",1);
    Settings->SetEnabled("CameraColorGamma",1);
    CameraColorProfileText->setEnabled(1);
  } else if (TmpCameraColor == ptCameraColor_Flat) {
    Settings->SetEnabled("CameraColorProfileIntent",1);
    Settings->SetEnabled("CameraColorGamma",1);
    CameraColorProfileText->setEnabled(1);
  } else {
    Settings->SetEnabled("CameraColorProfileIntent",0);
    Settings->SetEnabled("CameraColorGamma",0);
    CameraColorProfileText->setEnabled(0);
  }

  Settings->SetEnabled("CaRed",Settings->GetInt("CaCorrect")==ptCACorrect_Manual);
  Settings->SetEnabled("CaBlue",Settings->GetInt("CaCorrect")==ptCACorrect_Manual);

  QFileInfo PathInfo(Settings->GetString("CameraColorProfile"));
  QString ShortFileName = PathInfo.baseName();
  CameraColorProfileText->setText(ShortFileName);
  // End CameraColorChoice.


  // Preview Color Profile.
  PathInfo.setFile(Settings->GetString("PreviewColorProfile"));
  ShortFileName = PathInfo.baseName();
  PreviewColorProfileText->setText(ShortFileName);

  bool Temp = Settings->GetInt("CMQuality") != ptCMQuality_FastSRGB;
  Settings->SetEnabled("PreviewColorProfileIntent", Temp);
  PreviewProfileWidget->setEnabled(Temp);
  // End Preview Color Profile.

  // Preview Color Profile.
  PathInfo.setFile(Settings->GetString("OutputColorProfile"));
  ShortFileName = PathInfo.baseName();
  OutputColorProfileText->setText(ShortFileName);
  // End Preview Color Profile.

  // GimpExecCommand
  PathInfo.setFile(Settings->GetString("GimpExecCommand"));
  ShortFileName = PathInfo.fileName();
  GimpExecCommandText->setText(ShortFileName);
  // GimpExecCommand

  // StartupSettings
  PathInfo.setFile(Settings->GetString("StartupSettingsFile"));
  ShortFileName = PathInfo.baseName();
  StartupSettingsText->setText(ShortFileName);
  // StartupSettings

  // New BlackPoint ?
  Settings->SetEnabled("BlackPoint",Settings->GetInt("ManualBlackPoint"));
  // End BlackPoint

  // New WhitePoint ?
  Settings->SetEnabled("WhitePoint",Settings->GetInt("ManualWhitePoint"));
  // End WhitePoint

  // New BadPixels ?
  if (Settings->GetInt("HaveBadPixels") == 2) {
    // Small routine that lets Shortfilename point to the basename.
    QFileInfo PathInfo(Settings->GetString("BadPixelsFileName"));
    QString ShortFileName = PathInfo.baseName();
    Settings->AddOrReplaceOption("BadPixels",ShortFileName,QVariant(2));
  }
  Settings->SetValue("BadPixels",Settings->GetInt("HaveBadPixels"));
  // End BadPixels.

  // New DarkFrame ?
  if (Settings->GetInt("HaveDarkFrame") == 2) {
    // Small routine that lets Shortfilename point to the basename.
    QFileInfo PathInfo(Settings->GetString("DarkFrameFileName"));
    QString ShortFileName = PathInfo.baseName();
    Settings->AddOrReplaceOption("DarkFrame",ShortFileName,QVariant(2));
  }
  Settings->SetValue("DarkFrame",Settings->GetInt("HaveDarkFrame"));
  // End DarkFrame.

  // New White balances,due to photo from other camera ?
  // Also the CameraMake model for lensfun is adapted.
  if (m_CameraMake != Settings->GetString("CameraMake")
      ||
      m_CameraModel != Settings->GetString("CameraModel") ) {

    m_CameraMake  = Settings->GetString("CameraMake");
    m_CameraModel = Settings->GetString("CameraModel");
    // New white balances !
    // First clear the whole WhiteBalanceChoice
    Settings->ClearOptions("WhiteBalance", 1);

    // Add the camera dependent custom ones.
    // First calculate which index our camera has int ptWhiteBalances.
    short WBIdx = 4; // To jump over the first 4 standard ones.
    while ( !ptWhiteBalances[WBIdx].m_Make.isEmpty()
            &&
            ( ptWhiteBalances[WBIdx].m_Make  != m_CameraMake
              ||
              ptWhiteBalances[WBIdx].m_Model != m_CameraModel
            )
          ) {
      WBIdx++;
    }
    // And then add the custom whitebalances.
    while (ptWhiteBalances[WBIdx].m_Make  == m_CameraMake
           &&
           ptWhiteBalances[WBIdx].m_Model == m_CameraModel){
      Settings->AddOrReplaceOption("WhiteBalance",
                                   ptWhiteBalances[WBIdx].m_Name,
                                   QVariant(WBIdx));
      WBIdx++;
    }
  }

  // Setting for interpolation passes only on DCB needed

  if (Settings->GetInt("Interpolation")==ptInterpolation_DCB ||
      Settings->GetInt("Interpolation")==ptInterpolation_DCBSoft ||
      Settings->GetInt("Interpolation")==ptInterpolation_DCBSharp)
    Settings->SetEnabled("InterpolationPasses",1);
  else
    Settings->SetEnabled("InterpolationPasses",0);


  // ChannelMixer
  Settings->SetValue("ChannelMixerR2R",ChannelMixer->m_Mixer[0][0]);
  Settings->SetValue("ChannelMixerG2R",ChannelMixer->m_Mixer[0][1]);
  Settings->SetValue("ChannelMixerB2R",ChannelMixer->m_Mixer[0][2]);
  Settings->SetValue("ChannelMixerR2G",ChannelMixer->m_Mixer[1][0]);
  Settings->SetValue("ChannelMixerG2G",ChannelMixer->m_Mixer[1][1]);
  Settings->SetValue("ChannelMixerB2G",ChannelMixer->m_Mixer[1][2]);
  Settings->SetValue("ChannelMixerR2B",ChannelMixer->m_Mixer[2][0]);
  Settings->SetValue("ChannelMixerG2B",ChannelMixer->m_Mixer[2][1]);
  Settings->SetValue("ChannelMixerB2B",ChannelMixer->m_Mixer[2][2]);

  // Preview Mode
  PreviewModeButton->setChecked(Settings->GetInt("PreviewMode"));

  // Run mode
  RunButton->setEnabled(Settings->GetInt("RunMode"));

  // PizeSize Widget
  PipeSizeWidget->setEnabled(1-
    (Settings->GetInt("AutomaticPipeSize") &&
     Settings->ToolIsActive("TabResize")));

  // Show containers
  BottomContainer->setVisible(Settings->GetInt("ShowBottomContainer"));
  ControlFrame->setVisible(Settings->GetInt("ShowToolContainer"));

  // Geometry
  ResizeHeightWidget->setVisible(Settings->GetInt("ResizeDimension") == ptResizeDimension_WidthHeight);

  // Exposure
  if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Auto) {
    Settings->SetEnabled("WhiteFraction",1);
    Settings->SetEnabled("WhiteLevel",1);
  } else {
    Settings->SetEnabled("WhiteFraction",0);
    Settings->SetEnabled("WhiteLevel",0);
  }

  // Black and White
  if (Settings->GetInt("BWStylerFilmType") == ptFilmType_ChannelMixer) {
    Settings->SetEnabled("BWStylerMultR", 1);
    Settings->SetEnabled("BWStylerMultG", 1);
    Settings->SetEnabled("BWStylerMultB", 1);
  } else {
    Settings->SetEnabled("BWStylerMultR", 0);
    Settings->SetEnabled("BWStylerMultG", 0);
    Settings->SetEnabled("BWStylerMultB", 0);
  }

  // Texture Overlay
  PathInfo.setFile(Settings->GetString("TextureOverlayFile"));
  ShortFileName = PathInfo.completeBaseName();
  TextureOverlayText->setText(ShortFileName);
  if (Settings->GetInt("TextureOverlayMask") > 0) {
    Settings->SetEnabled("TextureOverlayExponent",1);
    Settings->SetEnabled("TextureOverlayInnerRadius",1);
    Settings->SetEnabled("TextureOverlayOuterRadius",1);
    Settings->SetEnabled("TextureOverlayRoundness",1);
    Settings->SetEnabled("TextureOverlayCenterX",1);
    Settings->SetEnabled("TextureOverlayCenterY",1);
    Settings->SetEnabled("TextureOverlaySoftness",1);
  } else {
    Settings->SetEnabled("TextureOverlayExponent",0);
    Settings->SetEnabled("TextureOverlayInnerRadius",0);
    Settings->SetEnabled("TextureOverlayOuterRadius",0);
    Settings->SetEnabled("TextureOverlayRoundness",0);
    Settings->SetEnabled("TextureOverlayCenterX",0);
    Settings->SetEnabled("TextureOverlayCenterY",0);
    Settings->SetEnabled("TextureOverlaySoftness",0);
  }
  PathInfo.setFile(Settings->GetString("TextureOverlay2File"));
  ShortFileName = PathInfo.completeBaseName();
  TextureOverlay2Text->setText(ShortFileName);
  if (Settings->GetInt("TextureOverlay2Mask") > 0) {
    Settings->SetEnabled("TextureOverlay2Exponent",1);
    Settings->SetEnabled("TextureOverlay2InnerRadius",1);
    Settings->SetEnabled("TextureOverlay2OuterRadius",1);
    Settings->SetEnabled("TextureOverlay2Roundness",1);
    Settings->SetEnabled("TextureOverlay2CenterX",1);
    Settings->SetEnabled("TextureOverlay2CenterY",1);
    Settings->SetEnabled("TextureOverlay2Softness",1);
  } else {
    Settings->SetEnabled("TextureOverlay2Exponent",0);
    Settings->SetEnabled("TextureOverlay2InnerRadius",0);
    Settings->SetEnabled("TextureOverlay2OuterRadius",0);
    Settings->SetEnabled("TextureOverlay2Roundness",0);
    Settings->SetEnabled("TextureOverlay2CenterX",0);
    Settings->SetEnabled("TextureOverlay2CenterY",0);
    Settings->SetEnabled("TextureOverlay2Softness",0);
  }

  // Color buttons
  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("Tone1ColorRed"));
  Color.setGreen(Settings->GetInt("Tone1ColorGreen"));
  Color.setBlue(Settings->GetInt("Tone1ColorBlue"));
  Pix.fill(Color);
  Tone1ColorButton->setIcon(Pix);
  Color.setRed(Settings->GetInt("Tone2ColorRed"));
  Color.setGreen(Settings->GetInt("Tone2ColorGreen"));
  Color.setBlue(Settings->GetInt("Tone2ColorBlue"));
  Pix.fill(Color);
  Tone2ColorButton->setIcon(Pix);
  Color.setRed(Settings->GetInt("GradualOverlay1ColorRed"));
  Color.setGreen(Settings->GetInt("GradualOverlay1ColorGreen"));
  Color.setBlue(Settings->GetInt("GradualOverlay1ColorBlue"));
  Pix.fill(Color);
  GradualOverlay1ColorButton->setIcon(Pix);
  Color.setRed(Settings->GetInt("GradualOverlay2ColorRed"));
  Color.setGreen(Settings->GetInt("GradualOverlay2ColorGreen"));
  Color.setBlue(Settings->GetInt("GradualOverlay2ColorBlue"));
  Pix.fill(Color);
  GradualOverlay2ColorButton->setIcon(Pix);

  UpdateGradualBlurUI();

  // sRGB gamma compensation
  Settings->SetEnabled("OutputGamma",Settings->GetInt("OutputGammaCompensation"));
  Settings->SetEnabled("OutputLinearity",Settings->GetInt("OutputGammaCompensation"));

  // Save options
  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG)
    Settings->SetEnabled("SaveSampling",1);
  else
    Settings->SetEnabled("SaveSampling",0);

  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG ||
      Settings->GetInt("SaveFormat")==ptSaveFormat_PNG ||
      Settings->GetInt("SaveFormat")==ptSaveFormat_PNG16)
    Settings->SetEnabled("SaveQuality",1);
  else
    Settings->SetEnabled("SaveQuality",0);

  // Statusbar : photo
  QStringList InputFileNameList = Settings->GetStringList("InputFileNameList");
  if (InputFileNameList.count() > 0) {
    PathInfo.setFile(InputFileNameList[0]);
    ShortFileName = PathInfo.fileName();

    QString Tmp = "";
    QString Report = "";
    Report += Tmp.setNum(Settings->GetInt("ImageW"));
    Report += " x ";
    Report += Tmp.setNum(Settings->GetInt("ImageH"));
    Report += "     ";
    SizeFullLabel->setText(Report);

    int TmpScaled = Settings->GetInt("Scaled");

    Report = "";
    Report += Tmp.setNum(Settings->GetInt("PipeImageW") << TmpScaled);
    Report += " x ";
    Report += Tmp.setNum(Settings->GetInt("PipeImageH") << TmpScaled);
    Report += "     ";
    SizeFullCropLabel->setText(Report);

    Report = "";
    Report += Tmp.setNum(Settings->GetInt("PipeImageW"));
    Report += " x ";
    Report += Tmp.setNum(Settings->GetInt("PipeImageH"));
    Report += "     ";
    SizeCurrentLabel->setText(Report);

  } else {
    // Startup.

  }
}

//==============================================================================
// Display strings from settings
void ptMainWindow::Settings_2_Form() {
  if (Settings->GetInt("JobMode") == 1) return;

  // Metadata
  edtOutputSuffix->setText(Settings->GetString("OutputFileNameSuffix"));
  edtImageTitle->setText(Settings->GetString("ImageTitle"));
  edtCopyright->setText( Settings->GetString("Copyright"));
// ATZ
  StarRating1->setStarCount(Settings->GetInt("ImageRating"));
  ColorLabel1->setSelectedLabel(Settings->GetInt("ColorLabel"));
// end ATZ
}

//==============================================================================
// Read settings from Form
void ptMainWindow::Form_2_Settings() {
  if (Settings->GetInt("JobMode") == 1) return;

// ATZ
  ptAddUndo();
  Settings->SetValue("ImageRating", StarRating1->starCount());
  Settings->SetValue("ColorLabel", ColorLabel1->selectedLabel());
// end ATZ

  //Metadata
  Settings->SetValue("OutputFileNameSuffix", edtOutputSuffix->text().trimmed());
  Settings->SetValue("ImageTitle", edtImageTitle->text().trimmed());
  Settings->SetValue("Copyright",  edtCopyright->text().trimmed());
}

////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateFilenameInfo(const QStringList FileNameList) {
  QFileInfo fn(FileNameList[0]);
  if (FileNameList.length() > 0 ) {
    FileNameLabel->setText(fn.fileName());
    #ifdef Q_OS_WIN
      FilePathLabel->setText(fn.canonicalPath().replace(QString("/"), QString("\\")));
    #else
      FilePathLabel->setText(fn.canonicalPath());
    #endif
  } else {
    FileNameLabel->setText("");
    FilePathLabel->setText("");
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// UpdateExifInfo()
// Update the exif info gui element.
// TODO Optimize : slow.
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateExifInfo(Exiv2::ExifData ExifData) {

  // Exif info for the info window (the previous infowindow)

  Exiv2::ExifData::iterator Pos;
  QString TheInfo;
  QString TempString = "";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Image.Make"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  while (TheInfo.endsWith(" ")) TheInfo.chop(1);
  TempString = TheInfo +": ";
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Image.Model"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  TempString = TempString + TheInfo;
  InfoCameraLabel->setText(TempString);
  TheInfo="";

  // Idea from UFRaw
  if (ExifData.findKey(Exiv2::ExifKey("Exif.Image.LensInfo")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Image.LensInfo"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.Nikon3.LensData")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Nikon3.LensData"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.Nikon3.Lens")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Nikon3.Lens"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.CanonCs.LensType")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.CanonCs.LensType"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
#endif
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.Canon.0x0095")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Canon.0x0095"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.Minolta.LensID")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Minolta.LensID"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
#if EXIV2_TEST_VERSION(0,16,0)
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.Pentax.LensType")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Pentax.LensType"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
#endif
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.OlympusEq.LensType")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.OlympusEq.LensType"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  } else if (ExifData.findKey(Exiv2::ExifKey("Exif.Panasonic.LensType")) != ExifData.end() ) {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Panasonic.LensType"));
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  InfoLensLabel->setText(TheInfo);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  InfoTimeLabel->setText(TheInfo);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  } else {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue"));
    if (Pos != ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      TheInfo.append(QString(str.str().c_str()));
    }
  }
  TempString = TheInfo + tr(" at ");
  TheInfo="";

  Settings->SetValue("ApertureFromExif", 0.0);
  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    Settings->SetValue("ApertureFromExif", QString(str.str().c_str()).remove("F", Qt::CaseInsensitive).toDouble());
    TheInfo.append(QString(str.str().c_str()));
  } else {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue"));
    if (Pos != ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      Settings->SetValue("ApertureFromExif", QString(str.str().c_str()).remove("F", Qt::CaseInsensitive).toDouble());
      TheInfo.append(QString(str.str().c_str()));
    }
  }
  TempString = TempString + TheInfo + tr(" with ISO ");
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  if (TheInfo == "" || TheInfo == " ") TheInfo = "NN";
  TempString = TempString + TheInfo;
  InfoExposureLabel->setText(TempString);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  if (TheInfo != "" && TheInfo != " ") {
    // save focal length (need 35mm equiv.)
    QString FL = TheInfo;
    while(FL.endsWith("m") || FL.endsWith(" ")) FL.chop(1);
    if (FL.toFloat() >= 4.0f && FL.toFloat() <= 6000.0f)
      Settings->SetValue("FocalLengthIn35mmFilm",FL.toFloat());
    else
      Settings->SetValue("FocalLengthIn35mmFilm",50.0);
  } else {
    Settings->SetValue("FocalLengthIn35mmFilm",50.0);
  }
  TempString = TheInfo;
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLengthIn35mmFilm"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  if (TheInfo != "" && TheInfo != " " && TheInfo != TempString ) {
    TempString = TempString + tr(" (35mm equiv.: ") + TheInfo + ")";
    // save focal length (crop camera!)
    QString FL = TheInfo;
    while(FL.endsWith("m") || FL.endsWith(" ")) FL.chop(1);
    if (FL.toFloat() >= 4.0f && FL.toFloat() <= 6000.0f)
      Settings->SetValue("FocalLengthIn35mmFilm",FL.toFloat());
    else
      Settings->SetValue("FocalLengthIn35mmFilm",50.0);
  }
  InfoFocalLengthLabel->setText(TempString);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.Flash"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString::fromLocal8Bit(str.str().c_str()));
  }
  InfoFlashLabel->setText(TheInfo);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.WhiteBalance"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString::fromLocal8Bit(str.str().c_str()));
  }
  InfoWhitebalanceLabel->setText(TheInfo);
  TheInfo="";

  // dcraw Data
  TheInfo = Settings->GetString("CameraMake") + ": " + Settings->GetString("CameraModel");
  InfoDcrawLabel->setText(TheInfo);
}

//==============================================================================


////////////////////////////////////////////////////////////////////////////////
//
// UpdateCropToolUI
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateCropToolUI() {
  bool OnOff = false;
  if (ViewWindow != NULL) {   // when called from MainWindow constructor, ViewWindow doesn't yet exist
    if (ViewWindow->interaction() == iaCrop) {
      OnOff = true;
    }
  }
  CropWidget->setEnabled(!OnOff);
  MakeCropButton->setVisible(!OnOff);
  ConfirmCropButton->setVisible(OnOff);
  CancelCropButton->setVisible(OnOff);
  CropCenterHorButton->setVisible(OnOff);
  CropCenterVertButton->setVisible(OnOff);

  if (Settings->GetInt("FixedAspectRatio") != 0) {
    AspectRatioWLabel->setEnabled(true);
    AspectRatioHLabel->setEnabled(true);
    AspectRatioWWidget->setEnabled(true);
    AspectRatioHWidget->setEnabled(true);
  } else {
    AspectRatioWLabel->setEnabled(false);
    AspectRatioHLabel->setEnabled(false);
    AspectRatioWWidget->setEnabled(false);
    AspectRatioHWidget->setEnabled(false);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// InitVisibleTools
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::InitVisibleTools() {
  m_VisibleToolsModel = new ptVisibleToolsModel;

  // fill items corresponding to tabs
  for (int i=0; i < ProcessingTabBook->count(); i++) {
    QStandardItem *topItem = new QStandardItem(ProcessingTabBook->tabText(i));
    topItem->setFlags(Qt::NoItemFlags);
    m_VisibleToolsModel->appendRow(topItem);
  }

  QStringList hiddenTools = Settings->GetStringList("HiddenTools");
  QStringList favouriteTools = Settings->GetStringList("FavouriteTools");

  foreach (QString hBoxName, *m_GroupBoxesOrdered) {
    auto hFilt = m_GroupBox->value(hBoxName);
    auto item  = new QStandardItem(hFilt->caption());

    // set tool state and corresponding icon will be set automatically
    if (hiddenTools.contains(hBoxName))
      item->setData(tsHidden, Qt::UserRole+1);
    else
      if (favouriteTools.contains(hBoxName))
        item->setData(tsFavourite, Qt::UserRole+1);
      else
        item->setData(tsNormal, Qt::UserRole+1);

    // check if tool can be hidden and set corresponding flag
    if (hFilt->canHide())
      item->setData(0, Qt::UserRole+2);
    else
      item->setData(1, Qt::UserRole+2);

    m_VisibleToolsModel->item(hFilt->parentTabIdx())->appendRow(item);
  }

  // connect model with Visible Tools View
  VisibleToolsView->setModel(m_VisibleToolsModel);
  VisibleToolsView->setEditTriggers(QAbstractItemView::CurrentChanged |
                                    QAbstractItemView::DoubleClicked);
  VisibleToolsView->setItemDelegate(new ptVisibleToolsItemDelegate);
}

//==============================================================================

void ptMainWindow::SwitchUIState(const ptUIState AState)
{
  if (FUIState == AState) return;

// ATZ
  if (FUIState == uisProcessing && Settings->GetInt("HaveImage") == 1) {
    ptConfirmRequest::saveImage();
  }
// end ATZ

  FUIState = AState;

  switch (AState) {
    case uisProcessing:
      // Processing
      MainStack->setCurrentWidget(ProcessingPage);
      Settings->SetValue("FileMgrIsOpen", 0);
      Settings->SetValue("BatchIsOpen", 0);
      break;
    case uisFileMgr:
      // Filemanager
  #ifndef PT_WITHOUT_FILEMGR
      MainStack->setCurrentWidget(FileManagerPage);
      Settings->SetValue("FileMgrIsOpen", 1);
  #endif
      break;
    case uisBatch:
      MainStack->setCurrentWidget(BatchPage);
      Settings->SetValue("BatchIsOpen", 1);
      break;
    default:
      GInfo->Raise("Unknown UI state", AT);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// ApplyVisibleTools
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::ApplyVisibleTools() {
  QString FirstActive;

  foreach (QString hBoxName, *m_GroupBoxesOrdered) {
    auto hFilt    = m_GroupBox->value(hBoxName);
    auto hToolBox = hFilt->guiWidget();

    // find all items with necessary name (in different tabs)
    QList<QStandardItem *> itemList =
        m_VisibleToolsModel->findItems(hFilt->caption(), Qt::MatchExactly | Qt::MatchRecursive);

    // find item corresponding to proper tab (maybe their is a simplier way)
    QStandardItem *item = NULL;
    foreach (QStandardItem *it, itemList) {
      QString tabName = ProcessingTabBook->tabText(hFilt->parentTabIdx());
      if (m_VisibleToolsModel->indexFromItem(it).parent() ==
          m_VisibleToolsModel->indexFromItem(
              m_VisibleToolsModel->findItems(tabName, Qt::MatchExactly).first()) )
      {
        item = it;
        break;
      }
    }

    // hide/show tools according to state
    if (item->data(Qt::UserRole+1).toInt() == tsHidden) {
      QStringList hiddenTools = Settings->GetStringList("HiddenTools");
      // hide tool if it was visible
      if (!hiddenTools.contains(hBoxName)) {
        hiddenTools.append(hBoxName);
        hToolBox->hide();
        // find first active tool, which changes it's state
        if (hFilt->isActive() && FirstActive.size() == 0)
          FirstActive = hBoxName;
        Settings->SetValue("HiddenTools", hiddenTools);
      }

      QStringList favouriteTools = Settings->GetStringList("FavouriteTools");
      if (favouriteTools.contains(hBoxName)) {
        favouriteTools.removeAll(hBoxName);
        Settings->SetValue("FavouriteTools", favouriteTools);
      }
    }

    if (item->data(Qt::UserRole+1).toInt() == tsFavourite) {
      QStringList favouriteTools = Settings->GetStringList("FavouriteTools");
      if (!favouriteTools.contains(hBoxName)) {
        favouriteTools.append(hBoxName);
        Settings->SetValue("FavouriteTools", favouriteTools);
      }

      QStringList hiddenTools = Settings->GetStringList("HiddenTools");
      // show tool if it was hidden
      if (hiddenTools.contains(hBoxName)) {
        hiddenTools.removeAll(hBoxName);
        hToolBox->show();
        Settings->SetValue("HiddenTools", hiddenTools);
        // find first active tool, which changes it's state
        if (hFilt->isActive() && FirstActive.size() == 0)
          FirstActive = hBoxName;
      }
    }

    if (item->data(Qt::UserRole+1).toInt() == tsNormal) {
      QStringList hiddenTools = Settings->GetStringList("HiddenTools");
      // show tool if it was hidden
      if (hiddenTools.contains(hBoxName)) {
        hiddenTools.removeAll(hBoxName);
        hToolBox->show();
        Settings->SetValue("HiddenTools", hiddenTools);
        // find first active tool, which changes it's state
        if (hFilt->isActive() && FirstActive.size() == 0)
          FirstActive = hBoxName;
      }

      QStringList favouriteTools = Settings->GetStringList("FavouriteTools");
      if (favouriteTools.contains(hBoxName)) {
        favouriteTools.removeAll(hBoxName);
        Settings->SetValue("FavouriteTools", favouriteTools);
      }
    }
  }

  // Image need to be updated, if active tools have changed their state
  if (FirstActive.size() != 0)
    Update(FirstActive);
}


////////////////////////////////////////////////////////////////////////////////
//
// UpdateVisibleTools
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateVisibleTools() {
  QStringList hiddenTools = Settings->GetStringList("HiddenTools");
  QStringList favouriteTools = Settings->GetStringList("FavouriteTools");

  foreach (QString itGroupBox, *m_GroupBoxesOrdered) {
    auto hFilt = m_GroupBox->value(itGroupBox);

    // find all TreeItems with necessary name (in different tabs)
    QList<QStandardItem *> itemList =
        m_VisibleToolsModel->findItems(hFilt->caption(), Qt::MatchExactly | Qt::MatchRecursive);

    // find item corresponding to proper tab (maybe there is a simpler way)
    QStandardItem *item = NULL;
    foreach (QStandardItem *it, itemList) {
      QString tabName = ProcessingTabBook->tabText(hFilt->parentTabIdx());
      if (m_VisibleToolsModel->indexFromItem(it).parent() ==
          m_VisibleToolsModel->indexFromItem(
              m_VisibleToolsModel->findItems(tabName, Qt::MatchExactly).first() ))
      {
        item = it;
        break;
      }
    }

    // set tool state and corresponding icon will be set automatically
    if (hiddenTools.contains(itGroupBox)) {
      m_VisibleToolsModel->setData(m_VisibleToolsModel->indexFromItem(item),
                                   tsHidden, Qt::UserRole+1);
    } else {
      if (favouriteTools.contains(itGroupBox)) {
        m_VisibleToolsModel->setData(m_VisibleToolsModel->indexFromItem(item),
                                     tsFavourite, Qt::UserRole+1);
      } else {
        m_VisibleToolsModel->setData(m_VisibleToolsModel->indexFromItem(item),
                                     tsNormal, Qt::UserRole+1);
      }
    }

    // check if tool can be hidden and set corresponding flag
    if (Settings->ToolAlwaysVisible(itGroupBox))
      m_VisibleToolsModel->setData(m_VisibleToolsModel->indexFromItem(item), 1, Qt::UserRole+2);
    else
      m_VisibleToolsModel->setData(m_VisibleToolsModel->indexFromItem(item), 0, Qt::UserRole+2);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// LoadUISettings
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::LoadUISettings(const QString &fileName) {
  QSettings UISettings(fileName, QSettings::IniFormat);
  UISettings.sync();

  if (UISettings.status() != QSettings::NoError) {
    ptMessageBox::critical(0, "Error", "Error reading UI file\n" + fileName);
    return;
  }
  if (UISettings.value("Magic") != "photivoUIFile") {
    ptMessageBox::warning(0, "Error", fileName + "\nis not an UI file.");
    return;
  }

  Settings->SetValue("FavouriteTools", UISettings.value("FavouriteTools").toStringList());

  QStringList hOldHidden = Settings->GetStringList("HiddenTools");
  QStringList hNewHidden = UISettings.value("HiddenTools").toStringList();
  QStringList Temp = hNewHidden;

  // find difference between previous and current hidden tools list
  hOldHidden.removeDuplicates();
  hNewHidden.removeDuplicates();
  foreach (QString str, hOldHidden) {
    if (hNewHidden.contains(str)) {
      hOldHidden.removeOne(str);
      hNewHidden.removeOne(str);
    }
  }

  // create a list of active tools which change their state
  QStringList hActiveTools;
  for (QString str: hNewHidden) {
    auto hFilt = m_GroupBox->value(str);
    hFilt->guiWidget()->hide();
    if (hFilt->isActive())
      hActiveTools.append(str);
  }

  Settings->SetValue("HiddenTools", Temp);
  for (QString str: hOldHidden) {
    auto hFilt = m_GroupBox->value(str);
    hFilt->guiWidget()->show();
    if (hFilt->isActive())
      hActiveTools.append(str);
  }

  // Image need to be updated, if active tools have changed their state
  foreach (QString str, *m_GroupBoxesOrdered) {
    if (hActiveTools.contains(str)) {
      Update(str);
      break;
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// SaveUISettings
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::SaveUISettings(const QString &fileName) const {
  QSettings UISettings(fileName, QSettings::IniFormat);
  UISettings.clear();

  UISettings.setValue("Magic", "photivoUIFile");
  UISettings.setValue("HiddenTools", Settings->GetStringList("HiddenTools"));
  UISettings.setValue("FavouriteTools", Settings->GetStringList("FavouriteTools"));

  UISettings.sync();
  if (UISettings.status() != QSettings::NoError)
    ptMessageBox::critical(0, "Error", "Error writing UI file\n" + fileName);
}


////////////////////////////////////////////////////////////////////////////////
//
// OnVisibleToolsDiscardButtonClicked
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::OnVisibleToolsDiscardButtonClicked() {
  // update VisibleTools box
  UpdateVisibleTools();
}


////////////////////////////////////////////////////////////////////////////////
//
// OnVisibleToolsLoadButtonClicked
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::OnVisibleToolsLoadButtonClicked() {
  QString UIFilePattern = tr("Photivo UI file (*.ptu);;All files (*.*)");
  QString UIFileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open UI"),
                                                    Settings->GetString("UIDirectory"),
                                                    UIFilePattern);
  if (UIFileName.size() == 0) return;

  LoadUISettings(UIFileName);

  // update VisibleTools box
  UpdateVisibleTools();
}


////////////////////////////////////////////////////////////////////////////////
//
// OnVisibleToolsSaveButtonClicked
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::OnVisibleToolsSaveButtonClicked() {
  // apply changes at VisibleTools box
  ApplyVisibleTools();

  QString UIFilePattern = tr("Photivo UI file (*.ptu);;All files (*.*)");
  QString UIFileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save UI"),
                                                    Settings->GetString("UIDirectory"),
                                                    UIFilePattern);
  if (UIFileName.size() == 0) return;

  QFileInfo PathInfo(UIFileName);
  if (PathInfo.suffix().size() == 0)
    UIFileName += ".ptu";

  SaveUISettings(UIFileName);
}


////////////////////////////////////////////////////////////////////////////////
//
// Update lensfun UI elements
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateLfunDistUI() {
  short DistModel = Settings->GetInt("LfunDistModel");
  LfunDistPoly3Container->setVisible(DistModel == ptLfunDistModel_Poly3);
  LfunDistPoly5Container->setVisible(DistModel == ptLfunDistModel_Poly5);
  LfunDistFov1Container->setVisible(DistModel == ptLfunDistModel_Fov1);
  LfunDistPTLensContainer->setVisible(DistModel == ptLfunDistModel_PTLens);
}

void ptMainWindow::UpdateLfunCAUI() {
  short CAModel = Settings->GetInt("LfunCAModel");
  LfunCALinearContainer->setVisible(CAModel == ptLfunCAModel_Linear);
  LfunCAPoly3Container->setVisible(CAModel == ptLfunCAModel_Poly3);
}

void ptMainWindow::UpdateLfunVignetteUI() {
  short VignetteModel = Settings->GetInt("LfunVignetteModel");
  LfunVignettePoly6Container->setVisible(VignetteModel == ptLfunVignetteModel_Poly6);
}

////////////////////////////////////////////////////////////////////////////////
//
// Update liquid rescale UI elements
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateLiquidRescaleUI() {
  short Scaling = Settings->GetInt("LqrScaling");
  LqrHorScaleWidget->setVisible(Scaling == ptLqr_ScaleRelative);
  LqrVertScaleWidget->setVisible(Scaling == ptLqr_ScaleRelative);
  LqrWHContainter->setVisible(Scaling == ptLqr_ScaleAbsolute);
}


/////////////////////////////////////////////////////////////////////////////
//
// Update gradual blur UI elements
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateGradualBlurUI() {
  bool Visible = Settings->GetInt("GradBlur1") == ptGradualBlur_Linear  ||
                    Settings->GetInt("GradBlur1") == ptGradualBlur_MaskLinear;
  Settings->Show("GradBlur1Angle",      Visible);
  Settings->Show("GradBlur1Vignette",  !Visible);
  Settings->Show("GradBlur1Roundness", !Visible);
  Settings->Show("GradBlur1CenterX",   !Visible);
  Settings->Show("GradBlur1CenterY",   !Visible);

  Visible = Settings->GetInt("GradBlur2") == ptGradualBlur_Linear  ||
            Settings->GetInt("GradBlur2") == ptGradualBlur_MaskLinear;
  Settings->Show("GradBlur2Angle",      Visible);
  Settings->Show("GradBlur2Vignette",  !Visible);
  Settings->Show("GradBlur2Roundness", !Visible);
  Settings->Show("GradBlur2CenterX",   !Visible);
  Settings->Show("GradBlur2CenterY",   !Visible);
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
////////////////////////////////////////////////////////////////////////////////

ptMainWindow::~ptMainWindow() {
  while (ToolBoxStructureList.size()) {
    ToolBoxStructureList.removeAt(0);
  }
}

//==============================================================================

#ifdef Q_OS_WIN
bool ptMainWindow::winEvent(MSG *message, long *result) {
  return ptEcWin7::GetInstance()->winEvent(message, result);
}

#endif
