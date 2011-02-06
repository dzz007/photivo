////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
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

#include "ptMainWindow.h"
#include "ptViewWindow.h"
#include "ptWhiteBalances.h"
#include "ptChannelMixer.h"
#include "ptError.h"
#include "ptGuiOptions.h"
#include "ptLensfun.h"
#include "ptSettings.h"
#include "ptConstants.h"
#include "ptDefines.h"
#include "ptTheme.h"

#include <iostream>
#include <iomanip>

#include <iostream>
#include <QMessageBox>
using namespace std;

extern ptTheme* Theme;
extern ptViewWindow* ViewWindow;

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


////////////////////////////////////////////////////////////////////////////////
//
// ptMainWindow constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptMainWindow::ptMainWindow(const QString Title)
  : QMainWindow(NULL) {

  // Setup from the Gui builder.
  setupUi(this);
  setWindowTitle(Title);

  // Setup splitter
  //~ if (Settings->GetInt("SwitchLayout")) {
    //~ QList SplitterSizes = MainSplitter.sizes();
    //~ SplitterSizes.move(0,1);
  //~
    //~ MainSplitter->insertWidget(1,MainSplitter->widget(0));
    //~ MainSplitter->setSizes(SplitterSizes);
    //~ MainSplitter->setStretchFactor(0,1);
  //~ }

  //~ QDockWidget *ControlsDockWidget = new QDockWidget(tr("Controls"));
  //~ ControlsDockWidget->setObjectName("ControlsDockWidget");
  //~ ControlsDockWidget->setWidget(MainSplitter->widget(0));
  //~ ControlsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  //~ ControlsDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  //~ addDockWidget(Qt::LeftDockWidgetArea, ControlsDockWidget);

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
    //printf("(%s,%d) '%s'\n",__FILE__,__LINE__,Key.toAscii().data());
    switch (Settings->GetGuiType(Key)) {

      case ptGT_InputSlider :
      case ptGT_Input       :
        //printf("(%s,%d) Creating Input for '%s'\n",
        //       __FILE__,__LINE__,Key.toAscii().data());
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
        //       __FILE__,__LINE__,Key.toAscii().data());
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
        //       __FILE__,__LINE__,Key.toAscii().data());
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
        //       __FILE__,__LINE__,Key.toAscii().data());
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
        //       __FILE__,__LINE__,Key.toAscii().data());
        continue;
    };
    // To sync the state of the now created gui element we have
    // to set the value. As the setting now has the gui element
    // attached, it will be updated.
    Settings->SetValue(Key,Settings->GetValue(Key));
    m_ToolBoxes = NULL;
  }

  //
  // Menu structure related.
  //

  // menu removed ;-)

  // Leftover after menu was removed
  // m_ToolBoxes is set in here
  OnToolBoxesEnabledTriggered(Settings->GetInt("ToolBoxMode"));

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
  Macro_ConnectSomeButton(ZoomFull);
  Macro_ConnectSomeButton(FullScreen);
  FullScreenButton->setChecked(0);
  Macro_ConnectSomeButton(LoadStyle);

  //
  // Gimp related
  //

#if defined (DLRAW_GIMP_PLUGIN) || defined (DLRAW_HAVE_GIMP)
  Macro_ConnectSomeButton(ToGimp);
#else
  ToGimpButton->hide();
#endif

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
  // TAB : Lensfun
  //

  // Not coverd by the macros as it takes input from lensfun
  // database rather than gui settings.

  // 'None' on 0. Hence -1.
  Settings->AddOrReplaceOption("LensfunCamera",
                               tr("None"),
                               QVariant(-1));
  for (short i=0; i<LensfunData->m_NrCameras; i++) {
    QString CameraName;
    CameraName += "(";
    CameraName += LensfunData->m_Cameras[i].Make;
    CameraName += ") ";
    CameraName += LensfunData->m_Cameras[i].Model;
    Settings->AddOrReplaceOption("LensfunCamera",
                                 CameraName,
                                 QVariant(i));
  }

  Settings->SetEnabled("LensfunTCAEnable",0);
  Settings->SetEnabled("LensfunVignettingEnable",0);
  Settings->SetEnabled("LensfunDistortionEnable",0);

  //
  // TAB : RGB
  //

  Macro_ConnectSomeButton(RotateLeft);
  Macro_ConnectSomeButton(RotateRight);
  Macro_ConnectSomeButton(RotateAngle);
  Macro_ConnectSomeButton(MakeCrop);

  Macro_ConnectSomeButton(ChannelMixerOpen);
  Macro_ConnectSomeButton(ChannelMixerSave);

  if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Auto) {
    Settings->SetEnabled("WhiteFraction",1);
    Settings->SetEnabled("WhiteLevel",1);
  } else {
    Settings->SetEnabled("WhiteFraction",0);
    Settings->SetEnabled("WhiteLevel",0);
  }

  Macro_ConnectSomeButton(CurveRGBOpen);
  Macro_ConnectSomeButton(CurveRGBSave);
  Macro_ConnectSomeButton(CurveROpen);
  Macro_ConnectSomeButton(CurveRSave);
  Macro_ConnectSomeButton(CurveGOpen);
  Macro_ConnectSomeButton(CurveGSave);
  Macro_ConnectSomeButton(CurveBOpen);
  Macro_ConnectSomeButton(CurveBSave);

  //
  // TAB : Lab
  //

  Macro_ConnectSomeButton(CurveLOpen);
  Macro_ConnectSomeButton(CurveLSave);
  Macro_ConnectSomeButton(CurveaOpen);
  Macro_ConnectSomeButton(CurveaSave);
  Macro_ConnectSomeButton(CurvebOpen);
  Macro_ConnectSomeButton(CurvebSave);
  Macro_ConnectSomeButton(CurveLByHueOpen);
  Macro_ConnectSomeButton(CurveLByHueSave);
  Macro_ConnectSomeButton(CurveHueOpen);
  Macro_ConnectSomeButton(CurveHueSave);
  Macro_ConnectSomeButton(CurveTextureOpen);
  Macro_ConnectSomeButton(CurveTextureSave);
  Macro_ConnectSomeButton(CurveSaturationOpen);
  Macro_ConnectSomeButton(CurveSaturationSave);
  Macro_ConnectSomeButton(CurveShadowsHighlightsOpen);
  Macro_ConnectSomeButton(CurveShadowsHighlightsSave);
  Macro_ConnectSomeButton(CurveDenoiseOpen);
  Macro_ConnectSomeButton(CurveDenoiseSave);

  //
  // TAB : EyeCandy
  //

  Macro_ConnectSomeButton(Tone1Color);
  Macro_ConnectSomeButton(Tone2Color);

  Macro_ConnectSomeButton(TextureOverlay);
  Macro_ConnectSomeButton(TextureOverlayClear);

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

  //~ connect(TagsEditWidget,SIGNAL(textChanged()),
  //~ this,SLOT(OnTagsEditTextChanged()));

  Macro_ConnectSomeButton(BaseCurveOpen);
  Macro_ConnectSomeButton(BaseCurveSave);

  Macro_ConnectSomeButton(BaseCurve2Open);
  Macro_ConnectSomeButton(BaseCurve2Save);

  Macro_ConnectSomeButton(OutputColorProfileReset);
  Macro_ConnectSomeButton(WriteOutput);

  Macro_ConnectSomeButton(WritePipe);

  Settings->SetEnabled("OutputGamma",Settings->GetInt("OutputGammaCompensation"));
  Settings->SetEnabled("OutputLinearity",Settings->GetInt("OutputGammaCompensation"));

  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG)
    Settings->SetEnabled("SaveSampling",1);
  else
    Settings->SetEnabled("SaveSampling",0);

  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG ||
      Settings->GetInt("SaveFormat")==ptSaveFormat_PNG)
    Settings->SetEnabled("SaveQuality",1);
  else
    Settings->SetEnabled("SaveQuality",0);

  // hide some stuff instead of removing from the code
  findChild<ptGroupBox *>(QString("TabToolBoxControl"))->setVisible(0);
  findChild<ptGroupBox *>(QString("TabMemoryTest"))->setVisible(0);
  findChild<ptGroupBox *>(QString("TabRememberSettings"))->setVisible(0);
  findChild<ptGroupBox *>(QString("TabLensfun"))->setVisible(0);
  m_ToolBoxes->removeOne(findChild<ptGroupBox *>(QString("TabLensfun")));
  findChild<ptGroupBox *>(QString("TabOutput"))->setVisible(0);
  m_ToolBoxes->removeOne(findChild<ptGroupBox *>(QString("TabOutput")));

  UpdateToolBoxes();

  // Set us in the beginning of the tabbook and show mainwindow.
  // But we do not want to generate events for this during setup
  MainTabBook->blockSignals(1);
  MainTabBook->setCurrentIndex(0);
  MainTabBook->blockSignals(0);

  MainTabBook->setCurrentWidget(TabProcessing);

  ProcessingTabBook->blockSignals(1);
  ProcessingTabBook->setCurrentIndex(0);
  ProcessingTabBook->blockSignals(0);

  m_ActiveTabs.append(GeometryTab);
  m_ActiveTabs.append(RGBTab);
  m_ActiveTabs.append(LabCCTab);
  m_ActiveTabs.append(LabSNTab);
  m_ActiveTabs.append(LabEyeCandyTab);
  m_ActiveTabs.append(EyeCandyTab);
  m_ActiveTabs.append(OutTab);

  m_StatusIcon = QIcon(QString::fromUtf8(":/photivo/Icons/circleactive.png"));

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
  Temp.replace("_"," ");
  Temp.replace("!","(");
  Temp.replace("@",")");
  AppVersionLabel->setText(Temp);

  Tabbar = ProcessingTabBook->findChild<QTabBar*>();
  Tabbar->installEventFilter(this);
  WritePipeButton->installEventFilter(this);
  ToGimpButton->installEventFilter(this);
  ResetButton->installEventFilter(this);

  // Set help pages
  findChild <ptGroupBox*>("TabCrop")->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/geometry#crop");
  findChild <ptGroupBox*>("TabWhiteBalance")->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/camera#white_balance");
  findChild <ptGroupBox*>("TabBW")->
    SetHelpUri("http://photivo.org/photivo/manual/tabs/eyecandy#black_and_white");

  // context menu for save button
  m_AtnSavePipe = new QAction(tr("Save current pipe"), this);
  connect(m_AtnSavePipe, SIGNAL(triggered()), this, SLOT(SaveMenuPipe()));
  m_AtnSaveFull = new QAction(tr("Save full size"), this);
  connect(m_AtnSaveFull, SIGNAL(triggered()), this, SLOT(SaveMenuFull()));
  m_AtnSaveSettings = new QAction(tr("Save settings file"), this);
  connect(m_AtnSaveSettings, SIGNAL(triggered()), this, SLOT(SaveMenuSettings()));
  m_AtnSaveJobfile = new QAction(tr("Save job file"), this);
  connect(m_AtnSaveJobfile, SIGNAL(triggered()), this, SLOT(SaveMenuJobfile()));

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
}

void CB_Event0();
void ptMainWindow::Event0TimerExpired() {
  ::CB_Event0();
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
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      QPoint position = mouseEvent->pos();
      int c = Tabbar->count();
      int clickedItem = -1;

      for (int i=0; i<c; i++) {
        if ( Tabbar->tabRect(i).contains( position ) ) {
          clickedItem = i;
          break;
        }
      }
      //QMessageBox::information(NULL,"Event","ContextMenu on "+QString::number(clickedItem));
      return true;
    } else if (obj == WritePipeButton) {
      QMenu Menu(NULL);
      Menu.setStyle(Theme->ptStyle);
      Menu.setPalette(Theme->ptMenuPalette);
      Menu.addAction(m_AtnSavePipe);
      Menu.addAction(m_AtnSaveFull);
      Menu.addAction(m_AtnSaveSettings);
      Menu.addAction(m_AtnSaveJobfile);
      Menu.exec(static_cast<QMouseEvent *>(event)->globalPos());
    } else if (obj == ToGimpButton) {
      QMenu Menu(NULL);
      Menu.setStyle(Theme->ptStyle);
      Menu.setPalette(Theme->ptMenuPalette);
      Menu.addAction(m_AtnGimpSavePipe);
      Menu.addAction(m_AtnGimpSaveFull);
      Menu.exec(static_cast<QMouseEvent *>(event)->globalPos());
    } else if (obj == ResetButton) {
      QMenu Menu(NULL);
      Menu.setStyle(Theme->ptStyle);
      Menu.setPalette(Theme->ptMenuPalette);
      Menu.addAction(m_AtnMenuUserReset);
      Menu.addAction(m_AtnMenuFullReset);
      Menu.addSeparator();
      Menu.addAction(m_AtnMenuOpenPreset);
      Menu.addAction(m_AtnMenuOpenSettings);
      Menu.exec(static_cast<QMouseEvent *>(event)->globalPos());
    }
    return QObject::eventFilter(obj, event);
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
// GetCurrentTab
//
// Determine the current tab selected.
//
////////////////////////////////////////////////////////////////////////////////

short ptMainWindow::GetCurrentTab() {
  // I opt for matching onto widget name, rather than on
  // index, as I feel that this is more robust for change.
  //~ if (ProcessingTabBook->currentWidget() == GenericTab) return ptGenericTab;
  if (ProcessingTabBook->currentWidget()== CameraTab) return ptCameraTab;
  else if (ProcessingTabBook->currentWidget()== GeometryTab) return ptGeometryTab;
  else if (ProcessingTabBook->currentWidget()== RGBTab) return ptRGBTab;
  else if (ProcessingTabBook->currentWidget()== LabCCTab) return ptLabCCTab;
  else if (ProcessingTabBook->currentWidget()== LabSNTab) return ptLabSNTab;
  else if (ProcessingTabBook->currentWidget()== LabEyeCandyTab) return ptLabEyeCandyTab;
  else if (ProcessingTabBook->currentWidget()== EyeCandyTab) return ptEyeCandyTab;
  else if (ProcessingTabBook->currentWidget()== OutTab) return ptOutTab;
  else {
     ptLogError(ptError_Argument,"Unforeseen tab.");
     assert(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// All kind of Gui events.
// Translated back to a CB_ function into ptMain
//
////////////////////////////////////////////////////////////////////////////////

//
// Tabbook switching
//

void CB_Tabs(const short Index);
void ptMainWindow::on_ProcessingTabBook_currentChanged(const int Index) {
  ::CB_Tabs(Index);
}

//
// Menu structure
//

// menu removed ;-) mike

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
  // String list for the tool boxes
  if (m_ToolBoxes) delete m_ToolBoxes;
  m_ToolBoxes = new QList<ptGroupBox *>;

  // Procedure
  QList <QToolBox *> ToolBoxes = findChildren <QToolBox *> ();
  for (short i=0; i<ToolBoxes.size(); i++) {
    sToolBoxStructure* ToolBoxStructure = new sToolBoxStructure;
    ToolBoxStructureList.append(ToolBoxStructure);
    ToolBoxStructure->ToolBox = ToolBoxes[i];
    ToolBoxStructure->Parent =
      qobject_cast <QWidget*>(ToolBoxes[i]->parent());
    ToolBoxStructure->ParentLayout =
      qobject_cast <QVBoxLayout*>(ToolBoxStructure->Parent->layout());
    assert (ToolBoxStructure->ParentLayout);
    for (short j=0; j<ToolBoxes[i]->count(); j++) {
      QWidget* Page = ToolBoxes[i]->widget(j);
      QVBoxLayout* PageLayout = qobject_cast <QVBoxLayout*>(Page->layout());
      ToolBoxStructure->Pages.append(Page);
      ToolBoxStructure->PageLayouts.append(PageLayout);
      // Alternative groupboxes. The layout of which still needs to be created.
      ptGroupBox* GroupBox =
        new ptGroupBox(ToolBoxStructure->ToolBox->itemText(j),this,
          ToolBoxStructure->ToolBox->widget(j)->objectName());
      // Names for the groupboxes!
      GroupBox->setObjectName(ToolBoxStructure->ToolBox->widget(j)->objectName());
      if (ToolBoxStructure->ToolBox != SettingsToolBox &&
          ToolBoxStructure->ToolBox != InfoToolBox)
        m_ToolBoxes->append(GroupBox);

      QVBoxLayout* GroupBoxLayout = new QVBoxLayout(GroupBox->m_Widget);
      GroupBoxLayout->setSpacing(0);
      //~ GroupBoxLayout->setContentsMargins(0,0,0,0);
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
      assert(DirectChildrenOfPage.size() == 1);
      ToolBoxStructure->Widgets.append(DirectChildrenOfPage[0]);
    }
  }
}

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
        ToolBoxStructure->Widgets[j]->setObjectName("GroupContainer");
        //~ ToolBoxStructure->GroupBoxLayouts[j]->setContentsMargins(0,0,0,0);
//~ QList <QToolBox *> ToolBoxes = findChildren <QToolBox *> ()
      QList <QWidget *> TempList = ToolBoxStructure->Widgets[j]->findChildren <QWidget *> ();
        for (short k=0; k<TempList.size(); k++) {
          if (TempList[k]->layout()) TempList[k]->setObjectName("GroupBoxTools");
        }
//~ ToolBoxStructure->GroupBoxLayouts[j]->setSpacing(0);
//~ ToolBoxStructure->GroupBoxLayouts[j]->setMargin(0);
//~ ToolBoxStructure->GroupBoxLayouts[j]->addStretch();
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
    for (short Idx=0; Idx<ToolBoxStructureList.size(); Idx++) {
      sToolBoxStructure* ToolBoxStructure = ToolBoxStructureList[Idx];
      for (short j=0; j<ToolBoxStructure->Widgets.size(); j++) {
        ToolBoxStructure->Widgets[j]->setParent(ToolBoxStructure->Pages[j]);
        ToolBoxStructure->PageLayouts[j]->
          addWidget(ToolBoxStructure->Widgets[j]);
        ToolBoxStructure->PageLayouts[j]->addStretch();
      }
      ToolBoxStructure->ToolBox->setParent(ToolBoxStructure->Parent);
      ToolBoxStructure->ParentLayout->addWidget(ToolBoxStructure->ToolBox);
      ToolBoxStructure->Parent->show();
    }
  }
}

//
// Gimp
//

void ptMainWindow::OnToGimpButtonClicked() {
#ifdef DLRAW_GIMP_PLUGIN
  ::CB_MenuFileExit(1);
#endif
#ifdef DLRAW_HAVE_GIMP
  GimpSaveMenuPipe();
#endif
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

void CB_InputChanged(const QString ObjectName, const QVariant Value);
void ptMainWindow::OnInputChanged(const QVariant Value) {
  QObject* Sender = sender();
  printf("(%s,%d) Sender : '%s'\n",
         __FILE__,__LINE__,Sender->objectName().toAscii().data());
  CB_InputChanged(Sender->objectName(),Value);

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
  MainTabBook->setCurrentWidget(TabProcessing);
}

void ptMainWindow::OnTabSettingsButtonClicked() {
  if (MainTabBook->currentWidget() == TabSetting)
    MainTabBook->setCurrentWidget(TabProcessing);
  else
    MainTabBook->setCurrentWidget(TabSetting);
}

void ptMainWindow::OnTabInfoButtonClicked() {
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

void CB_OpenFileButton();
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

void CB_CurveRGBOpenButton();
void ptMainWindow::OnCurveRGBOpenButtonClicked() {
  ::CB_CurveRGBOpenButton();
}

void CB_CurveRGBSaveButton();
void ptMainWindow::OnCurveRGBSaveButtonClicked() {
  ::CB_CurveRGBSaveButton();
}

void CB_CurveROpenButton();
void ptMainWindow::OnCurveROpenButtonClicked() {
  ::CB_CurveROpenButton();
}

void CB_CurveRSaveButton();
void ptMainWindow::OnCurveRSaveButtonClicked() {
  ::CB_CurveRSaveButton();
}

void CB_CurveGOpenButton();
void ptMainWindow::OnCurveGOpenButtonClicked() {
  ::CB_CurveGOpenButton();
}

void CB_CurveGSaveButton();
void ptMainWindow::OnCurveGSaveButtonClicked() {
  ::CB_CurveGSaveButton();
}

void CB_CurveBOpenButton();
void ptMainWindow::OnCurveBOpenButtonClicked() {
  ::CB_CurveBOpenButton();
}

void CB_CurveBSaveButton();
void ptMainWindow::OnCurveBSaveButtonClicked() {
  ::CB_CurveBSaveButton();
}

void CB_CurveLOpenButton();
void ptMainWindow::OnCurveLOpenButtonClicked() {
  ::CB_CurveLOpenButton();
}

void CB_CurveLSaveButton();
void ptMainWindow::OnCurveLSaveButtonClicked() {
  ::CB_CurveLSaveButton();
}

void CB_CurveaOpenButton();
void ptMainWindow::OnCurveaOpenButtonClicked() {
  ::CB_CurveaOpenButton();
}

void CB_CurveaSaveButton();
void ptMainWindow::OnCurveaSaveButtonClicked() {
  ::CB_CurveaSaveButton();
}

void CB_CurvebOpenButton();
void ptMainWindow::OnCurvebOpenButtonClicked() {
  ::CB_CurvebOpenButton();
}

void CB_CurvebSaveButton();
void ptMainWindow::OnCurvebSaveButtonClicked() {
  ::CB_CurvebSaveButton();
}

void CB_CurveLByHueOpenButton();
void ptMainWindow::OnCurveLByHueOpenButtonClicked() {
  ::CB_CurveLByHueOpenButton();
}

void CB_CurveLByHueSaveButton();
void ptMainWindow::OnCurveLByHueSaveButtonClicked() {
  ::CB_CurveLByHueSaveButton();
}

void CB_CurveHueOpenButton();
void ptMainWindow::OnCurveHueOpenButtonClicked() {
  ::CB_CurveHueOpenButton();
}

void CB_CurveHueSaveButton();
void ptMainWindow::OnCurveHueSaveButtonClicked() {
  ::CB_CurveHueSaveButton();
}

void CB_CurveTextureOpenButton();
void ptMainWindow::OnCurveTextureOpenButtonClicked() {
  ::CB_CurveTextureOpenButton();
}

void CB_CurveTextureSaveButton();
void ptMainWindow::OnCurveTextureSaveButtonClicked() {
  ::CB_CurveTextureSaveButton();
}

void CB_CurveShadowsHighlightsOpenButton();
void ptMainWindow::OnCurveShadowsHighlightsOpenButtonClicked() {
  ::CB_CurveShadowsHighlightsOpenButton();
}

void CB_CurveShadowsHighlightsSaveButton();
void ptMainWindow::OnCurveShadowsHighlightsSaveButtonClicked() {
  ::CB_CurveShadowsHighlightsSaveButton();
}

void CB_CurveDenoiseOpenButton();
void ptMainWindow::OnCurveDenoiseOpenButtonClicked() {
  ::CB_CurveDenoiseOpenButton();
}

void CB_CurveDenoiseSaveButton();
void ptMainWindow::OnCurveDenoiseSaveButtonClicked() {
  ::CB_CurveDenoiseSaveButton();
}

void CB_CurveSaturationOpenButton();
void ptMainWindow::OnCurveSaturationOpenButtonClicked() {
  ::CB_CurveSaturationOpenButton();
}

void CB_CurveSaturationSaveButton();
void ptMainWindow::OnCurveSaturationSaveButtonClicked() {
  ::CB_CurveSaturationSaveButton();
}

void CB_BaseCurveOpenButton();
void ptMainWindow::OnBaseCurveOpenButtonClicked() {
  ::CB_BaseCurveOpenButton();
}

void CB_BaseCurveSaveButton();
void ptMainWindow::OnBaseCurveSaveButtonClicked() {
  ::CB_BaseCurveSaveButton();
}

void CB_BaseCurve2OpenButton();
void ptMainWindow::OnBaseCurve2OpenButtonClicked() {
  ::CB_BaseCurve2OpenButton();
}

void CB_BaseCurve2SaveButton();
void ptMainWindow::OnBaseCurve2SaveButtonClicked() {
  ::CB_BaseCurve2SaveButton();
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

void CB_WriteOutputButton();
void ptMainWindow::OnWriteOutputButtonClicked() {
  ::CB_WriteOutputButton();
}

void CB_WritePipeButton();
void ptMainWindow::OnWritePipeButtonClicked() {
  ::CB_WritePipeButton();
}

//~ void CB_TagsEditTextEdit(const QString Text);
//~ void ptMainWindow::OnTagsEditTextChanged() {
  //~ ::CB_TagsEditTextEdit(TagsEditWidget->toPlainText());
//~ }


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

// Catch keyboard shortcuts
void CB_FullScreenButton(const int State);
void UpdateSettings();
void CB_RunModeCheck(const QVariant Check);
void CB_OpenPresetFileButton();
void CB_MenuFileOpen(const short HaveFile);
void CB_InputChanged(const QString,const QVariant);
void CB_ZoomFitButton();
void CB_WritePipeButton();
void CB_SpecialPreviewChoice(const QVariant Choice);
void CB_MenuFileExit(const short);
void ViewWindowStatusReport(short State);
void ptMainWindow::keyPressEvent(QKeyEvent *Event) {
  if (ViewWindow->m_SelectionAllowed &&
      Event->key()==Qt::Key_Alt) {
    ViewWindow->LightsOut();
    return;
  }
  if (Event->key()==Qt::Key_Escape) { // back to used view
    if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_RGB)
        CB_SpecialPreviewChoice(ptSpecialPreview_RGB);
    else {
      if (Settings->GetInt("ShowToolContainer")==0) {
        Settings->SetValue("ShowToolContainer",1);
        UpdateSettings();
      } else
        ::CB_FullScreenButton(0);
      }
  } else if (Event->key()==Qt::Key_F11) { // toggle full screen
    if (isFullScreen()==true)
      ::CB_FullScreenButton(0);
    else
      ::CB_FullScreenButton(1);
  } else if (Event->key()==Qt::Key_1 && Event->modifiers()==Qt::NoModifier) {
    CB_InputChanged("ZoomInput",50);
  } else if (Event->key()==Qt::Key_2 && Event->modifiers()==Qt::NoModifier) {
    CB_InputChanged("ZoomInput",100);
  } else if (Event->key()==Qt::Key_3 && Event->modifiers()==Qt::NoModifier) {
    CB_InputChanged("ZoomInput",200);
  } else if (Event->key()==Qt::Key_4 && Event->modifiers()==Qt::NoModifier) {
    CB_ZoomFitButton();
  } else if (Event->key()==Qt::Key_F1) {
    QDesktopServices::openUrl(QString("http://photivo.org/photivo/manual"));
  }
  // most shortcuts should only work when we are not in special state like cropping
  if (Settings->GetInt("BlockTools")==0) {
    if (Event->key()==Qt::Key_P && Event->modifiers()==Qt::NoModifier) {
        MainTabBook->setCurrentWidget(TabProcessing);
    } else if (Event->key()==Qt::Key_S && Event->modifiers()==Qt::NoModifier) {
        MainTabBook->setCurrentWidget(TabSetting);
    } else if (Event->key()==Qt::Key_I && Event->modifiers()==Qt::NoModifier) {
        MainTabBook->setCurrentWidget(TabInfo);
    } else if (Event->key()==Qt::Key_M && Event->modifiers()==Qt::NoModifier) {
      ::CB_RunModeCheck(1-Settings->GetInt("RunMode"));
    } else if (Event->key()==Qt::Key_Space) {
      Settings->SetValue("ShowToolContainer",1-Settings->GetInt("ShowToolContainer"));
      UpdateSettings();
    } else if (Event->key()==Qt::Key_F5) {
      OnRunButtonClicked();
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
    } else if (Event->key()==Qt::Key_Period && Event->modifiers()==Qt::NoModifier) {
      ProcessingTabBook->setCurrentIndex(MIN(ProcessingTabBook->currentIndex()+1,ProcessingTabBook->count()));
    } else if (Event->key()==Qt::Key_Comma && Event->modifiers()==Qt::NoModifier) {
      ProcessingTabBook->setCurrentIndex(MAX(ProcessingTabBook->currentIndex()-1,0));
    } else if (Event->key()==Qt::Key_T && Event->modifiers()==Qt::NoModifier) {
      CB_PreviewModeButton(1-Settings->GetInt("PreviewTabMode"));
    } else if (Event->key()==Qt::Key_0 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_RGB)
        CB_SpecialPreviewChoice(ptSpecialPreview_RGB);
      else ViewWindowStatusReport(0);
    } else if (Event->key()==Qt::Key_9 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_Structure)
        CB_SpecialPreviewChoice(ptSpecialPreview_Structure);
      else ViewWindowStatusReport(0);
    } else if (Event->key()==Qt::Key_8 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_L)
        CB_SpecialPreviewChoice(ptSpecialPreview_L);
      else ViewWindowStatusReport(0);
    } else if (Event->key()==Qt::Key_7 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_A)
        CB_SpecialPreviewChoice(ptSpecialPreview_A);
      else ViewWindowStatusReport(0);
    } else if (Event->key()==Qt::Key_6 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_B)
        CB_SpecialPreviewChoice(ptSpecialPreview_B);
      else ViewWindowStatusReport(0);
    } else if (Event->key()==Qt::Key_5 && Event->modifiers()==Qt::NoModifier) {
      if (Settings->GetInt("SpecialPreview")!=ptSpecialPreview_Gradient)
        CB_SpecialPreviewChoice(ptSpecialPreview_Gradient);
      else ViewWindowStatusReport(0);
    } else if (Event->key()==Qt::Key_C && Event->modifiers()==Qt::NoModifier) {
      Settings->SetValue("ExposureIndicator",1-Settings->GetInt("ExposureIndicator"));
      Update(ptProcessorPhase_NULL);
    // hidden tools, needs GUI implementation
    } else if (Event->key()==Qt::Key_H && Event->modifiers()==Qt::NoModifier) {
      QString Tools = "";
      QString Tab = "";
      for (int i=0; i<m_ToolBoxes->size();i++) {
        if (Settings->ToolIsHidden(m_ToolBoxes->at(i)->objectName())) {
          Tab = m_ToolBoxes->at(i)->parentWidget()->parentWidget()->parentWidget()->parentWidget()->objectName();
          Tools = Tools + Tab + ": " + Settings->ToolGetName(m_ToolBoxes->at(i)->objectName()) + "\n";
        }
      }
      if (Tools == "") Tools = "No tools hidden!";
      QMessageBox::information(this,"Hidden tools",Tools);
      /*findChild<QWidget *>(QString("TabGenCorrections"))->
        setVisible(1-findChild<QWidget *>(QString("TabGenCorrections"))->isVisible()); */
    } else if (Event->key()==Qt::Key_U && Event->modifiers()==Qt::NoModifier) {
      // show hidden tools on current tab
      int Active = 0;
      QString Tool= "";
      QStringList TempList = Settings->GetStringList("HiddenTools");
      TempList.removeDuplicates();
      for (int i=0; i<m_ToolBoxes->size();i++) {
        if (TempList.contains(m_ToolBoxes->at(i)->objectName())) {
          QString Tab = m_ToolBoxes->at(i)->parentWidget()->parentWidget()->parentWidget()->parentWidget()->objectName();
          if (ProcessingTabBook->currentWidget()->objectName()==Tab) {
            m_ToolBoxes->at(i)->show();
            TempList.removeOne(m_ToolBoxes->at(i)->objectName());
            Settings->SetValue("HiddenTools", TempList);
            if (Settings->ToolIsActive(m_ToolBoxes->at(i)->objectName())) {
              Active = 1;
              Tool = m_ToolBoxes->at(i)->objectName();
            }
          }
        }
      }
      Settings->SetValue("HiddenTools", TempList);
      // run processor if needed
      if (Active) Update(Tool);
    /*} else if (Event->key()==Qt::Key_L && Event->modifiers()==Qt::NoModifier) {
      QString Tools = "";
      for (int i=0; i<m_ToolBoxes->size();i++) {
        Tools = Tools + m_ToolBoxes->at(i)->objectName() + " ";
        if (i%4==0) Tools = Tools + "\n";
      }
      QMessageBox::warning(this,"Tools",Tools);*/ // plain list all tools
    } else if (Event->key()==Qt::Key_A && Event->modifiers()==Qt::NoModifier) {
      QString Tools = "";
      QString Tab = "";
      for (int i=0; i<m_ToolBoxes->size();i++) {
        if (Settings->ToolIsActive(m_ToolBoxes->at(i)->objectName())) {
          Tab = m_ToolBoxes->at(i)->parentWidget()->parentWidget()->parentWidget()->parentWidget()->objectName();
          Tools = Tools + Tab + ": " + Settings->ToolGetName(m_ToolBoxes->at(i)->objectName()) + "\n";
        }
      }
      if (Tools == "") Tools = "No tools active!";
      QMessageBox::information(this,"Active tools",Tools);
    } else if (Event->key()==Qt::Key_B && Event->modifiers()==Qt::NoModifier) {
      QString Tools = "";
      QString Tab = "";
      for (int i=0; i<m_ToolBoxes->size();i++) {
        if (Settings->ToolIsBlocked(m_ToolBoxes->at(i)->objectName())) {
          Tab = m_ToolBoxes->at(i)->parentWidget()->parentWidget()->parentWidget()->parentWidget()->objectName();
          Tools = Tools + Tab + ": " + Settings->ToolGetName(m_ToolBoxes->at(i)->objectName()) + "\n";
        }
      }
      if (Tools == "") Tools = "No tools blocked!";
      QMessageBox::information(this,"Blocked tools",Tools);
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
// UpdateToolBoxes
//
////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateToolBoxes() {
  for (int i=0; i<m_ToolBoxes->size();i++) {
    if (Settings->ToolIsHidden(m_ToolBoxes->at(i)->objectName())) {
      m_ToolBoxes->at(i)->hide();
    } else {
      m_ToolBoxes->at(i)->show();
    }
    m_ToolBoxes->at(i)->Update();
  }

  // disable Raw tools when we have a bitmap
  QList<ptGroupBox *> m_RawTools;
  m_RawTools << findChild <ptGroupBox*>("TabCameraColorSpace")
             << findChild <ptGroupBox*>("TabGenCorrections")
             << findChild <ptGroupBox*>("TabWhiteBalance")
             << findChild <ptGroupBox*>("TabDemosaicing")
             << findChild <ptGroupBox*>("TabHighlightRecovery");
  short Temp = Settings->GetInt("IsRAW");
  for (int i = 0; i < m_RawTools.size(); i++) {
    m_RawTools.at(i)->SetEnabled(Temp);
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
  for (int i=0; i<m_ToolBoxes->size();i++)
    m_ToolBoxes->at(i)->SetActive(Settings->ToolIsActive(m_ToolBoxes->at(i)->objectName()));

  // Status LED on tabs
  if(Settings->GetInt("TabStatusIndicator")) {
    ProcessingTabBook->setIconSize(QSize(Settings->GetInt("TabStatusIndicator"),Settings->GetInt("TabStatusIndicator")));
    QList <ptGroupBox *> Temp;
    for (int j=0; j<m_ActiveTabs.size();j++) {
      Temp = m_ActiveTabs.at(j)->findChildren <ptGroupBox *> ();
      int k=0;
      for (int i=0; i<Temp.size();i++)
        if(Settings->ToolIsActive(Temp.at(i)->objectName())) k=1;

      if (k>0)
        ProcessingTabBook->setTabIcon(ProcessingTabBook->indexOf(m_ActiveTabs.at(j)),m_StatusIcon);
      else
        ProcessingTabBook->setTabIcon(ProcessingTabBook->indexOf(m_ActiveTabs.at(j)),QIcon());
    }
  } else {
    for (int j=0; j<m_ActiveTabs.size();j++)
      ProcessingTabBook->setTabIcon(ProcessingTabBook->indexOf(m_ActiveTabs.at(j)),QIcon());
  }

// Added later next to the other fields
  //~ // Zoom factor
  //~ QString ZoomText;
  //~ QString Tmp;
  //~ int TmpZoom = Settings->GetInt("Zoom");
  //~ int TmpScaled = Settings->GetInt("Scaled");
  //~ ZoomText += Tmp.setNum((uint16_t)
    //~ ((Settings->GetInt("ImageW") >> TmpScaled)*TmpZoom/100));
  //~ ZoomText += " X ";
  //~ ZoomText += Tmp.setNum((uint16_t)
    //~ ((Settings->GetInt("ImageH") >> TmpScaled)*TmpZoom/100));
  //~ ZoomText += " (";
  //~ ZoomText += Tmp.setNum(TmpZoom >> TmpScaled);
  //~ ZoomText += "%)";
  //~ // ZoomLabel->setText(ZoomText); Moved down to account for startup situation

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

  // lensfun : get make and model out of the exif and
  // compare with lensfun database cameras.
  // Select the choice accordingly.

  int PreviousLensfunCameraIndex = Settings->GetInt("LensfunCameraIndex");

  if (Settings->GetInt("LensfunCameraUpdatedByProcessor")) {

    Settings->SetValue("LensfunCameraUpdatedByProcessor",0);

    Settings->SetValue("LensfunCamera",0); // XXX JDLA Check me !
    Settings->SetValue("LensfunCameraIndex",-1);

    short HaveCamera = Settings->GetString("LensfunCameraMake").size()
                     && Settings->GetString("LensfunCameraModel").size();
    for (short i=0;
         HaveCamera && i<Settings->GetNrOptions("LensfunCamera");
         i++) {
      QVariant ItemData = Settings->GetOptionsValue("LensfunCamera",i);
      if (ItemData.toInt() == -1) continue;
      /*
      printf("(%s,%d) Checking '%s' against '%s' and '%s' against '%s'\n",
             __FILE__,__LINE__,
             Settings->GetString("LensfunCameraMake").toAscii().data(),
             LensfunData->m_Cameras[ItemData.toInt()].Make.toAscii().data(),
             Settings->GetString("LensfunCameraModel").toAscii().data(),
             LensfunData->m_Cameras[ItemData.toInt()].Model.toAscii().data());
      */
      if ( (Settings->GetString("LensfunCameraMake") ==
            LensfunData->m_Cameras[ItemData.toInt()].Make)
          &&
           (Settings->GetString("LensfunCameraModel") ==
            LensfunData->m_Cameras[ItemData.toInt()].Model) ) {
        // got it !
        //printf("(%s,%d) Got It\n",__FILE__,__LINE__);
        Settings->SetValue("LensfunCamera",ItemData); // XXX JDLA CHECK ME !
        Settings->SetValue("LensfunCameraIndex",ItemData.toInt());
        break;
      }
    }
  }

  if (Settings->GetInt("LensfunCameraIndex") != PreviousLensfunCameraIndex) {
    // Presents sensible lenses, i.e. the mount corresponds.
    Settings->ClearOptions("LensfunLens");
    Settings->AddOrReplaceOption("LensfunLens",
                                 tr("None"),
                                 QVariant(-1)); // None on 0.
    for (short i=0; i<LensfunData->m_NrLenses; i++) {
      QString LensMount   = LensfunData->m_Lenses[i].Mount;
      QString CameraMount;
      if (Settings->GetInt("LensfunCameraIndex") == -1) {
        CameraMount = "GENERIC";
      } else {
        CameraMount =
          LensfunData->m_Cameras[Settings->GetInt("LensfunCameraIndex")].Mount;
      }
      // Some flexible match.
      if ( LensMount == "GENERIC" ||
           LensMount == CameraMount ||
           (LensMount == "CANON EF" && CameraMount == "CANON EF-S") ) {
        // We have a lens with a matching mount.
        QString LensName;
        LensName += "(";
        LensName += LensfunData->m_Lenses[i].Make;
        LensName += ") ";
        LensName += LensfunData->m_Lenses[i].Model;
        Settings->AddOrReplaceOption("LensfunLens",LensName,QVariant(i));
      }
    }
  }

  Settings->SetEnabled("LensfunTCAEnable",
                       Settings->GetInt("LensfunHaveTCAModel"));
  LensfunTCAModelLabel->setText(Settings->GetString("LensfunTCAModel"));

  Settings->SetEnabled("LensfunVignettingEnable",
                       Settings->GetInt("LensfunHaveVignettingModel"));
  LensfunVignettingModelLabel->
    setText(Settings->GetString("LensfunVignettingModel"));

  Settings->SetEnabled("LensfunDistortionEnable",
                       Settings->GetInt("LensfunHaveDistortionModel"));
  LensfunDistortionModelLabel->
    setText(Settings->GetString("LensfunDistortionModel"));

  //~ // Resize
  //~ Settings->SetMaximum("ResizeW",Settings->GetInt("CropW"));
  //~ Settings->SetMaximum("ResizeH",Settings->GetInt("CropH"));

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
  ShortFileName = PathInfo.baseName();
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

  // sRGB gamma compensation
  Settings->SetEnabled("OutputGamma",Settings->GetInt("OutputGammaCompensation"));
  Settings->SetEnabled("OutputLinearity",Settings->GetInt("OutputGammaCompensation"));

  // Save options
  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG)
    Settings->SetEnabled("SaveSampling",1);
  else
    Settings->SetEnabled("SaveSampling",0);

  if (Settings->GetInt("SaveFormat")==ptSaveFormat_JPEG ||
      Settings->GetInt("SaveFormat")==ptSaveFormat_PNG)
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
    //~ SizeLabel->setText(Report);
    SizeFullLabel->setText(Report);

    int TmpScaled = Settings->GetInt("Scaled");
    //~ Report = "";
    //~ Report += Tmp.setNum(Settings->GetInt("ImageW") >> TmpScaled);
    //~ Report += " X ";
    //~ Report += Tmp.setNum(Settings->GetInt("ImageH") >> TmpScaled);
    //~ Report += " (";
    //~ Report += Tmp.setNum(100>>TmpScaled);
    //~ Report += "%)";
    //~ PipeSizeLabel->setText(Report);

    Report = "";
    Report += Tmp.setNum(Settings->GetInt("PipeImageW") << TmpScaled);
    Report += " x ";
    Report += Tmp.setNum(Settings->GetInt("PipeImageH") << TmpScaled);
    //~ Report += " (";
    //~ Report += Tmp.setNum(100*(Settings->GetInt("PipeImageW") << TmpScaled)/
    //~ Settings->GetInt("ImageW"));
    //~ Report += "%)";
    //~ ViewSizeLabel->setText(Report);
    Report += "     ";
    SizeFullCropLabel->setText(Report);

    Report = "";
    Report += Tmp.setNum(Settings->GetInt("PipeImageW"));
    Report += " x ";
    Report += Tmp.setNum(Settings->GetInt("PipeImageH"));
    //~ Report += " (";
    //~ Report += Tmp.setNum(100*Settings->GetInt("PipeImageW")/
    //~ Settings->GetInt("ImageW"));
    //~ Report += "%)";
    //~ PipeViewSizeLabel->setText(Report);
    Report += "     ";
    SizeCurrentLabel->setText(Report);

    //~ int TmpZoom = Settings->GetInt("Zoom");

    //~ Report = "";
    //~ Report += Tmp.setNum(Settings->GetInt("PipeImageW") * TmpZoom/100);
    //~ Report += " X ";
    //~ Report += Tmp.setNum(Settings->GetInt("PipeImageH") * TmpZoom/100);
    //~ Report += " (";
    //~ Report += Tmp.setNum(Settings->GetInt("PipeImageW") * TmpZoom /
                         //~ Settings->GetInt("ImageW"));
    //~ Report += "%)";
    //~ ScreenSizeLabel->setText(Report);

  } else {
    // Startup.

  }
}


////////////////////////////////////////////////////////////////////////////////

void ptMainWindow::UpdateFilenameInfo(const QStringList FileNameList) {
  QFileInfo fn(FileNameList[0]);
  if (FileNameList.length() > 0 ) {
    FileNameLabel->setText(fn.fileName());
    #ifdef Q_OS_WIN32
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

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  } else {
    Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue"));
    if (Pos != ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
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
  }
  InfoFocalLengthLabel->setText(TempString);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.Flash"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  InfoFlashLabel->setText(TheInfo);
  TheInfo="";

  Pos = ExifData.findKey(Exiv2::ExifKey("Exif.Photo.WhiteBalance"));
  if (Pos != ExifData.end() ) {
    std::stringstream str;
    str << *Pos;
    TheInfo.append(QString(str.str().c_str()));
  }
  InfoWhitebalanceLabel->setText(TheInfo);
  TheInfo="";

  // dcraw Data
  TheInfo = Settings->GetString("CameraMake") + ": " + Settings->GetString("CameraModel");
  InfoDcrawLabel->setText(TheInfo);
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
////////////////////////////////////////////////////////////////////////////////

ptMainWindow::~ptMainWindow() {
  //printf("(%s,%d) %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
  for (short i=0; i<ToolBoxStructureList.size(); i++) {
    //delete ToolBoxStructureList[i];
  }
  while (ToolBoxStructureList.size()) {
    ToolBoxStructureList.removeAt(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
