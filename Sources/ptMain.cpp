/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2011 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2010-2011 Bernd Schoeler <brjohn@brother-john.net>
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

#define QT_CLEAN_NAMESPACE

#include <QtGui>
#include <QtCore>
//#include <QtNetwork>
#include <string>
#include <csignal>

#include "ptMessageBox.h"
#include "ptDcRaw.h"
#include "ptProcessor.h"
#include "ptMainWindow.h"
#include "ptViewWindow.h"
#include "ptCurveWindow.h"
#include "ptHistogramWindow.h"
#include "ptGuiOptions.h"
#include "ptSettings.h"
//#include "ptLensfun.h"    // TODO BJ: implement lensfun DB
#include "ptError.h"
#include "ptRGBTemperature.h"
#include "ptWhiteBalances.h"
#include "ptChannelMixer.h"
#include "ptCurve.h"
#include "ptFastBilateral.h"
#include "ptTheme.h"
#include "ptWiener.h"
#include "qtsingleapplication/qtsingleapplication.h"
#ifdef Q_OS_MAC
    #include <QFileOpenEvent>
#endif

#include <Magick++.h>

#ifdef Q_OS_WIN32
#include "qt_windows.h"
#include "qlibrary.h"
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif
#endif

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// This is the file where everything is started.
// It starts not only the gui but it is *also* the starting point
// for guiless jobmode. The design is such that the pipe runs
// independent whether or not gui thingies are attached to it.
//
////////////////////////////////////////////////////////////////////////////////

DcRaw*       TheDcRaw        = NULL;
ptProcessor* TheProcessor    = NULL;

ptCurve*  RGBGammaCurve     = NULL;
ptCurve*  RGBContrastCurve  = NULL;
ptCurve*  ExposureCurve     = NULL;
ptCurve*  ContrastCurve     = NULL;
// RGB,R,G,B,L,a,b,Base
ptCurve*  Curve[17]         = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
ptCurve*  BackupCurve[17]   = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
// I don't manage to init statically following ones. Done in InitCurves.
QStringList CurveKeys, CurveToolNameKeys, CurveBackupKeys;
QStringList CurveFileNamesKeys;

ptChannelMixer* ChannelMixer = NULL;

cmsHPROFILE PreviewColorProfile = NULL;
cmsCIExyY       D65;
cmsCIExyY       D50;
// precalculated color transform
cmsHTRANSFORM ToPreviewTransform = NULL;


//
// The 'tee' towards the display.
// Visualization :
//
// Raw->Image_AfterDcRaw->Image_AfterLensfun->Image_AfterRGB->Image_AfterLab->
// Image_AfterGREYC->Image_AfterEyeCandy
// -------------------------------------------------------------------------
//                                       |
//                                Somewhere a tee to the preview
//                                       |
//                                  PreviewImage
//
// The pipe changed, Lab and Greyc became LabCC and LabSN


ptImage*  PreviewImage     = NULL;
ptImage*  HistogramImage   = NULL;

// The main windows of the application.
ptMainWindow*      MainWindow      = NULL;
ptViewWindow*      ViewWindow      = NULL;
ptHistogramWindow* HistogramWindow = NULL;
ptCurveWindow*     CurveWindow[17] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

// Error dialog for segfaults
ptMessageBox* SegfaultErrorBox;

// Theming
ptTheme* Theme = NULL;

QTranslator appTranslator;

// Gui options and settings.
ptGuiOptions  *GuiOptions = NULL;
ptSettings    *Settings = NULL;

// Lensfun database.
//ptLensfun*  LensfunData = NULL;    // TODO BJ: implement lensfun DB

// Screen position
QPoint MainWindowPos;
QSize  MainWindowSize;

// Run mode
short NextPhase;
short NextSubPhase;
short ImageSaved;
short ImageCleanUp;

// uint16_t (0,0xffff) to float (0.0, 1.0)
float    ToFloatTable[0x10000];
uint16_t ToSRGBTable[0x10000];

// Filter patterns for the filechooser.
QString ChannelMixerFilePattern;
QString CurveFilePattern;
QString JobFilePattern;
QString SettingsFilePattern;
QString ProfilePattern;
QString RawPattern;
QString BitmapPattern;

void InitStrings() {
  ChannelMixerFilePattern =
    QCoreApplication::translate("Global Strings","Photivo channelmixer file (*.ptm);;All files (*.*)");
  CurveFilePattern =
    QCoreApplication::translate("Global Strings","Photivo curve file (*.ptc);;All files (*.*)");
  JobFilePattern =
    QCoreApplication::translate("Global Strings","Photivo job file (*.ptj);;All files (*.*)");
  SettingsFilePattern =
    QCoreApplication::translate("Global Strings","Photivo settings file (*.pts);;All files (*.*)");
  ProfilePattern =
    QCoreApplication::translate("Global Strings","ICC colour profiles (*.icc *.icm);;All files (*.*)");

  // QFileDialog has no case insensitive option ...
  RawPattern =
    QCoreApplication::translate("Global Strings","Raw files ("
                                                 "*.arw *.ARW *.Arw "
                                                 "*.bay *.BAY *.Bay "
                                                 "*.bmq *.BMQ *.Bmq "
                                                 "*.cr2 *.CR2 *.Cr2 "
                                                 "*.crw *.CRW *.Crw "
                                                 "*.cs1 *.CS1 *.Cs1 "
                                                 "*.dc2 *.DC2 *.Dc2 "
                                                 "*.dcr *.DCR *.Dcr "
                                                 "*.dng *.DNG *.Dng "
                                                 "*.erf *.ERF *.Erf "
                                                 "*.fff *.FFF *.Fff "
                                                 "*.hdr *.HDR *.Hdr "
                                                 "*.ia  *.IA *.Ia "
                                                 "*.k25 *.K25 "
                                                 "*.kc2 *.KC2 *.Kc2 "
                                                 "*.kdc *.KDC *.Kdc "
                                                 "*.mdc *.MDC *.Mdc "
                                                 "*.mef *.MEF *.Mef "
                                                 "*.mos *.MOS *.Mos "
                                                 "*.mrw *.MRW *.Mrw "
                                                 "*.nef *.NEF *.Nef "
                                                 "*.orf *.ORF *.Orf "
                                                 "*.pef *.PEF *.Pef "
                                                 "*.pxn *.PXN *.Pxn "
                                                 "*.qtk *.QTK *.Qtk "
                                                 "*.raf *.RAF *.Raf "
                                                 "*.raw *.RAW *.Raw "
                                                 "*.rdc *.RDC *.Rdc "
                                                 "*.rw2 *.RW2 *.Rw2 "
                                                 "*.sr2 *.SR2 *.Sr2 "
                                                 "*.srf *.SRF *.Srf "
                                                 "*.sti *.STI *.Sti "
                                                 "*.tif *.TIF *.Tif "
                                                 "*.x3f *.X3F *.X3f)"
                                                 ";;Bitmaps ("
                                                 "*.jpeg *.JPEG *.Jpeg "
                                                 "*.jpg *.JPG *.Jpg "
                                                 "*.tiff *.TIFF *.Tiff "
                                                 "*.tif *.TIF *.Tif "
                                                 "*.bmp *.BMP *.Bmp "
                                                 "*.ppm *.PPm *.Ppm "
                                                 ";;All files (*.*)");

  BitmapPattern =
    QCoreApplication::translate("Global Strings","Bitmaps ("
                                                 "*.jpeg *.JPEG *.Jpeg "
                                                 "*.jpg *.JPG *.Jpg "
                                                 "*.tiff *.TIFF *.Tiff "
                                                 "*.tif *.TIF *.Tif "
                                                 "*.bmp *.BMP *.Bmp "
                                                 "*.ppm *.PPm *.Ppm "
                                                 ";;All files (*.*)");
}

////////////////////////////////////////////////////////////////////////////////
//
// Some function prototypes.
//
////////////////////////////////////////////////////////////////////////////////

void   RunJob(const QString FileName);
short  ReadJobFile(const QString FileName);
short  ReadSettingsFile(const QString FileName, short& NextPhase);
void   WriteOut();
void   UpdatePreviewImage(const ptImage* ForcedImage   = NULL,
                          const short    OnlyHistogram = 0);
void   UpdateCropToolUI();
void   InitCurves();
void   InitChannelMixers();
void   PreCalcTransforms();
void   CB_ChannelMixerChoice(const QVariant Choice);
void   CB_CurveChoice(const int Channel, const int Choice);
void   CB_CurveWindowRecalc(const short Channel, const short ForceUpdate = 0);
void   CB_ZoomFitButton();
void   CB_MenuFileOpen(const short HaveFile);
void   CB_MenuFileExit(const short);
void   ExtFileOpen(const QString file);
void   CB_WritePipeButton();
void   CB_OpenPresetFileButton();
void   CB_OpenSettingsFileButton();
void   CB_CropOrientationButton();
short  WriteSettingsFile(const QString FileName, const short IsJobFile = 0);
void   SetBackgroundColor(int SetIt);
void   CB_StyleChoice(const QVariant Choice);
void   CB_SliderWidthInput(const QVariant Value);
void Export(const short mode);
void Update(short Phase,
            short SubPhase      = -1,
            short WithIdentify  = 1,
            short ProcessorMode = ptProcessorMode_Preview);
int CalculatePipeSize(const bool NewImage = false);
void CB_OpenSettingsFile(QString SettingsFileName);
void SaveButtonToolTip(const short mode);

int    photivoMain(int Argc, char *Argv[]);
void   CleanupResources();
void copyFolder(QString sourceFolder, QString destFolder);
bool GBusy = false;

////////////////////////////////////////////////////////////////////////////////
//
// Progress function in the GUI.
// Can later also in job.
//
////////////////////////////////////////////////////////////////////////////////

void ReportProgress(const QString Message) {
  printf("Progress : %s\n",Message.toAscii().data());
  if (!MainWindow) return;
  MainWindow->StatusLabel->setText(Message);
  MainWindow->StatusLabel->repaint();
  // Workaround to keep the GUI responsive
  // during pipe processing...
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

////////////////////////////////////////////////////////////////////////////////
//
// Main : instantiating toplevel windows, settings and options ..
//
////////////////////////////////////////////////////////////////////////////////

short   InStartup  = 1;

short   JobMode = 0;
QString JobFileName;
QString ImageFileToOpen = "";
#ifdef Q_OS_MAC
    bool MacGotFileEvent=false;
#endif
QRect DetailViewRect;

#ifndef DLRAW_GIMP_PLUGIN
void SegfaultAbort(int) {
  // prevent infinite recursion if SegfaultAbort() causes another segfault
  std::signal(SIGSEGV, SIG_DFL);
  std::signal(SIGABRT, SIG_DFL);
  SegfaultErrorBox->exec();
  std::abort();
}

int main(int Argc, char *Argv[]) {
  int RV = photivoMain(Argc,Argv);
  DelAndNull(SegfaultErrorBox);
  CleanupResources(); // Not necessary , for debug.
  return RV;
}
#endif


//class QtSingleApplication with redefined event to handle QFileOpenEvent
#ifdef Q_OS_MAC
  class MyQApplication : public QtSingleApplication {
  protected:
    virtual bool event(QEvent *event);
  public:
      bool initialized;
      void macinit();
      MyQApplication(const QString &appId, int & argc, char ** arg);
  };

  bool MyQApplication::event(QEvent *event)
  {
      switch (event->type()) {
      case QEvent::FileOpen:
          ImageFileToOpen=static_cast<QFileOpenEvent *>(event)->file();
          MacGotFileEvent=true;
          if(initialized){
              //skip on first Event
          ExtFileOpen(ImageFileToOpen);
          }
          return true;
      default:
          MacGotFileEvent=false;
          break;
      }
      return QtSingleApplication::event(event);
  }
  MyQApplication::MyQApplication(const QString &appId,int & argc, char ** argv) : QtSingleApplication(appId,argc, argv){
  initialized=false;
  };
  void MyQApplication::macinit(){
      initialized=true;
  };
#endif


#ifdef Q_OS_MAC
MyQApplication* TheApplication;
#else
QtSingleApplication* TheApplication;
#endif

int photivoMain(int Argc, char *Argv[]) {
  QString VerTemp(TOSTRING(APPVERSION));    //also used for the cli syntax error msg below!
  printf("Photivo version %s\n", VerTemp.toAscii().data());

  Magick::InitializeMagick(*Argv);

  // TextCodec
  QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());

  //QApplication TheApplication(Argc,Argv);
  QStringList environment = QProcess::systemEnvironment();
  QString user=environment.filter(QRegExp("^USERNAME=|^USER=",Qt::CaseInsensitive)).first();
  if(!user.isEmpty()){
      int l=user.indexOf("=",0);
      user="_"+user.right(user.length()-l-1);
  }
  user="photivo"+user;

#ifdef Q_OS_MAC
  TheApplication = new MyQApplication(user, Argc,Argv);
#else
  TheApplication = new QtSingleApplication(user, Argc,Argv);
#endif
  //close on last window closed, seems to be unneeded
  TheApplication->connect(TheApplication, SIGNAL(lastWindowClosed()), TheApplication, SLOT(quit()));

  // Error handling for segfaults (to prevent silent crashes)
  SegfaultErrorBox = new ptMessageBox(QMessageBox::Critical,
                                      QObject::tr("Photivo crashed"),
                                      QObject::tr("Photivo crashed. You can get help on our flickr forum at\n"
                                      "<a href=\"http://www.flickr.com/groups/photivo/discuss/\">http://www.flickr.com/groups/photivo/discuss/</a>"
                                      "\nWhen you post there make sure to describe your last actions before the crash occurred."),
                                      QMessageBox::Ok);
  SegfaultErrorBox->setTextFormat(Qt::RichText);
  std::signal(SIGSEGV, SegfaultAbort);
  std::signal(SIGABRT, SegfaultAbort);


  // Handle cli arguments
  ImageCleanUp = 0;
  short AllowNewInstance = 0;
  short CliArgumentsError = 0;

  QString PhotivoCliUsageMsg = QObject::tr(
          "Syntax: photivo [-i imagefile | -j jobfile | -g imagefile] [h] [--new-instance]\n"
          "Options:\n"
          "-i imagefile      Specify image file to load. The -i is optional. Starting\n"
          "                  Photivo with just a file name works the same.\n"
          "-j jobfile        Specify jobfile for batch processing. Job files are created\n"
          "                  in Photivo and then executed with this option.\n"
          "-g imagefile      Specify temporary file used for Gimp-to-Photivo export.\n"
          "                  Internal option, not intended for general use.\n"
          "                  BEWARE! This option deletes imagefile!\n"
          "--new-instance    Allow opening another Photivo instance instead of using a\n"
          "                  currently running Photivo. Job files are always opened in a\n"
          "                  new instance.\n"
          "-h                Display this usage information.\n\n"
          "For more documentation visit the wiki:\n"
          "http://photivo.org/photivo/start\n"
          );
#ifdef Q_OS_MAC
//Just Skip if engaged by QFileOpenEvent
  if(!MacGotFileEvent) {
#endif
    // Put Argv[] into a string list for non-PITA handling
    QStringList CliArgs;
    for (int i = 1; i < Argc; i++) {
      CliArgs << Argv[i];
    }

    if (CliArgs.indexOf("-h") > -1) {
#ifdef Q_OS_WIN32
      // no ptMessageBox because we're on Windows and Theme object not yet created
      ptMessageBox::information(0, QObject::tr("Photivo command line options"), PhotivoCliUsageMsg);
#else
      fprintf(stdout,"%s",PhotivoCliUsageMsg.toAscii().data());
#endif
      exit(0);
    }

    if (CliArgs.indexOf("--new-instance") > -1) {
      AllowNewInstance = 1;
      CliArgs.removeAll("--new-instance");
    }

    // all the file open switches
    if (CliArgs.count() > 0) {
      if (CliArgs[0] == "-i") {
        CliArgs.removeAt(0);
        if (CliArgs.count() != 1) {
          CliArgumentsError = 1;
        } else {
          ImageFileToOpen = CliArgs[0];
        }

      } else if (CliArgs[0] == "-j") {
        CliArgs.removeAt(0);
        if (CliArgs.count() != 1) {
          CliArgumentsError = 1;
        } else {
          AllowNewInstance = 1;
          JobMode = 1;
          JobFileName = CliArgs[0];
        }

      } else if (CliArgs[0] == "-g") {
        CliArgs.removeAt(0);
        if (CliArgs.count() != 1) {
          CliArgumentsError = 1;
        } else {
          ImageCleanUp = 1;
          ImageFileToOpen = CliArgs[0];
        }

      // only possibility left is file name without -i
      } else {
        if (CliArgs.count() != 1) {
          CliArgumentsError = 1;
        } else {
          ImageFileToOpen = CliArgs[0];
        }
      }
    }

    if (CliArgumentsError == 1) {
#ifdef Q_OS_WIN32
      // no ptMessageBox because we're on Windows and Theme object not yet created
      ptMessageBox::critical(0, QObject::tr("Unrecognized command line options"), PhotivoCliUsageMsg);
#else
      fprintf(stderr,"%s",PhotivoCliUsageMsg.toAscii().data());
#endif
      exit(EXIT_FAILURE);
    }

#ifdef Q_OS_MAC
  }
#endif


  //QtSingleInstance, add CLI-Switch to skip and allow multiple instances
  if(TheApplication->isRunning() && (AllowNewInstance == 0)) {
    if (ImageFileToOpen != "") {
      TheApplication->sendMessage(ImageFileToOpen);
    } else if (JobFileName != "") {
      TheApplication->sendMessage(JobFileName);
    }
    return 0;
  }


#ifdef Q_OS_MAC
  QDir dir(QApplication::applicationDirPath());
  QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif


  // Some QStringLists to be initialized, has to be the same order as the constants.
  CurveKeys << "CurveRGB"
            << "CurveR"
            << "CurveG"
            << "CurveB"
            << "CurveL"
            << "CurveLa"
            << "CurveLb"
            << "CurveSaturation"
            << "BaseCurve"
            << "BaseCurve2"
            << "CurveLByHue"
            << "CurveTexture"
            << "CurveShadowsHighlights"
            << "CurveDenoise"
            << "CurveHue"
            << "CurveDenoise2"
            << "CurveOutline";

  CurveToolNameKeys << "TabRGBCurve"
                    << "TabRToneCurve"
                    << "TabGToneCurve"
                    << "TabBToneCurve"
                    << "TabLCurve"
                    << "TabABCurves"
                    << "TabABCurves"
                    << "TabSaturationCurve"
                    << "TabBaseCurve"
                    << "TabAfterGammaCurve"
                    << "TabLbyHue"
                    << "TabLABTextureCurve"
                    << "TabLABShadowsHighlights"
                    << "TabDetailCurve"
                    << "TabHueCurve"
                    << "TabDenoiseCurve"
                    << "TabOutline";

  CurveFileNamesKeys << "CurveFileNamesRGB"
                     << "CurveFileNamesR"
                     << "CurveFileNamesG"
                     << "CurveFileNamesB"
                     << "CurveFileNamesL"
                     << "CurveFileNamesLa"
                     << "CurveFileNamesLb"
                     << "CurveFileNamesSaturation"
                     << "CurveFileNamesBase"
                     << "CurveFileNamesBase2"
                     << "CurveFileNamesLByHue"
                     << "CurveFileNamesTexture"
                     << "CurveFileNamesShadowsHighlights"
                     << "CurveFileNamesDenoise"
                     << "CurveFileNamesHue"
                     << "CurveFileNamesDenoise2"
                     << "CurveFileNamesOutline";

  CurveBackupKeys = CurveKeys;

  // User home folder, where Photivo stores its ini and all Presets, Curves etc
  // %appdata%\Photivo on Windows, ~/.photivo on Linux or the program folder for the
  // portable Windows version.
  short IsPortableProfile = 0;
  QString AppDataFolder = "";
  QString Folder = "";
#ifdef Q_OS_WIN32
  IsPortableProfile = QFile::exists("use-portable-profile");
  if (IsPortableProfile != 0) {
      printf("Photivo running in portable mode.\n");
      AppDataFolder = QCoreApplication::applicationDirPath();
      Folder = "";
  } else {
      // Get %appdata% via WinAPI call
      QLibrary library(QLatin1String("shell32"));
      QT_WA(
              {
              typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPTSTR, int, BOOL);
              GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathW");
              if (SHGetSpecialFolderPath) {
              TCHAR path[MAX_PATH];
              SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE);
              AppDataFolder = QString::fromUtf16((ushort*)path);
              }
              },
              {
              typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, char*, int, BOOL);
              GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathA");
              if (SHGetSpecialFolderPath) {
              char path[MAX_PATH];
              SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE);
              AppDataFolder = QString::fromLocal8Bit(path);
              }
              }
           );

      // WinAPI returns path with native separators "\". We need to change this to "/" for Qt.
      AppDataFolder.replace(QString("\\"), QString("/"));
      // Keeping the leading "/" separate here is important or mkdir will fail.
      Folder = "Photivo/";
  }
#else
  Folder = ".photivo/";
  AppDataFolder = QDir::homePath();
#endif

  QString UserDirectory = AppDataFolder + "/" + Folder;

  if (IsPortableProfile == 0) {
      QDir home(AppDataFolder);
      if (!home.exists(Folder))
          home.mkdir(Folder);
  }

  QString SettingsFileName = UserDirectory + "photivo.ini";
  // this has to be changed when we move to a different tree structure!
#ifdef __unix__
  QString NewShareDirectory(TOSTRING(PREFIX));
  if (NewShareDirectory.endsWith("/")) NewShareDirectory.chop(1);
  NewShareDirectory.append("/share/photivo/");
#else
  QString NewShareDirectory = QCoreApplication::applicationDirPath().append("/");
#endif

  QFileInfo SettingsFileInfo(SettingsFileName);
  short NeedInitialization = 1;
  short FirstStart = 1;
  if (SettingsFileInfo.exists() &&
          SettingsFileInfo.isFile() &&
          SettingsFileInfo.isReadable()) {
      // photivo was initialized
      NeedInitialization = 0;
      FirstStart = 0;
      printf("Existing settingsfile '%s'\n",SettingsFileName.toAscii().data());
  } else {
      printf("New settingsfile '%s'\n",SettingsFileName.toAscii().data());
  }

  printf("User directory: '%s'; \n",UserDirectory.toAscii().data());
  printf("Share directory: '%s'; \n",NewShareDirectory.toAscii().data());

  // We need to load the translation before the ptSettings
  QSettings* TempSettings = new QSettings(SettingsFileName, QSettings::IniFormat);

  if (TempSettings->value("SettingsVersion",0).toInt() < PhotivoSettingsVersion)
      NeedInitialization = 1;

  // Initialize the user folder if needed
  /* TODO: for testing. Enable the other line below once profile versions are final. */
  if (IsPortableProfile == 0) {
      //if (NeedInitialization == 1 && IsPortableProfile == 0) {
      printf("Initializing/Updating user profile...\n");
      QFile::remove(UserDirectory + "photivo.png");
      QFile::copy(NewShareDirectory + "photivo.png",
              UserDirectory + "photivo.png");
      QFile::remove(UserDirectory + "photivoLogo.png");
      QFile::copy(NewShareDirectory + "photivoLogo.png",
              UserDirectory + "photivoLogo.png");
      QFile::remove(UserDirectory + "photivoPreview.jpg");
      QFile::copy(NewShareDirectory + "photivoPreview.jpg",
              UserDirectory + "photivoPreview.jpg");
      QStringList SourceFolders;
      SourceFolders << NewShareDirectory + "Translations"
          << NewShareDirectory + "Curves"
          << NewShareDirectory + "ChannelMixers"
          << NewShareDirectory + "Presets"
          << NewShareDirectory + "Profiles"
          << NewShareDirectory + "LensfunDatabase"
          << NewShareDirectory + "UISettings";
      QStringList DestFolders;
      DestFolders << UserDirectory + "Translations"
          << UserDirectory + "Curves"
          << UserDirectory + "ChannelMixers"
          << UserDirectory + "Presets"
          << UserDirectory + "Profiles"
          << UserDirectory + "LensfunDatabase"
          << UserDirectory + "UISettings";

      for (int i = 0; i < SourceFolders.size(); i++) {
          copyFolder(SourceFolders.at(i), DestFolders.at(i));
      }
  }


  // Load Translation
  int TranslMode = TempSettings->value("TranslationMode",0).toInt();
  QDir TranslDir(UserDirectory + "Translations");
  QStringList UiLanguages = TranslDir.entryList(QStringList("photivo_*.qm"), QDir::Files|QDir::Readable, QDir::Name).replaceInStrings(".qm", "", Qt::CaseInsensitive);
  UiLanguages.replaceInStrings("photivo_", "", Qt::CaseInsensitive);
  int LangIdx = -1;

  if (TranslMode == 1) {
      LangIdx = UiLanguages.indexOf(TempSettings->value("UiLanguage","").toString());
      if (LangIdx >= 0) {
          QTranslator qtTranslator;
          appTranslator.load("photivo_" + UiLanguages[LangIdx], UserDirectory + "Translations");
          TheApplication->installTranslator(&appTranslator);
          qtTranslator.load("qt_" + UiLanguages[LangIdx], UserDirectory + "Translations");
          TheApplication->installTranslator(&qtTranslator);
          printf("Enabled translation: \"%s\".\n", UiLanguages[LangIdx].toAscii().data());
      }
  }

  delete TempSettings;

  // Persistent settings.
  // fixed the remember level to 2, since we have settings files now
  short RememberSettingLevel = 2;

  // Load the Settings (are also partly used in JobMode)
  Settings = new ptSettings(RememberSettingLevel, UserDirectory);

  // Set directories for needed files
  Settings->SetValue("UserDirectory", UserDirectory);
  Settings->SetValue("ShareDirectory",NewShareDirectory);
  Settings->SetValue("MainDirectory",QCoreApplication::applicationDirPath().append("/"));

  // Set paths once with first start
  if (FirstStart == 1) {
      Settings->SetValue("RawsDirectory", UserDirectory);
      Settings->SetValue("OutputDirectory", UserDirectory);
      Settings->SetValue("UIDirectory", UserDirectory + "UISettings");
      Settings->SetValue("PresetDirectory", UserDirectory + "Presets");
      Settings->SetValue("CurvesDirectory", UserDirectory + "Curves");
      Settings->SetValue("ChannelMixersDirectory", UserDirectory + "ChannelMixers");
      Settings->SetValue("TranslationsDirectory", UserDirectory + "Translations");
      Settings->SetValue("CameraColorProfilesDirectory", UserDirectory + "Profiles/Camera");
      Settings->SetValue("PreviewColorProfilesDirectory", UserDirectory + "Profiles/Preview");
      Settings->SetValue("OutputColorProfilesDirectory", UserDirectory + "Profiles/Output");
      Settings->SetValue("StandardAdobeProfilesDirectory", UserDirectory + "Profiles/Camera/Standard");
      Settings->SetValue("LensfunDatabaseDirectory", UserDirectory + "LensfunDatabase");
      Settings->SetValue("PreviewColorProfile", UserDirectory + "Profiles/Preview/sRGB.icc");
      Settings->SetValue("OutputColorProfile", UserDirectory + "Profiles/Output/sRGB.icc");
      Settings->SetValue("StartupSettingsFile", UserDirectory + "Presets/MakeFancy.pts");
  }

  // Initialize patterns (after translation)
  InitStrings();

  // Load also the LensfunDatabase.
  printf("Lensfun database: '%s'; \n",Settings->GetString("LensfunDatabaseDirectory").toAscii().data());
  //  LensfunData = new ptLensfun;    // TODO BJ: implement lensfun DB

  // Instantiate the processor.
  TheProcessor = new ptProcessor(ReportProgress);

  // ChannelMixer instance.
  ChannelMixer = new (ptChannelMixer); // Automatically a identity mixer

  // First check if we are maybe started as a command line with options.
  // (And thus have to run a batch job)

  if (JobMode) {
      for (short Channel=0; Channel < CurveKeys.size(); Channel++) {
          Curve[Channel] = new ptCurve(Channel); // Automatically a null curve.
      }
      RunJob(JobFileName);
      exit(EXIT_SUCCESS);
  }

  // If falling through to here we are in an interactive non-job mode.
  // Start op the Gui stuff.

  // Start the theme class
  Theme = new ptTheme(TheApplication);
  Theme->SetCustomCSS(Settings->GetString("CustomCSSFile"));

  GuiOptions = new ptGuiOptions();

  // Open and keep open the profile for previewing.
  PreviewColorProfile = cmsOpenProfileFromFile(
          Settings->GetString("PreviewColorProfile").toAscii().data(),
          "r");
  if (!PreviewColorProfile) {
      ptLogError(ptError_FileOpen,
              Settings->GetString("PreviewColorProfile").toAscii().data());
      return ptError_FileOpen;
  }

  MainWindow =
      new ptMainWindow(QObject::tr("Photivo"));
  QObject::connect(TheApplication, SIGNAL(messageReceived(const QString &)),
          MainWindow,   SLOT(OtherInstanceMessage(const QString &)));


  ViewWindow =
      new ptViewWindow(MainWindow->ViewFrameCentralWidget, MainWindow);

  HistogramWindow =
      new ptHistogramWindow(NULL,MainWindow->HistogramFrameCentralWidget);

  // Populate Translations combobox
  MainWindow->PopulateTranslationsCombobox(UiLanguages, LangIdx);

  // Theming
  CB_StyleChoice(Settings->GetInt("Style"));

  SetBackgroundColor(Settings->GetInt("BackgroundColor"));

  // Different curvewindows.
  QStringList Temp = Settings->GetStringList("BlockedTools");
  Settings->SetValue("BlockedTools",QStringList());
  MainWindow->UpdateToolBoxes();

  QWidget* ParentWidget[] = {MainWindow->RGBCurveCentralWidget,
      MainWindow->RCurveCentralWidget,
      MainWindow->GCurveCentralWidget,
      MainWindow->BCurveCentralWidget,
      MainWindow->LCurveCentralWidget,
      MainWindow->aCurveCentralWidget,
      MainWindow->bCurveCentralWidget,
      MainWindow->SaturationCurveCentralWidget,
      MainWindow->BaseCurveCentralWidget,
      MainWindow->BaseCurve2CentralWidget,
      MainWindow->LByHueCurveCentralWidget,
      MainWindow->TextureCurveCentralWidget,
      MainWindow->ShadowsHighlightsCurveCentralWidget,
      MainWindow->DenoiseCurveCentralWidget,
      MainWindow->HueCurveCentralWidget,
      MainWindow->Denoise2CurveCentralWidget,
      MainWindow->OutlineCurveCentralWidget};

  for (short Channel=0; Channel < CurveKeys.size(); Channel++) {
      Curve[Channel] = new ptCurve(Channel); // Automatically a null curve.
      CurveWindow[Channel] =
          new ptCurveWindow(Curve[Channel],Channel,ParentWidget[Channel]);
  }
  Settings->SetValue("BlockedTools",Temp);
  MainWindow->UpdateToolBoxes();

  // Calculate a nice position.
  // Persistent settings.

  QRect DesktopRect = (QApplication::desktop())->screenGeometry(MainWindow);

  if (RememberSettingLevel == 0) {
      MainWindowPos  = QPoint(DesktopRect.width()/20,DesktopRect.height()/20);
      MainWindowSize = QSize(DesktopRect.width()*9/10,DesktopRect.height()*9/10);
  } else {
      MainWindowPos = Settings->m_IniSettings->
          value("MainWindowPos",
                  QPoint(DesktopRect.width()/20,
                      DesktopRect.height()/20)
               ).toPoint();
      MainWindowSize = Settings->m_IniSettings->
          value("MainWindowSize",
                  QSize(DesktopRect.width()*9/10,
                      DesktopRect.height()*9/10)
               ).toSize();
  }

  if (RememberSettingLevel) {
      MainWindow->MainSplitter->
          restoreState(Settings->m_IniSettings->
                  value("MainSplitter").toByteArray());
      MainWindow->ControlSplitter->
          restoreState(Settings->m_IniSettings->
                  value("ControlSplitter").toByteArray());
  } else {
      // Initial value of splitter.
      QList <int> SizesList;
      SizesList.append(250);
      SizesList.append(1000); // Value obtained to avoid resizing at startup.
      MainWindow->MainSplitter->setSizes(SizesList);
  }

  MainWindow->resize(MainWindowSize);
  MainWindow->move(MainWindowPos);

  if (Settings->m_IniSettings->value("IsMaximized",0).toBool()) {
      MainWindow->showMaximized();
  } else {
      MainWindow->show();
  }

  // Update the preview image will result in displaying the splash.
  Update(ptProcessorPhase_NULL);

  // Open and keep open the profile for previewing.
  PreviewColorProfile = cmsOpenProfileFromFile(
          Settings->GetString("PreviewColorProfile").toAscii().data(),
          "r");
  if (!PreviewColorProfile) {
      ptLogError(ptError_FileOpen,
              Settings->GetString("PreviewColorProfile").toAscii().data());
      assert(PreviewColorProfile);
  }

  // Start event loops.
  return TheApplication->exec();
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  // This is only needed for the gimp integration.
  // Calling over and over photivoMain would otherwise leak like hell
  // (or any comparable place).
  //
  ////////////////////////////////////////////////////////////////////////////////

  void CleanupResources() {
      //printf("(%s,%d) qApp:%p\n",__FILE__,__LINE__,qApp);
      // Not : is done at CB for the exit.
      // delete Settings; // Don't, is done at CB_MenuFileExit
      // Also : do not delete items which are handled by MainWindow, such as
      // ViewWindow or HistogramWindow or CurveWindows
      //  delete LensfunData;    // TODO BJ: implement lensfun DB
      delete TheProcessor;
      delete ChannelMixer;
      delete GuiOptions;
      delete MainWindow;  // Cleans up HistogramWindow and ViewWindow also !
      ViewWindow = NULL;  // needs to be NULL to properly construct MainWindow
      for (short Channel=0; Channel < CurveKeys.size(); Channel++) {
          delete Curve[Channel];
      }
      delete PreviewImage;
      delete TheDcRaw;
}
void   ExtFileOpen(const QString file){
    ImageFileToOpen = file;
    CB_MenuFileOpen(1);
}


////////////////////////////////////////////////////////////////////////////////
//
// Hack : called at t=0 after the event loop started.
//
////////////////////////////////////////////////////////////////////////////////

void CB_Event0() {
  // Init Curves : supposed to be in event loop indeed.
  // (f.i. for progress reporting)
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  if (Settings->GetInt("JobMode") == 0)
    SaveButtonToolTip(Settings->GetInt("SaveButtonMode"));

  // uint16_t (0,0xffff) to float (0.0, 1.0)
#pragma omp parallel for
  for (uint32_t i=0; i<0x10000; i++) {
    ToFloatTable[i] = (float)i/(float)0xffff;
  }

  // linear RGB to sRGB table
#pragma omp parallel for
  for (uint32_t i=0; i<0x10000; i++) {
    if ((double)i/0xffff <= 0.0031308)
      ToSRGBTable[i] = CLIP((int32_t)(12.92*i));
    else
      ToSRGBTable[i] = CLIP((int32_t)((1.055*pow((double)i/0xffff,1.0/2.4)-0.055)*0xffff));
  }

  // Init run mode
  NextPhase = ptProcessorPhase_Raw;
  NextSubPhase = ptProcessorPhase_Load;
  ImageSaved = 0;

  InitCurves();
  InitChannelMixers();
  PreCalcTransforms();

  if (Settings->GetInt("JobMode") == 0) { // not job mode!
    // Load user settings
    if (Settings->GetInt("StartupSettings")) {
      if (ImageCleanUp == 0) {
        CB_OpenSettingsFile(Settings->GetString("StartupSettingsFile"));
      } else { // we got an image from gimp -> neutral display
        CB_OpenSettingsFile(Settings->GetString("PresetDirectory") + "/Neutral_absolute.pts");
      }
      // clean up
      QStringList Temp;
      Temp << "CropX" << "CropY" << "CropW" << "CropH";
      Temp << "RotateW" << "RotateH";
      for (int i = 0; i < Temp.size(); i++) Settings->SetValue(Temp.at(i),0);
    }

    if (ImageFileToOpen.size()) {
      CB_MenuFileOpen(1);
    }
    MainWindow->UpdateSettings();
    ViewWindow->setFocus();
    HistogramWindow->Init();
  }

  if (Settings->GetStringList("FavouriteTools") == QStringList()) {
    QStringList Temp;
    Temp << "TabWhiteBalance"
         << "TabRotation"
         << "TabCrop"
         << "TabReinhard05"
         << "TabLABTexture2"
         << "TabDetailCurve"
         << "TabLABVignette"
         << "TabOutContrast";
    Settings->SetValue("FavouriteTools", Temp);
  }

  InStartup = 0;
//prepare for further QFileOpenEvent(s)
#ifdef Q_OS_MAC
  TheApplication->macinit();
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// InitCurves
// Bring the curves comboboxes in sync and read the correct curve
// just after initialization.
// (This part needed to be added after the persist settings in QSetting
// where the m_CurveFileNames are inited, but not the associated comboboxes
// nor the reading of the curve)
// In fact we mimick here the one after one reading of the curves
// which also ensures no unreadable curves are there, for instance because
// they were meanwhile removed.
//
////////////////////////////////////////////////////////////////////////////////

void InitCurves() {

  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  for (short Channel=0; Channel<CurveKeys.size(); Channel++) {

    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // All curvefilenames with this channel.
    QStringList CurveFileNames =
      Settings->GetStringList(CurveFileNamesKeys[Channel]);
    // Curve set for this particular channel.
    short SettingsCurve = Settings->GetInt(CurveKeys[Channel]);
    ReportProgress(QObject::tr("Loading curves (") + CurveKeys[Channel] + ")");

    // Start adding for this channel.
    for (short Idx = 0; Idx<CurveFileNames.count(); Idx++) {
      if (!Curve[Channel]) Curve[Channel] = new(ptCurve);
      if (Curve[Channel]->ReadCurve(CurveFileNames[Idx].toAscii().data())) {
        QString ErrorMessage = QObject::tr("Cannot read curve ")
                           + " '"
                           + CurveFileNames[Idx]
                           + "'" ;
        if (Settings->GetInt("JobMode") == 0) {
          ptMessageBox::warning(MainWindow,
                           QObject::tr("Curve read error"),
                           ErrorMessage);
        }

        // Remove this invalid and continue.
        // Some househoding due to removal.
        if (SettingsCurve > ptCurveChoice_File+Idx) {
          SettingsCurve--;
        } else if (SettingsCurve == ptCurveChoice_File+Idx) {
          SettingsCurve=0;
        }
        CurveFileNames.removeAt(Idx);
        Idx--;
        continue;
      }

      // Small routine that lets Shortfilename point to the basename.
      QFileInfo PathInfo(CurveFileNames[Idx]);
      QString ShortFileName = PathInfo.baseName().left(18);
      Settings->AddOrReplaceOption(CurveKeys[Channel],
                                   ShortFileName,
                                   ptCurveChoice_File+Idx);
    }

    // We have to write back some stuff to the settins.
    Settings->SetValue(CurveKeys[Channel],SettingsCurve);
    Settings->SetValue(CurveFileNamesKeys[Channel],CurveFileNames);
    // And process now as if just chosen
    // TODO JDLA : Wouldn't this be implied by the setCurrentIndex signal ?
    // Probably not : we're not yet in eventloop.
    CB_CurveChoice(Channel,SettingsCurve);
  }
  ReportProgress(QObject::tr("Ready"));
}

////////////////////////////////////////////////////////////////////////////////
//
// InitChannelMixers
// Bring the mixer combobox in sync and read the correct mixers
// just after initialization.
// In fact we mimick here the one after one reading of the mixers
// which also ensures no unreadable mixers are there, for instance because
// they were meanwhile removed.
//
////////////////////////////////////////////////////////////////////////////////

void InitChannelMixers() {

  ReportProgress(QObject::tr("Loading channelmixers"));
  QStringList ChannelMixerFileNames =
    Settings->GetStringList("ChannelMixerFileNames");
  // Start adding
  for (short Idx = 0;
       Idx<ChannelMixerFileNames.count();
       Idx++) {

    if (ChannelMixer->ReadChannelMixer(
                  ChannelMixerFileNames[Idx].toAscii().data())) {
      QString ErrorMessage = QObject::tr("Cannot read channelmixer ")
                           + " '"
                           + ChannelMixerFileNames[Idx]
                           + "'" ;
      if (Settings->GetInt("JobMode") == 0) {
        ptMessageBox::warning(MainWindow,
                           QObject::tr("Channelmixer read error"),
                           ErrorMessage);
      }

      // Remove this invalid and continue.
      // Some househoding due to removal.
      if (Settings->GetInt("ChannelMixer") > ptChannelMixerChoice_File+Idx) {
        Settings->SetValue("ChannelMixer",Settings->GetInt("ChannelMixer")-1);
      } else if (Settings->GetInt("ChannelMixer") ==
                 ptChannelMixerChoice_File+Idx) {
        Settings->SetValue("ChannelMixer",0);
      }
      ChannelMixerFileNames.removeAt(Idx);
      Idx--;
      continue;
    }

    // Small routine that lets Shortfilename point to the basename.
    QFileInfo PathInfo(ChannelMixerFileNames[Idx]);
    QString ShortFileName = PathInfo.baseName().left(18);
    Settings->AddOrReplaceOption("ChannelMixer",
                                 ShortFileName,
                                 ptChannelMixerChoice_File+Idx);
  }

  // Store the maybe changed list again.
  Settings->SetValue("ChannelMixerFileNames",ChannelMixerFileNames);

  // And process now as if just chosen
  // TODO JDLA : Wouldn't this be implied by the setCurrentIndex signal ?
  // Probably not : we're not yet in eventloop.
  CB_ChannelMixerChoice(QVariant(Settings->GetInt("ChannelMixer")));

  ReportProgress(QObject::tr("Ready"));
}

////////////////////////////////////////////////////////////////////////////////
//
// Precalculation of color transforms
//
////////////////////////////////////////////////////////////////////////////////

void PreCalcTransforms() {
  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  cmsCIExyY DFromReference;

  switch (Settings->GetInt("WorkColor")) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DFromReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DFromReference = D50;
      break;
    default:
      assert(0);
  }

  InProfile = cmsCreateRGBProfile(&DFromReference,
                                  (cmsCIExyYTRIPLE*)&RGBPrimaries[Settings->GetInt("WorkColor")],
                                  Gamma3);

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return;
  }

  cmsFreeToneCurve(Gamma);

  if (Settings->GetInt("CMQuality") == ptCMQuality_HighResPreCalc) {
    ToPreviewTransform =
      cmsCreateTransform(InProfile,
                         TYPE_RGB_16,
                         PreviewColorProfile,
                         TYPE_RGB_16,
                         Settings->GetInt("PreviewColorProfileIntent"),
                         cmsFLAGS_HIGHRESPRECALC | cmsFLAGS_BLACKPOINTCOMPENSATION);
  } else { // fast sRGB preview also uses the not optimized profile for output
    ToPreviewTransform =
      cmsCreateTransform(InProfile,
                         TYPE_RGB_16,
                         PreviewColorProfile,
                         TYPE_RGB_16,
                         Settings->GetInt("PreviewColorProfileIntent"),
                         cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);
  }

  cmsCloseProfile(InProfile);
}

////////////////////////////////////////////////////////////////////////////////
//
// Copy folder
//
////////////////////////////////////////////////////////////////////////////////

void copyFolder(QString sourceFolder, QString destFolder)
{
  QDir sourceDir(sourceFolder);
  if(!sourceDir.exists())
    return;
  QDir destDir(destFolder);
  if(!destDir.exists())
  {
    destDir.mkdir(destFolder);
  }
  QStringList files = sourceDir.entryList(QDir::Files);
  for(int i = 0; i< files.count(); i++)
  {
    QString srcName = sourceFolder + "/" + files[i];
    QString destName = destFolder + "/" + files[i];
    QFile::remove(destName);
    QFile::copy(srcName, destName);
  }
  files.clear();
  files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
  for(int i = 0; i< files.count(); i++)
  {
    QString srcName = sourceFolder + "/" + files[i];
    QString destName = destFolder + "/" + files[i];
    copyFolder(srcName, destName);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Update
//
////////////////////////////////////////////////////////////////////////////////

void Update(short Phase,
            short SubPhase      /* = -1 */,
            short WithIdentify  /* = 1 */,
            short ProcessorMode /* = ptProcessorMode_Preview */) {

  if (Settings->GetInt("BlockUpdate") == 1) return; // hard block
  if (Settings->GetInt("PipeIsRunning") == 1) {
    // record that we got here with the new settings
    // and make sure the pipe is run after the current
    // pass automatically -> timer
    return;
  } else Settings->SetValue("PipeIsRunning",1);

  if (Phase < ptProcessorPhase_NULL) {
    // main processing
    if (Phase < NextPhase) NextPhase = Phase;
    if (SubPhase > 0 && SubPhase < NextSubPhase) NextSubPhase = SubPhase;
    if (Settings->GetInt("RunMode") == 1) {
      // we're in manual mode!
      MainWindow->UpdateSettings();
    } else {
      // run processor!
      ImageSaved = 0;
      MainWindow->UpdateSettings();
      if(Settings->GetInt("HaveImage")==1) {
        if (NextPhase < ptProcessorPhase_Output)
          TheProcessor->Run(NextPhase, NextSubPhase, WithIdentify, ProcessorMode);
        UpdatePreviewImage();
      }
      NextPhase = ptProcessorPhase_Output;
      NextSubPhase = ptProcessorPhase_Highlights;
    }
  } else if (Phase == ptProcessorPhase_OnlyHistogram) {
    // only histogram update, don't care about manual mode
    UpdatePreviewImage(NULL,1);
  } else if (Phase == ptProcessorPhase_NULL) {
    // only preview update, don't care about manual mode
    UpdatePreviewImage(NULL,0);
  } else if (Phase == ptProcessorPhase_WriteOut) {
    // write output
    WriteOut();
  } else if (Phase == ptProcessorPhase_ToGimp) {
    // export
    Export(ptExportMode_GimpPipe);
  } else {
    // should not happen!
    assert(0);
  }
  Settings->SetValue("PipeIsRunning",0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Update
// Overloaded function, expects toolname and will call Update
//
////////////////////////////////////////////////////////////////////////////////

int GetProcessorPhase(const QString GuiName) {
  int Phase = 0;
  QString Tab = MainWindow->m_GroupBox->value(GuiName)->GetTabName();
  //ptMessageBox::information(0,"Feedback","I was called from \n" + GuiName + "\nMy tab is\n" Tab);
  if (Tab == "GeometryTab") Phase = ptProcessorPhase_Geometry;
  else if (Tab == "RGBTab") Phase = ptProcessorPhase_RGB;
  else if (Tab == "LabCCTab") Phase = ptProcessorPhase_LabCC;
  else if (Tab == "LabSNTab") Phase = ptProcessorPhase_LabSN;
  else if (Tab == "LabEyeCandyTab") Phase = ptProcessorPhase_LabEyeCandy;
  else if (Tab == "EyeCandyTab") Phase = ptProcessorPhase_EyeCandy;
  else if (Tab == "OutTab") Phase = ptProcessorPhase_Output;
  else Phase = ptProcessorPhase_Raw;
  return Phase;
}

void Update(const QString GuiName) {
  int Phase = GetProcessorPhase(GuiName);
  // It is assumed that no tool before white balance will use this.
  if (Phase < 2) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  } else {
    Update(Phase);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// Block tools
// 0: enable tools, 1: disable everything, 2: disable but keep crop tools enabled
//
////////////////////////////////////////////////////////////////////////////////

void BlockTools(const short state) {
  // enable all
  if (state == 0) {
    MainWindow->SpecialPreviewWidget->setEnabled(true);
    if (Settings->GetInt("BlockTools") == 1) {
      MainWindow->ControlFrame->setEnabled(true);

    } else if (Settings->GetInt("BlockTools") == 2) {
      MainWindow->HistogramFrameCentralWidget->setEnabled(true);
      MainWindow->SearchWidget->setEnabled(true);
      MainWindow->PipeControlWidget->setEnabled(true);
      MainWindow->StatusWidget->setEnabled(true);

      if (MainWindow->m_MovedTools->size() > 0) {
        // UI doesn't display tabs
        for (int i = 0; i < MainWindow->m_MovedTools->size(); i++) {
          MainWindow->m_MovedTools->at(i)->setEnabled(true);
        }

      } else {
        // UI in tab mode
        for (int i = 0; i < MainWindow->ProcessingTabBook->count(); i++) {
          if (MainWindow->ProcessingTabBook->widget(i) != MainWindow->GeometryTab) {
            MainWindow->ProcessingTabBook->setTabEnabled(i, true);
          }
        }

        QList<ptGroupBox *> GeometryTools;
        GeometryTools << MainWindow->m_GroupBox->value("TabLensfunLensParameters")
                      << MainWindow->m_GroupBox->value("TabLensfunCA")
                      << MainWindow->m_GroupBox->value("TabLensfunVignette")
                      << MainWindow->m_GroupBox->value("TabLensfunDistortion")
                      << MainWindow->m_GroupBox->value("TabLensfunGeometry")
                      << MainWindow->m_GroupBox->value("TabDefish")
                      << MainWindow->m_GroupBox->value("TabRotation")
                      << MainWindow->m_GroupBox->value("TabLiquidRescale")
                      << MainWindow->m_GroupBox->value("TabResize")
                      << MainWindow->m_GroupBox->value("TabFlip")
                      << MainWindow->m_GroupBox->value("TabBlock");
        for (int i = 0; i < GeometryTools.size(); i++) {
          GeometryTools.at(i)->SetEnabled(true);
        }
      }
    }


  // block everything
  } else if (state == 1) {
    assert(Settings->GetInt("BlockTools") == 0);
    MainWindow->SpecialPreviewWidget->setEnabled(false);
    MainWindow->ControlFrame->setEnabled(false);


  // block everything except crop tool
  } else if (state == 2) {
    assert(Settings->GetInt("BlockTools") == 0);
    MainWindow->SpecialPreviewWidget->setEnabled(false);
    MainWindow->HistogramFrameCentralWidget->setEnabled(false);
    MainWindow->PipeControlWidget->setEnabled(false);
    MainWindow->StatusWidget->setEnabled(false);
    MainWindow->SearchWidget->setEnabled(false);

    if (MainWindow->m_MovedTools->size()>0) {
      // UI doesn't display tabs
      for (int i = 0; i < MainWindow->m_MovedTools->size(); i++) {
        if (MainWindow->m_MovedTools->at(i)->objectName() != "TabCrop")
          MainWindow->m_MovedTools->at(i)->setEnabled(false);
      }

    } else {
      // UI in tab mode
      for (int i = 0; i < MainWindow->ProcessingTabBook->count(); i++) {
        if (MainWindow->ProcessingTabBook->widget(i) != MainWindow->GeometryTab) {
          MainWindow->ProcessingTabBook->setTabEnabled(i, false);
        }
      }

      QList<ptGroupBox *> GeometryTools;
      GeometryTools << MainWindow->m_GroupBox->value("TabLensfunLensParameters")
                    << MainWindow->m_GroupBox->value("TabLensfunCA")
                    << MainWindow->m_GroupBox->value("TabLensfunVignette")
                    << MainWindow->m_GroupBox->value("TabLensfunDistortion")
                    << MainWindow->m_GroupBox->value("TabLensfunGeometry")
                    << MainWindow->m_GroupBox->value("TabDefish")
                    << MainWindow->m_GroupBox->value("TabRotation")
                    << MainWindow->m_GroupBox->value("TabLiquidRescale")
                    << MainWindow->m_GroupBox->value("TabResize")
                    << MainWindow->m_GroupBox->value("TabFlip")
                    << MainWindow->m_GroupBox->value("TabBlock");
      for (int i = 0; i < GeometryTools.size(); i++) {
        GeometryTools.at(i)->SetEnabled(false);
      }
    }
  }

  Settings->SetValue("BlockTools", state);
}

////////////////////////////////////////////////////////////////////////////////
//
// Histogram
//
////////////////////////////////////////////////////////////////////////////////
void HistogramCropDone(const ptStatus ExitStatus, QRect SelectionRect);

void HistogramGetCrop() {
  // Get the crop for the histogram
  if (Settings->GetInt("HistogramCrop")) {
    // Allow to be selected in the view window. And deactivate main.
    BlockTools(1);
    ViewWindow->ShowStatus(QObject::tr("Selection"));
    ViewWindow->StartSimpleRect(HistogramCropDone);
    ViewWindow->setFocus();
  } else {
    ReportProgress(QObject::tr("Updating histogram"));
    Update(ptProcessorPhase_NULL);
    ReportProgress(QObject::tr("Ready"));
  }
}

void HistogramCropDone(const ptStatus ExitStatus, QRect SelectionRect) {
  // Selection is done at this point. Disallow it further and activate main.
  BlockTools(0);
  if (ExitStatus == stFailure) {
    Settings->SetValue("HistogramCropX",0);
    Settings->SetValue("HistogramCropY",0);
    Settings->SetValue("HistogramCropW",0);
    Settings->SetValue("HistogramCropH",0);
    Settings->SetValue("HistogramCrop",0);
    return;
  }

  short XScale = 1<<Settings->GetInt("PipeSize");
  short YScale = 1<<Settings->GetInt("PipeSize");

  Settings->SetValue("HistogramCropX", SelectionRect.left() * XScale);
  Settings->SetValue("HistogramCropY", SelectionRect.top() * YScale);
  Settings->SetValue("HistogramCropW", SelectionRect.width() * XScale);
  Settings->SetValue("HistogramCropH", SelectionRect.height() * YScale);

  // Check if the chosen area is large enough
  if (Settings->GetInt("HistogramCropW") < 50 || Settings->GetInt("HistogramCropH") < 50) {
    ptMessageBox::information(0,
      QObject::tr("Selection too small"),
      QObject::tr("Selection rectangle needs to be at least 50x50 pixels in size.\nNo crop, try again."));
    Settings->SetValue("HistogramCropX",0);
    Settings->SetValue("HistogramCropY",0);
    Settings->SetValue("HistogramCropW",0);
    Settings->SetValue("HistogramCropH",0);
    Settings->SetValue("HistogramCrop",0);
  }

  ReportProgress(QObject::tr("Updating histogram"));
  Update(ptProcessorPhase_NULL);
  ReportProgress(QObject::tr("Ready"));
}


////////////////////////////////////////////////////////////////////////////////
//
// Status report in Viewwindow
//
////////////////////////////////////////////////////////////////////////////////

void ViewWindowShowStatus(short State) {
  if (ViewWindow) {
    ViewWindow->ShowStatus(State);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Operations done right before gamma and profile for whatever reason
// Finalrun to allow filters only on the final run
// Resize to forbid resizing (-> histogram only on crop)
//
////////////////////////////////////////////////////////////////////////////////

void BeforeGamma(ptImage* Image, const short FinalRun = 0, const short Resize = 1) {

  if (Settings->GetInt("WebResizeBeforeGamma")==1 && Resize) {
    if (FinalRun == 1) Settings->SetValue("FullOutput",1);
    if (Settings->ToolIsActive("TabWebResize")) {
      ReportProgress(QObject::tr("WebResizing"));
      //~ Image->FilteredResize(Settings->GetInt("WebResizeScale"),Settings->GetInt("WebResizeFilter"));
      Image->ptGMResize(Settings->GetInt("WebResizeScale"),
                        Settings->GetInt("WebResizeFilter"),
                        Settings->GetInt("WebResizeDimension"));
    }
    if (FinalRun == 1) Settings->SetValue("FullOutput",0);
  }

  // TODO put these curves together as devicelink into lcms

  // BaseCurve.
  if (Settings->ToolIsActive("TabBaseCurve")) {
    ReportProgress(QObject::tr("Applying base curve"));
    Image->ApplyCurve(Curve[ptCurveChannel_Base],7);
  }

  //GammaCompensation
  if (Settings->ToolIsActive("TabGammaCompensation")) {
    ReportProgress(QObject::tr("Applying gamma compensation"));
    ptCurve* CompensationCurve = new ptCurve();
    CompensationCurve->SetCurveFromFunction(DeltaGammaTool,Settings->GetDouble("OutputGamma"),
              Settings->GetDouble("OutputLinearity"));
    CompensationCurve->m_Type = ptCurveType_Full;
    CompensationCurve->m_IntendedChannel = ptCurveChannel_Base2;
    Image->ApplyCurve(CompensationCurve,7);
    delete CompensationCurve;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Operations done after gamma and profile for whatever reason
// Finalrun to allow filters only on the final run
// Resize to forbid resizing (-> histogram only on crop)
//
////////////////////////////////////////////////////////////////////////////////

void AfterAll(ptImage* Image, const short FinalRun = 0, const short Resize = 1) {
  // Sigmoidal contrast
  if (Settings->ToolIsActive("TabOutContrast")) {
    ReportProgress(QObject::tr("Applying RGB Contrast"));
    Image->SigmoidalContrast(Settings->GetDouble("RGBContrast3Amount"),
                                             Settings->GetDouble("RGBContrast3Threshold"));
  }

  // After gamma curve
  if (Settings->ToolIsActive("TabAfterGammaCurve")) {
    ReportProgress(QObject::tr("Applying after gamma curve"));
    Image->ApplyCurve(Curve[ptCurveChannel_Base2],7);
  }

  // WebResize for quality reasons done after output profile
  if (Settings->GetInt("WebResizeBeforeGamma")==0 && Resize) {
    if (FinalRun == 1) Settings->SetValue("FullOutput",1);
    if (Settings->ToolIsActive("TabWebResize")) {
      ReportProgress(QObject::tr("WebResizing"));
      Image->ptGMResize(Settings->GetInt("WebResizeScale"),
                        Settings->GetInt("WebResizeFilter"),
                        Settings->GetInt("WebResizeDimension"));
    }
    if (FinalRun == 1) Settings->SetValue("FullOutput",0);
  }
}

void EndSharpen(ptImage* Image, cmsHPROFILE Profile, const int Intent) {
  ReportProgress(QObject::tr("Wiener Filter"));
  int InColorSpace = Image->m_ColorSpace;
  cmsHPROFILE InternalProfile = 0;
  if (Profile == NULL || InColorSpace != ptSpace_Profiled) {
    // linear case
    cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
    cmsToneCurve* Gamma3[3];
    Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

    cmsCIExyY       DFromReference;

    switch (Image->m_ColorSpace) {
      case ptSpace_sRGB_D65 :
      case ptSpace_AdobeRGB_D65 :
        DFromReference = D65;
        break;
      case ptSpace_WideGamutRGB_D50 :
      case ptSpace_ProPhotoRGB_D50 :
        DFromReference = D50;
        break;
      default:
        assert(0);
    }

    InternalProfile = cmsCreateRGBProfile(&DFromReference,
                                    (cmsCIExyYTRIPLE*)&RGBPrimaries[Image->m_ColorSpace],
                                    Gamma3);

    if (!InternalProfile) {
      ptLogError(ptError_Profile,"Could not open InternalProfile profile.");
      return;
    }

    cmsFreeToneCurve(Gamma);
  } else {
    // profiled case
    InternalProfile = Profile;
  }

  cmsHPROFILE LabProfile = 0;
  LabProfile = cmsCreateLab4Profile(NULL);
  // to Lab
  cmsHTRANSFORM Transform;
  Transform = cmsCreateTransform(InternalProfile,
                                 TYPE_RGB_16,
                                 LabProfile,
                                 TYPE_Lab_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  int32_t Size = Image->m_Width*Image->m_Height;
  int32_t Step = 100000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(Image->m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }
  Image->m_ColorSpace = ptSpace_Lab;
  // Wiener Filter
  ptWienerFilterChannel(Image,
                        Settings->GetDouble("WienerFilter2Gaussian"),
                        Settings->GetDouble("WienerFilter2Box"),
                        Settings->GetDouble("WienerFilter2LensBlur"),
                        Settings->GetDouble("WienerFilter2Amount"),
                        Settings->GetInt("WienerFilter2UseEdgeMask"));

  // to RGB
  Transform = cmsCreateTransform(LabProfile,
                                 TYPE_Lab_16,
                                 InternalProfile,
                                 TYPE_RGB_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(Image->m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }

  cmsDeleteTransform(Transform);
  cmsCloseProfile(LabProfile);
  if (Profile == NULL || InColorSpace != ptSpace_Profiled)
    cmsCloseProfile(InternalProfile);
  Image->m_ColorSpace = InColorSpace;
}

////////////////////////////////////////////////////////////////////////////////
//
// Determine and update the preview image.
//
// ForcedImage claims that this image should be previewed.
// (somehow of a hack : it is used for the Crop function where
// we want the user to present fast a  new image to be cropped without
// having to calculate through the whole pipe).
//
// OnlyHistogram : hack to sync up the histogram when it was inactive.
//
////////////////////////////////////////////////////////////////////////////////

void UpdatePreviewImage(const ptImage* ForcedImage   /* = NULL  */,
                        const short    OnlyHistogram /* = false */) {

  if (!PreviewImage) PreviewImage = new (ptImage);

  // If we don't have yet a m_Image_AfterGeometry we are in
  // startup condition and show the start screen
  if (!TheProcessor->m_Image_AfterGeometry) {
    MainWindow->ViewFrameStackedWidget->setCurrentWidget(MainWindow->ViewStartPage);
    return;
  }

  MainWindow->ViewFrameStackedWidget->setCurrentWidget(MainWindow->ViewFrameCentralWidget);

  // Fast display of an image, e.g. for cropping
  // -> no histogram
  // -> only exposure for better display
  // -> transfer to preview color space
  if (ForcedImage) {
    PreviewImage->Set(ForcedImage);
    if (Settings->GetDouble("CropExposure") != 0.0) {
      float Factor = powf(2,Settings->GetDouble("CropExposure"));
      PreviewImage->Expose(Factor,ptExposureClipMode_Ratio);
    }
    BeforeGamma(PreviewImage,0,0);

    // Convert from working space to screen space.
    // Using lcms and a standard sRGB or custom profile.
    ptImage* ReturnValue = PreviewImage->lcmsRGBToPreviewRGB(Settings->GetInt("CMQuality") == ptCMQuality_FastSRGB);
    if (!ReturnValue) {
      ptLogError(ptError_lcms,"lcmsRGBToPreviewRGB");
      assert(ReturnValue);
    }
    AfterAll(PreviewImage,0,0);

    ViewWindow->UpdateImage(PreviewImage);
    ReportProgress(QObject::tr("Ready"));
    return;
  }

  ViewWindow->ShowStatus(ptStatus_Updating);
  ReportProgress(QObject::tr("Updating preview image"));

  if (!HistogramImage) HistogramImage = new (ptImage);

  short ActiveTab = MainWindow->GetCurrentTab();

  Settings->SetValue("ShowExposureIndicatorSensor",0);

  // Determine first what is the current image.
  if (ForcedImage) {
    PreviewImage->Set(ForcedImage);
  } else if (Settings->GetInt("PreviewMode") == ptPreviewMode_End) {
    PreviewImage->Set(TheProcessor->m_Image_AfterEyeCandy);
  } else {
    if (!Settings->GetInt("IsRAW")) ActiveTab = MAX(ptGeometryTab, ActiveTab);
    switch (ActiveTab) {
      case ptCameraTab:
        Settings->SetValue("ShowExposureIndicatorSensor",1);
        if (Settings->GetInt("ExposureIndicatorSensor")) {
          PreviewImage->Set(TheDcRaw,
                             Settings->GetInt("WorkColor"));
        } else {
          PreviewImage->Set(TheProcessor->m_Image_AfterDcRaw);
        }
        break;
      case ptGeometryTab:
        PreviewImage->Set(TheProcessor->m_Image_AfterGeometry);
        break;
      case ptRGBTab:
        PreviewImage->Set(TheProcessor->m_Image_AfterRGB);
        break;
      case ptLabCCTab:
        PreviewImage->Set(TheProcessor->m_Image_AfterLabCC);
        break;
      case ptLabSNTab:
        PreviewImage->Set(TheProcessor->m_Image_AfterLabSN);
        break;
      case ptLabEyeCandyTab:
        PreviewImage->Set(TheProcessor->m_Image_AfterLabEyeCandy);
        break;
      case ptEyeCandyTab:
      case ptOutTab:
        PreviewImage->Set(TheProcessor->m_Image_AfterEyeCandy);
        break;
      default:
        // Should not happen.
        assert(0);
    }
  }

  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Linear) {
    HistogramImage->Set(PreviewImage);
    // If we are in a Tab preview mode and in the lab mode
    // we do the conversion to Lab in case it would not have been
    // done yet (due to suppressed for speed in absense of USM or L Curve).
    // This way the histogram is an L Histogram at this point.
    if ( (Settings->GetInt("PreviewMode") == ptPreviewMode_Tab) &&
         (ActiveTab == ptLabCCTab || ActiveTab == ptLabSNTab || ActiveTab == ptLabEyeCandyTab) &&
         (HistogramImage->m_ColorSpace != ptSpace_Lab) ) {
      HistogramImage->RGBToLab();
    }
  } else if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output &&
             !(Settings->GetInt("HistogramCrop") && !Settings->GetInt("WebResize"))) {
    HistogramImage->Set(PreviewImage);
  } else if (Settings->GetInt("HistogramCrop")) {
    HistogramImage->Set(PreviewImage);
  }

  if (PreviewImage->m_ColorSpace == ptSpace_Lab)
    PreviewImage->LabToRGB(Settings->GetInt("WorkColor"));

  uint16_t Width = 0;
  uint16_t Height = 0;
  uint16_t TempCropX = 0;
  uint16_t TempCropY = 0;
  uint16_t TempCropW = 0;
  uint16_t TempCropH = 0;

  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Linear) {
    ReportProgress(QObject::tr("Updating histogram"));

    if (Settings->GetInt("HistogramCrop")) {
      float TmpScaled = TheProcessor->m_ScaleFactor;
      Width = HistogramImage->m_Width;
      Height = HistogramImage->m_Height;
      TempCropX = Settings->GetInt("HistogramCropX")*TmpScaled;
      TempCropY = Settings->GetInt("HistogramCropY")*TmpScaled;
      TempCropW = Settings->GetInt("HistogramCropW")*TmpScaled;
      TempCropH = Settings->GetInt("HistogramCropH")*TmpScaled;
      if ((((TempCropX) + (TempCropW)) > Width) ||
          (((TempCropY) + (TempCropH)) >  Height)) {
        ptMessageBox::information(MainWindow,
               QObject::tr("Histogram selection outside the image"),
               QObject::tr("Histogram selection rectangle too large.\nNo crop, try again."));
        Settings->SetValue("HistogramCropX",0);
        Settings->SetValue("HistogramCropY",0);
        Settings->SetValue("HistogramCropW",0);
        Settings->SetValue("HistogramCropH",0);
        Settings->SetValue("HistogramCrop",0);
      } else {
        HistogramImage->Crop(TempCropX, TempCropY, TempCropW, TempCropH);
      }
    }

    // In case of histogram update only, we're done.
    if (OnlyHistogram) {
      ViewWindow->ShowStatus(ptStatus_Done);
      return;
    }
  }

  uint8_t ExposureChannelMask = 0;
  uint16_t OverExposureLevel[3];
  uint16_t UnderExposureLevel[3];
  if (Settings->GetInt("ExposureIndicator")) {
    ReportProgress(QObject::tr("Indicating exposure"));

    if (Settings->GetInt("ExposureIndicatorR")) ExposureChannelMask |= 1;
    if (Settings->GetInt("ExposureIndicatorG")) ExposureChannelMask |= 2;
    if (Settings->GetInt("ExposureIndicatorB")) ExposureChannelMask |= 4;

    if (ActiveTab == ptCameraTab && Settings->GetInt("ExposureIndicatorSensor")){
      OverExposureLevel[0] = CLIP((int32_t)
        ((TheDcRaw->m_WhiteLevel_AfterPhase1-TheDcRaw->m_BlackLevel_AfterPhase1)
        *TheDcRaw->m_Multipliers[0]));
      OverExposureLevel[1] = CLIP((int32_t)
        ((TheDcRaw->m_WhiteLevel_AfterPhase1-TheDcRaw->m_BlackLevel_AfterPhase1)
        *TheDcRaw->m_Multipliers[1]));
      OverExposureLevel[2] = CLIP((int32_t)
        ((TheDcRaw->m_WhiteLevel_AfterPhase1-TheDcRaw->m_BlackLevel_AfterPhase1)
        *TheDcRaw->m_Multipliers[2]));
      UnderExposureLevel[0] = 0x0000;
      UnderExposureLevel[1] = 0x0000;
      UnderExposureLevel[2] = 0x0000;
    } else {
      OverExposureLevel[0] = 0xfff0;
      OverExposureLevel[1] = 0xfff0;
      OverExposureLevel[2] = 0xfff0;
      UnderExposureLevel[0] = 0x000f;
      UnderExposureLevel[1] = 0x000f;
      UnderExposureLevel[2] = 0x000f;
    }
  }

  if (!ForcedImage) {
    BeforeGamma(PreviewImage);
  }

  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output) {

    ReportProgress(QObject::tr("Updating histogram"));
    if (Settings->GetInt("HistogramCrop") && !Settings->GetInt("WebResize")) {
      HistogramImage->Set(PreviewImage);
    } else {
      BeforeGamma(HistogramImage,0,0);
    }

    ReportProgress(QObject::tr("Converting to output space"));

    cmsHPROFILE OutputColorProfile = NULL;
    OutputColorProfile = cmsOpenProfileFromFile(
                         Settings->GetString("OutputColorProfile").toAscii().data(), "r");
    if (!OutputColorProfile) {
      ptLogError(ptError_FileOpen,
        Settings->GetString("OutputColorProfile").toAscii().data());
      assert(OutputColorProfile);
    }

    // Convert from working space to screen space.
    // Using lcms and a standard sRGB or custom profile.

    ptImage* ReturnValue = HistogramImage->lcmsRGBToRGB(
      OutputColorProfile,
      Settings->GetInt("OutputColorProfileIntent"),
      Settings->GetInt("CMQuality"));
    if (!ReturnValue) {
      ptLogError(ptError_lcms,"lcmsRGBToRGB(OutputColorProfile)");
      assert(ReturnValue);
    }

    if (Settings->GetInt("HistogramCrop") && !Settings->GetInt("WebResize")) {
      AfterAll(HistogramImage);
      if (Settings->ToolIsActive("TabOutWiener") && Settings->GetInt("PreviewMode") == ptPreviewMode_End)
        EndSharpen(HistogramImage, OutputColorProfile, Settings->GetInt("OutputColorProfileIntent"));
    } else {
      AfterAll(HistogramImage,0,0);
      if (Settings->ToolIsActive("TabOutWiener") && Settings->GetInt("PreviewMode") == ptPreviewMode_End)
        EndSharpen(HistogramImage, OutputColorProfile, Settings->GetInt("OutputColorProfileIntent"));
    }

    // Close the output profile.
    cmsCloseProfile(OutputColorProfile);

    if (Settings->GetInt("HistogramCrop")) {
      float TmpScaled = TheProcessor->m_ScaleFactor;
      Width = HistogramImage->m_Width;
      Height = HistogramImage->m_Height;
      TempCropX = Settings->GetInt("HistogramCropX")*TmpScaled;
      TempCropY = Settings->GetInt("HistogramCropY")*TmpScaled;
      TempCropW = Settings->GetInt("HistogramCropW")*TmpScaled;
      TempCropH = Settings->GetInt("HistogramCropH")*TmpScaled;
      if ((((TempCropX) + (TempCropW)) >  Width) ||
          (((TempCropY) + (TempCropH)) >  Height)) {
        ptMessageBox::information(MainWindow,
          QObject::tr("Histogram selection outside the image"),
          QObject::tr("Histogram selection rectangle too large.\nNo crop, try again."));
        Settings->SetValue("HistogramCropX",0);
        Settings->SetValue("HistogramCropY",0);
        Settings->SetValue("HistogramCropW",0);
        Settings->SetValue("HistogramCropH",0);
        Settings->SetValue("HistogramCrop",0);
      } else {
        HistogramImage->Crop(TempCropX, TempCropY, TempCropW, TempCropH);
      }
    }
    if (OnlyHistogram) {
      ViewWindow->ShowStatus(ptStatus_Done);
      return;
    }
  }

  ReportProgress(QObject::tr("Converting to screen space"));

  // Convert from working space to screen space.
  // Using lcms and a standard sRGB or custom profile.
  PreviewImage->lcmsRGBToPreviewRGB(Settings->GetInt("CMQuality") == ptCMQuality_FastSRGB);

  if (!ForcedImage) {
    AfterAll(PreviewImage);
    if (Settings->ToolIsActive("TabOutWiener") && Settings->GetInt("PreviewMode") == ptPreviewMode_End)
      EndSharpen(PreviewImage, PreviewColorProfile, Settings->GetInt("PreviewColorProfileIntent"));
  }

  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Preview) {
    if (!Settings->GetInt("HistogramCrop") ||
        (Settings->GetInt("HistogramCrop") && !Settings->GetInt("WebResize"))) {
      HistogramImage->Set(PreviewImage);
    } else if (Settings->GetInt("HistogramCrop")) {
      BeforeGamma(HistogramImage,0,0);

      ptImage* ReturnValue = HistogramImage->lcmsRGBToPreviewRGB(Settings->GetInt("CMQuality") == ptCMQuality_FastSRGB);
      if (!ReturnValue) {
        ptLogError(ptError_lcms,"lcmsRGBToPreviewRGB");
        assert(ReturnValue);
      }

      AfterAll(HistogramImage,0,0);
      if (Settings->ToolIsActive("TabOutWiener") && Settings->GetInt("PreviewMode") == ptPreviewMode_End)
        EndSharpen(HistogramImage, PreviewColorProfile, Settings->GetInt("PreviewColorProfileIntent"));
    }

    if (Settings->GetInt("HistogramCrop")) {
      float TmpScaled = TheProcessor->m_ScaleFactor;
      Width = HistogramImage->m_Width;
      Height = HistogramImage->m_Height;
      TempCropX = Settings->GetInt("HistogramCropX")*TmpScaled;
      TempCropY = Settings->GetInt("HistogramCropY")*TmpScaled;
      TempCropW = Settings->GetInt("HistogramCropW")*TmpScaled;
      TempCropH = Settings->GetInt("HistogramCropH")*TmpScaled;
      if ((((TempCropX) + (TempCropW)) >  Width) ||
          (((TempCropY) + (TempCropH)) >  Height)) {
        ptMessageBox::information(MainWindow,
          QObject::tr("Histogram selection outside the image"),
          QObject::tr("Histogram selection rectangle too large.\nNo crop, try again."));
        Settings->SetValue("HistogramCropX",0);
        Settings->SetValue("HistogramCropY",0);
        Settings->SetValue("HistogramCropW",0);
        Settings->SetValue("HistogramCropH",0);
        Settings->SetValue("HistogramCrop",0);
      } else {
        HistogramImage->Crop(TempCropX, TempCropY, TempCropW, TempCropH);
      }
    }
  }

  if (!OnlyHistogram) {
    if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output) {
      if (Settings->GetInt("ExposureIndicator"))
        PreviewImage->IndicateExposure(HistogramImage,
                                      Settings->GetInt("ExposureIndicatorOver"),
                                      Settings->GetInt("ExposureIndicatorUnder"),
                                      ExposureChannelMask,
                                      OverExposureLevel,
                                      UnderExposureLevel);
    } else {
      if (Settings->GetInt("ExposureIndicator"))
        PreviewImage->IndicateExposure(Settings->GetInt("ExposureIndicatorOver"),
                                      Settings->GetInt("ExposureIndicatorUnder"),
                                      ExposureChannelMask,
                                      OverExposureLevel,
                                      UnderExposureLevel);
    }

    // Get a special preview
    if (Settings->GetInt("SpecialPreview"))
      PreviewImage->SpecialPreview(Settings->GetInt("SpecialPreview"),
                                   Settings->GetInt("PreviewColorProfileIntent"));

    // Update the ViewWindow and show if needed.
    ViewWindow->UpdateImage(PreviewImage);
  }

  HistogramWindow->UpdateView(HistogramImage);
  ViewWindow->ShowStatus(ptStatus_Done);

  ReportProgress(QObject::tr("Ready"));

  if (!OnlyHistogram)
    if (Settings->GetInt("WriteBackupSettings"))
      WriteSettingsFile(Settings->GetString("UserDirectory")+"backup.pts");
}

////////////////////////////////////////////////////////////////////////////////
//
// Update comboboxes of curves and channelmixer
// Checks if files are available
//
////////////////////////////////////////////////////////////////////////////////

void UpdateComboboxes(const QString Key) {
  QStringList ListCombos;
  ListCombos << CurveKeys
             << "ChannelMixer";
  if (!ListCombos.contains(Key)) return;

  // get current selection
  int Index = Settings->GetInt(Key) - 2;
  QStringList FileNames;
  if (Key == "ChannelMixer") {
    FileNames = Settings->GetStringList("ChannelMixerFileNames");
  } else { // Curves
    FileNames = Settings->GetStringList(CurveFileNamesKeys.at(CurveKeys.indexOf(Key)));
  }

  //save current selection
  QString CurrentIndex;
  if (Index > -1) CurrentIndex = FileNames.at(Index);

  FileNames.removeDuplicates();

  // check if files are available
  if (Key == "ChannelMixer") {
    ptChannelMixer Tmp;
    for (int i = 0; i < FileNames.size(); i++) {
      if (Tmp.ReadChannelMixer(FileNames.at(i).toAscii().data())) {
        if (Settings->GetInt("JobMode") == 0) {
          ptMessageBox::warning(MainWindow,
                               QObject::tr("Channelmixer read error"),
                               QObject::tr("Cannot read channelmixer ")
                                 + " '" + FileNames.at(i) + "'");
        }
        FileNames.replace(i, "@DELETEME@");
      }
    }
  } else {
    ptCurve Tmp;
    for (int i = 0; i < FileNames.size(); i++) {
      if (Tmp.ReadCurve(FileNames.at(i).toAscii().data())) {
        if (Settings->GetInt("JobMode") == 0) {
          ptMessageBox::warning(MainWindow,
                               QObject::tr("Curve read error"),
                               QObject::tr("Cannot read curve ")
                                 + " '" + FileNames.at(i) + "'");
        }
        FileNames.replace(i, "@DELETEME@");
      }
    }
  }
  FileNames.removeAll("@DELETEME@");

  // check if current selection is still available
  if (Index > -1) {
    if (FileNames.contains(CurrentIndex)) {
      Index = FileNames.indexOf(CurrentIndex);
      Settings->SetValue(Key, Index + 2);
    } else {
      Settings->SetValue(Key, 0);
    }
  }

  // Set combos in gui
  Settings->ClearOptions(Key, 1);
  for (int i = 0; i < FileNames.size(); i++) {
    // Small routine that lets Shortfilename point to the basename.
    QFileInfo PathInfo(FileNames[i]);
    QString ShortFileName = PathInfo.baseName().left(18);

    // Curves and Channelmixer have 2 default options -> index=2+i
    Settings->AddOrReplaceOption(Key, ShortFileName, 2+i);
  }

  // write clean lists to settings
  if (Key == "ChannelMixer") {
    Settings->SetValue("ChannelMixerFileNames", FileNames);
  } else { // Curves
    Settings->SetValue(CurveFileNamesKeys.at(CurveKeys.indexOf(Key)), FileNames);
  }

  // update gui display
  Settings->SetValue(Key,Settings->GetInt(Key));
}

////////////////////////////////////////////////////////////////////////////////
//
// RunJob (the non-interactive 'batch' run).
//
////////////////////////////////////////////////////////////////////////////////

void RunJob(const QString JobFileName) {
  Settings->SetValue("JobMode",1);
  // Init
  CB_Event0();

  // Read the gui settings from a file.
  short NextPhase = 1;
  short ReturnValue = ReadSettingsFile(JobFileName, NextPhase);
  if (ReturnValue) {
    printf("\nNo valid job file!\n\n");
    return;
  }

  QStringList InputFileNameList = Settings->GetStringList("InputFileNameList");
  assert(InputFileNameList.size());

  do {
    // Test if we can handle the file
    DcRaw* TestDcRaw = new(DcRaw);
    Settings->ToDcRaw(TestDcRaw);
    int OpenError = 0;
    uint16_t InputWidth = 0;
    uint16_t InputHeight = 0;
    if (TestDcRaw->Identify()) { // Bitmap
      try {
        Magick::Image image;

        image.ping(InputFileNameList[0].toAscii().data());

        InputWidth = image.columns();
        InputHeight = image.rows();
      } catch (Magick::Exception &Error) {
        OpenError = 1;
      }
      if (OpenError == 0) {
        Settings->SetValue("IsRAW",0);
        Settings->SetValue("ExposureNormalization",0.0);
      }
    } else {
      Settings->SetValue("IsRAW",1);
    }
    if (OpenError == 1) {
      // We don't have a RAW or a bitmap!
      QString ErrorMessage = QObject::tr("Cannot decode")
                           + " '"
                           + InputFileNameList[0]
                           + "'" ;
      printf("%s\n",ErrorMessage.toAscii().data());
      delete TestDcRaw;
    } else { // process
      QFileInfo PathInfo(InputFileNameList[0]);
      if (!Settings->GetString("OutputDirectory").isEmpty()) {
        Settings->SetValue("OutputFileName",
          Settings->GetString("OutputDirectory") + "/" + PathInfo.baseName());
      } else {
        Settings->SetValue("OutputFileName",
          PathInfo.dir().path() + "/" + PathInfo.baseName());
      }
      if (!Settings->GetInt("IsRAW")) {
        Settings->SetValue("OutputFileName",
                           Settings->GetString("OutputFileName") + "-new");
      }

      // Here we have the OutputFileName, but extension still to add.
      switch(Settings->GetInt("SaveFormat")) {
        case ptSaveFormat_JPEG :
          Settings->SetValue("OutputFileName",
                             Settings->GetString("OutputFileName") + ".jpg");
          break;
        case ptSaveFormat_PNG :
        case ptSaveFormat_PNG16 :
          Settings->SetValue("OutputFileName",
                             Settings->GetString("OutputFileName") + ".png");
          break;
        case ptSaveFormat_TIFF8 :
        case ptSaveFormat_TIFF16 :
          Settings->SetValue("OutputFileName",
                             Settings->GetString("OutputFileName") + ".tif");
          break;
        default :
          Settings->SetValue("OutputFileName",
                             Settings->GetString("OutputFileName") + ".ppm");
          break;
      }

      // Processing the job.
      delete TheDcRaw;
      delete TheProcessor;
      TheProcessor = new ptProcessor(ReportProgress);
      TheDcRaw = TestDcRaw;
      if (Settings->GetInt("IsRAW")==0) {
        Settings->SetValue("ImageW",InputWidth);
        Settings->SetValue("ImageH",InputHeight);
      } else {
        Settings->SetValue("ImageW",TheDcRaw->m_Width);
        Settings->SetValue("ImageH",TheDcRaw->m_Height);
      }

      TheProcessor->m_DcRaw = TheDcRaw;

      Settings->SetValue("RunMode",0);
      Settings->SetValue("FullOutput",1);
      TheProcessor->Run(ptProcessorPhase_Raw,ptProcessorPhase_Load,0,1);
      Settings->SetValue("FullOutput",0);
      // And write result.
      Update(ptProcessorPhase_WriteOut);
    }

    // Loop over the inputfiles by shifting the next one to index 0
    if (InputFileNameList.size()) {
      InputFileNameList.removeAt(0);
    }

    // Write the changed InputFileNameList back to the settings.
    Settings->SetValue("InputFileNameList",InputFileNameList);

  } while (InputFileNameList.size());
}

////////////////////////////////////////////////////////////////////////////////
//
// PrepareTags
//
////////////////////////////////////////////////////////////////////////////////

void PrepareTags(const QString TagsInput) {

  QString WorkString = TagsInput;
  WorkString.replace("\n",",");
  //~ WorkString.replace(",",", ");
  while (WorkString.contains("  "))
    WorkString.replace("  "," ");
  if (WorkString.startsWith(" ")) WorkString.remove(0,1);
  if (WorkString.endsWith(" ")) WorkString.remove(WorkString.size()-1,1);
  while (WorkString.contains("//"))
    WorkString.replace("//","/");
  while (WorkString.contains("/ /"))
    WorkString.replace("/ /","/");
  if (WorkString.endsWith("/")) WorkString.remove(WorkString.size()-1,1);
  WorkString.replace(" ,",",");
  WorkString.replace("/,",",");
  WorkString.replace(" ,",","); // We need this again!
  WorkString.replace(" /","/");
  WorkString.replace("/ ","/");
  WorkString.replace(",/",",");
  WorkString.replace(", ",",");
  if (WorkString.startsWith("/")) WorkString.remove(0,1);

  QStringList DigikamTags = WorkString.split(",", QString::SkipEmptyParts);

  QStringList Tags;
  QString TempString;
  for (int i = 0; i < DigikamTags.size(); i++) {
    TempString = DigikamTags.at(i);
    TempString.remove(0,TempString.lastIndexOf("/")+1);
    if (TempString.startsWith(" ")) TempString.remove(0,1);
    Tags << TempString;
  }

  Settings->SetValue("DigikamTagsList", DigikamTags);
  Settings->SetValue("TagsList", Tags);

  //~ WorkString += "\n";
  //~ for (int i = 0; i < DigikamTags.size(); i++) {
    //~ if (i) WorkString.append(",");
    //~ WorkString.append(DigikamTags.at(i));
    //~ WorkString.append("...");
    //~ WorkString.append(Tags.at(i));
  //~ }
  //~ ptMessageBox::warning(0,"Tags",WorkString);

  return;
}

////////////////////////////////////////////////////////////////////////////////
//
// WriteExif
// Append exif to an image file
//
////////////////////////////////////////////////////////////////////////////////

void WriteExif(const QString FileName, uint8_t* ExifBuffer, const unsigned ExifBufferLength) {

#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
  try {
    if (ExifBufferLength) {

      // Open the raw again for full exif data
      //~ Exiv2::Image::AutoPtr InImage =
      //~ Exiv2::ImageFactory::open((Settings->GetStringList("InputFileNameList"))[0].toAscii().data());
      //~ assert(InImage.get() != 0);
      //~ InImage->readMetadata();

      const unsigned char ExifHeader[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

      unsigned char*  Buffer;
      unsigned long   BufferLength;

      BufferLength = ExifBufferLength-sizeof(ExifHeader);

      Buffer = (unsigned char*) MALLOC(BufferLength);
      ptMemoryError(Buffer,__FILE__,__LINE__);

      Exiv2::ExifData exifData;

      memcpy(Buffer,ExifBuffer+sizeof(ExifHeader), BufferLength);

      Exiv2::ExifParser::decode(exifData, Buffer, BufferLength);

      // Reset orientation
      Exiv2::ExifData::iterator pos = exifData.begin();
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation")))
      != exifData.end() ) {
        pos->setValue("1"); // Normal orientation
      }

      // Code from UFRaw, necessary for Tiff files
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.ImageWidth")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.ImageLength")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.BitsPerSample")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.Compression")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.PhotometricInterpretation")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.FillOrder")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.SamplesPerPixel")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.StripOffsets")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.RowsPerStrip")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.StripByteCounts")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.XResolution")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.YResolution")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.PlanarConfiguration")))
      != exifData.end() )
        exifData.erase(pos);
      if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.ResolutionUnit")))
      != exifData.end() )
        exifData.erase(pos);

      if (Settings->GetInt("EraseExifThumbnail")) {
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
        Exiv2::ExifThumb Thumb(exifData);
        Thumb.erase();
#else
        exifData.eraseThumbnail();
#endif
      }

      QString JpegExtensions[] = {"jpg", "JPG", "Jpg", "jpeg", "Jpeg", "JPEG"};
      short deleteDNGdata = 0;
      for (int i=0; i<6; i++) if (!FileName.endsWith(JpegExtensions[i])) deleteDNGdata = 1;

      Exiv2::Image::AutoPtr Exiv2Image = Exiv2::ImageFactory::open(FileName.toAscii().data());

      Exiv2Image->readMetadata();
      Exiv2::ExifData &outExifData = Exiv2Image->exifData();
      pos = exifData.begin();
      while ( !exifData.empty() ) {
        if (deleteDNGdata == 0 || (*pos).key() != "Exif.Image.DNGPrivateData") {
          outExifData.add(*pos);
        }
        pos = exifData.erase(pos);
      }

      if (Settings->GetInt("JobMode") == 0)
        PrepareTags(MainWindow->TagsEditWidget->toPlainText());

      // IPTC data
      Exiv2::IptcData iptcData;

      QStringList Tags = Settings->GetStringList("TagsList");
      QStringList DigikamTags = Settings->GetStringList("DigikamTagsList");

      Exiv2::StringValue StringValue;
      for (int i = 0; i < Tags.size(); i++) {
        StringValue.read(Tags.at(i).toStdString());
        iptcData.add(Exiv2::IptcKey("Iptc.Application2.Keywords"), &StringValue);
      }

      // XMP data
      Exiv2::XmpData xmpData;

      if (Settings->GetInt("ImageRating"))
        xmpData["Xmp.xmp.Rating"] = Settings->GetInt("ImageRating");
      for (int i = 0; i < Tags.size(); i++) {
        xmpData["Xmp.dc.subject"] = Tags.at(i).toStdString();
      }
      for (int i = 0; i < DigikamTags.size(); i++) {
        xmpData["Xmp.digiKam.TagsList"] = DigikamTags.at(i).toStdString();
      }

      // Program name
      iptcData["Iptc.Application2.Program"] = ProgramName;
      iptcData["Iptc.Application2.ProgramVersion"] = "idle";
      xmpData["Xmp.xmp.CreatorTool"] = ProgramName;
      xmpData["Xmp.tiff.Software"] = ProgramName;

      // Title
      if (Settings->GetInt("JobMode") == 0)
        Settings->SetValue("ImageTitle",MainWindow->TitleEditWidget->text());
      QString TitleWorking = Settings->GetString("ImageTitle");
      while (TitleWorking.contains("  "))
        TitleWorking.replace("  "," ");
      if (TitleWorking != "" && TitleWorking != " ") {
        outExifData["Exif.Photo.UserComment"] = TitleWorking.toStdString();
        iptcData["Iptc.Application2.Caption"] = TitleWorking.toStdString();
        xmpData["Xmp.dc.description"] = TitleWorking.toStdString();
        xmpData["Xmp.exif.UserComment"] = TitleWorking.toStdString();
        xmpData["Xmp.tiff.ImageDescription"] = TitleWorking.toStdString();
      }

      Exiv2Image->setExifData(outExifData);
      Exiv2Image->setIptcData(iptcData);
      Exiv2Image->setXmpData(xmpData);
      Exiv2Image->writeMetadata();
    }
  } catch (Exiv2::AnyError& Error) {
    if (Settings->GetInt("JobMode") == 0) {
      ptMessageBox::warning(MainWindow,"Exiv2 Error","No exif data written!\nCaught Exiv2 exception '" + QString(Error.what()) + "'\n");
    } else {
      std::cout << "Caught Exiv2 exception '" << Error << "'\n";
    }
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// WriteOut
// Write out in one of the output formats (after applying output profile).
//
////////////////////////////////////////////////////////////////////////////////

void WriteOut() {

  ptImage* OutImage = NULL;

  if (Settings->GetInt("JobMode") == 1) {
    OutImage = TheProcessor->m_Image_AfterEyeCandy; // Job mode -> no cache
  } else {
    if (!OutImage) OutImage = new(ptImage);
    OutImage->Set(TheProcessor->m_Image_AfterEyeCandy);
  }

  BeforeGamma(OutImage, 1);

  cmsHPROFILE OutputColorProfile = NULL;

  ReportProgress(QObject::tr("Converting to output space"));

  // Prepare and open an output profile.
  OutputColorProfile = cmsOpenProfileFromFile(
    Settings->GetString("OutputColorProfile").toAscii().data(),
    "r");
  if (!OutputColorProfile) {
    ptLogError(ptError_FileOpen,
   Settings->GetString("OutputColorProfile").toAscii().data());
    assert(OutputColorProfile);
  }

  // Color space conversion
  ptImage* ReturnValue = OutImage->lcmsRGBToRGB(
    OutputColorProfile,
    Settings->GetInt("OutputColorProfileIntent"),
    Settings->GetInt("CMQuality"));
  if (!ReturnValue) {
    ptLogError(ptError_lcms,"lcmsRGBToRGB(OutputColorProfile)");
    assert(ReturnValue);
  }

  AfterAll(OutImage, 1);
  if (Settings->ToolIsActive("TabOutWiener"))
    EndSharpen(OutImage, OutputColorProfile, Settings->GetInt("OutputColorProfileIntent"));
  // Close the output profile.
  cmsCloseProfile(OutputColorProfile);

  ReportProgress(QObject::tr("Writing output"));

  bool FileWritten =  OutImage->ptGMCWriteImage(
    Settings->GetString("OutputFileName").toAscii().data(),
    Settings->GetInt("SaveFormat"),
    Settings->GetInt("SaveQuality"),
    Settings->GetInt("SaveSampling"),
    Settings->GetInt("SaveResolution"),
    Settings->GetString("OutputColorProfile").toAscii().data(),
    Settings->GetInt("OutputColorProfileIntent"));

  if (!FileWritten) {
    ptMessageBox::warning(MainWindow,"GraphicsMagick Error","No output file written.");
  } else {
    ReportProgress(QObject::tr("Writing output (exif)"));

    if (Settings->GetInt("IncludeExif") &&
        TheProcessor->m_ExifBufferLength) {
      WriteExif(Settings->GetString("OutputFileName"),
          TheProcessor->m_ExifBuffer,
          TheProcessor->m_ExifBufferLength);
    }
  }

  if (Settings->GetInt("JobMode") == 0) delete OutImage;

  if (JobMode == 0) {
    // This variable carries only JobMode, the "JobMode" settings
    // is also used for full image output.
    ReportProgress(QObject::tr("Writing output (settings)"));

    QFileInfo PathInfo(Settings->GetString("OutputFileName"));
    QString SettingsFileName = PathInfo.dir().path() + "/" + PathInfo.baseName() + ".pts";

    WriteSettingsFile(SettingsFileName);
  }

  ReportProgress(QObject::tr("Ready"));
}

void WritePipe(QString OutputName = "") {

  if (Settings->GetInt("HaveImage")==0) return;

  QStringList InputFileNameList = Settings->GetStringList("InputFileNameList");
  QFileInfo PathInfo(InputFileNameList[0]);
  QString SuggestedFileName = PathInfo.dir().path() + "/" + PathInfo.baseName();
  if (!Settings->GetInt("IsRAW")) SuggestedFileName += "-new";
  QString Pattern;

  switch(Settings->GetInt("SaveFormat")) {
    case ptSaveFormat_JPEG :
      SuggestedFileName += ".jpg";
      Pattern = QObject::tr("Jpg images (*.jpg *.jpeg);;All files (*.*)");
      break;
    case ptSaveFormat_PNG :
    case ptSaveFormat_PNG16 :
      SuggestedFileName += ".png";
      Pattern = QObject::tr("PNG images(*.png);;All files (*.*)");
      break;
    case ptSaveFormat_TIFF8 :
    case ptSaveFormat_TIFF16 :
      SuggestedFileName += ".tif";
      Pattern = QObject::tr("Tiff images (*.tif *.tiff);;All files (*.*)");
      break;
    default :
      SuggestedFileName += ".ppm";
      Pattern = QObject::tr("Ppm images (*.ppm);;All files (*.*)");
      break;
  }

  QString FileName = OutputName;
  if (FileName == "")
    FileName = QFileDialog::getSaveFileName(NULL,
                                            QObject::tr("Save File"),
                                            SuggestedFileName,
                                            Pattern);

  if (0 == FileName.size()) return; // Operation cancelled.

  Settings->SetValue("OutputFileName",FileName);

  // Write out (maybe after applying gamma).
  Update(ptProcessorPhase_WriteOut);
  ImageSaved = 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// WriteSettingsFile
// We take advantage the ini file possibilities of Qt.
//
////////////////////////////////////////////////////////////////////////////////

short WriteSettingsFile(const QString FileName, const short IsJobFile /* = 0 */) {

  QSettings JobSettings(FileName,QSettings::IniFormat);
  JobSettings.setValue("Magic","photivoSettingsFile");
  QStringList Keys = Settings->GetKeys();
  Keys.sort();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    if (!Settings->GetInJobFile(Key)) continue;
    if (IsJobFile == 0 && Key=="InputFileNameList") continue;
    if (IsJobFile == 0 && Key=="OutputDirectory") continue;
    JobSettings.setValue(Key,Settings->GetValue(Key));
  }
  // save the manual curves
  for (int i = 0; i < CurveKeys.size(); i++) {
    if (Settings->GetInt(CurveKeys.at(i))==ptCurveChoice_Manual) {
      JobSettings.setValue(CurveKeys.at(i) + "Counter",Curve[i]->m_NrAnchors);
      for (int j = 0; j < Curve[i]->m_NrAnchors; j++) {
        JobSettings.setValue(CurveKeys.at(i) + "X" + QString::number(j),Curve[i]->m_XAnchor[j]);
        JobSettings.setValue(CurveKeys.at(i) + "Y" + QString::number(j),Curve[i]->m_YAnchor[j]);
      }
      JobSettings.setValue(CurveKeys.at(i) + "Type",Curve[i]->m_IntType);
    }
  }
  JobSettings.sync();
  if (JobSettings.status() == QSettings::NoError) return 0;
  assert(JobSettings.status() == QSettings::NoError); // TODO
  return ptError_FileOpen;
}

////////////////////////////////////////////////////////////////////////////////
//
// ReadSettingsFile
// We take advantage the ini file possibilities of Qt.
//
// Setting files contain information about everything needed to process
// the image, they may need string corrections.
// Job files contain additional information about the files to process.
// Preset files contain just information about filters, no string
// correction will be needed.
//
////////////////////////////////////////////////////////////////////////////////

short ReadSettingsFile(const QString FileName, short& NextPhase) {
  QFileInfo Info(FileName);
  if (!Info.exists()) return ptError_FileOpen;

  // Temporary copy to leave the files untouched
  QTemporaryFile SettingsFile;
  SettingsFile.setAutoRemove(1);
  SettingsFile.setFileTemplate(QDir::tempPath()+"/XXXXXX.pts");
  QString TempName = SettingsFile.fileTemplate();
  if (QFile(TempName).exists()) QFile(TempName).remove();
  SettingsFile.open();
  QFile(FileName).copy(TempName);
  QSettings JobSettings(TempName,QSettings::IniFormat);
  if (!(JobSettings.value("Magic") == "photivoJobFile" ||
        JobSettings.value("Magic") == "photivoSettingsFile" ||
        JobSettings.value("Magic") == "dlRawJobFile" ||
        JobSettings.value("Magic") == "dlRawSettingsFile" ||
        JobSettings.value("Magic") == "photivoPresetFile")) {
    ptLogError(ptError_FileFormat,
               "'%s' has wrong format\n",
               FileName.toAscii().data());
    QFile::remove(TempName);
    SettingsFile.close();
    return ptError_FileFormat;
  }

  // Color space transformations precalc
  short NeedRecalcTransforms = 0;
  if (JobSettings.contains("CMQuality")) {
    if (JobSettings.value("CMQuality").toInt() != Settings->GetInt("CMQuality")) {
      NeedRecalcTransforms = 1;
    }
  } else if (JobSettings.contains("WorkColor")) {
    if (JobSettings.value("WorkColor").toInt() != Settings->GetInt("WorkColor")) {
      NeedRecalcTransforms = 1;
    }
  }

  // String corrections if directory got moved
  QStringList Directories;
    Directories << "TranslationsDirectory"
      << "CurvesDirectory"
      << "ChannelMixersDirectory"
      << "PresetDirectory"
      << "UIDirectory"
      << "CameraColorProfilesDirectory"
      << "PreviewColorProfilesDirectory"
      << "OutputColorProfilesDirectory"
      << "StandardAdobeProfilesDirectory"
      << "LensfunDatabaseDirectory"
      << "CameraColorProfile"
      << "PreviewColorProfile"
      << "OutputColorProfile"
      << "TextureOverlayFile";

  QStringList Locations;
  Locations << CurveFileNamesKeys
            << "ChannelMixerFileNames";

  QString OldString = QString();
  QString NewString = Settings->GetString("UserDirectory");
  int CorrectionNeeded = 0;

  if (JobSettings.contains("UserDirectory") &&
      JobSettings.value("Magic") != "photivoPresetFile") {
    // simplest case: file from a different user directory
    OldString = JobSettings.value("UserDirectory").toString();
    if (OldString != NewString) CorrectionNeeded = 1;
  } else if (JobSettings.contains("ShareDirectory") &&
      JobSettings.value("Magic") != "photivoPresetFile") {  // new style settings files
    // intermediate settings, just for compatibility
    OldString = JobSettings.value("ShareDirectory").toString();
    if (OldString != NewString) CorrectionNeeded = 1;
  } else if (JobSettings.value("Magic") != "photivoPresetFile") { // old style settings files
    // Adopt the old settings files, just for compatibility.
    // Asumes the LensfunsDatabase dir was the most stable directory
    if (!JobSettings.contains("LensfunDatabaseDirectory")) {
      ptMessageBox::warning(0,"Error","Old settings file, corrections not possible.\nNot applied!");
      QFile::remove(TempName);
      SettingsFile.close();
      return 0;
    } else {
      OldString = JobSettings.value("LensfunDatabaseDirectory").toString();
      OldString.chop(QString("LensfunDatabase").length());
      if (OldString != NewString) CorrectionNeeded = 1;
    }
  }

  if (CorrectionNeeded == 1) {
    int LeftPart = OldString.length();
    // Strings
    for (int i = 0; i < Directories.size(); i++) {
      if (JobSettings.value(Directories.at(i)).toString().left(LeftPart)==OldString) {
        QString TmpStr = JobSettings.value(Directories.at(i)).toString();
        TmpStr.remove(OldString);
        TmpStr.prepend(NewString);
        JobSettings.setValue(Directories.at(i),TmpStr);
      }
    }
    // Stringlists
    for (int i = 0; i < Locations.size(); i++) {
      QStringList TempList = JobSettings.value(Locations.at(i)).toStringList();
      for (int j = 0; j < TempList.size(); j++) {
        if (TempList.at(j).left(LeftPart)==OldString) {
          QString TmpStr = TempList.at(j);
          TmpStr.remove(OldString);
          TmpStr.prepend(NewString);
          TempList.replace(j,TmpStr);
        }
      }
      JobSettings.setValue(Locations.at(i),TempList);
    }
  }

  // All Settings were already read (with 0 remembering)
  // so we can safely use that to have them now overwritten
  // with the jobfile settings.
  QStringList Keys = Settings->GetKeys();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    if (!Settings->GetInJobFile(Key)) continue;
    if (!JobSettings.contains(Key)) continue;
    if (Key=="InputFileNameList" && Settings->GetInt("JobMode") == 0) continue;
    if (Key=="TranslationsDirectory") continue;
    if (Key=="CurvesDirectory") continue;
    if (Key=="ChannelMixersDirectory") continue;
    if (Key=="CameraColorProfilesDirectory") continue;
    if (Key=="PreviewColorProfilesDirectory") continue;
    if (Key=="OutputColorProfilesDirectory") continue;
    if (Key=="StandardAdobeProfilesDirectory") continue;
    if (Key=="LensfunDatabaseDirectory") continue;
    if (Key=="PreviewColorProfile") continue;
    if (Key=="OutputColorProfile") continue;
    if (Key=="OutputDirectory" && Settings->GetInt("JobMode") == 0) continue;
    if (Key=="PresetDirectory") continue;
    if (Key=="ShareDirectory") continue;
    if (Key=="UserDirectory") continue;
    if (Key=="HiddenTools") continue; // see below! BlockedTools are just set.
    QVariant Tmp = JobSettings.value(Key);
    // Correction needed as the values coming from the ini file are
    // often interpreted as strings even if they could be int or so.
    const QVariant::Type TargetType = (Settings->GetValue(Key)).type();
    if (Tmp.type() != TargetType) {
      if (TargetType == QVariant::Int ||
        TargetType == QVariant::UInt) {
        Tmp = Tmp.toInt();
      } else if (TargetType == QVariant::Double ||
                 (QMetaType::Type) TargetType == QMetaType::Float) {
        Tmp = Tmp.toDouble();
      } else if (TargetType == QVariant::StringList) {
        Tmp = Tmp.toStringList();
      } else {
        ptLogError(ptError_Argument,"Unexpected type %d",TargetType);
        assert(0);
      }
    }
    Settings->SetValue(Key,Tmp);
  }

  // Crop from settings file takes precedence. If its AR does not match the fixed
  // AR currently set in the UI, disable the fixed AR checkbox.
  if ((Settings->GetInt("Crop") == 1) && (Settings->GetInt("FixedAspectRatio") == 1)) {
    double CropARFromSettings = (double)Settings->GetInt("CropW") / Settings->GetInt("CropH");
    double CropARFromUI = (double)Settings->GetInt("AspectRatioW") / Settings->GetInt("AspectRatioH");
    if (qAbs(CropARFromSettings - CropARFromUI) > 0.01) {
      Settings->SetValue("FixedAspectRatio", 0);
    }
  }

  // Hidden tools:
  // current hidden tools, stay hidden, unless they are needed for settings.
  QStringList CurrentHiddenTools = Settings->GetStringList("HiddenTools");
  QStringList ProposedHiddenTools = JobSettings.value("HiddenTools").toStringList();
  CurrentHiddenTools.removeDuplicates();
  ProposedHiddenTools.removeDuplicates();
  Settings->SetValue("HiddenTools",ProposedHiddenTools);
  //~ QApplication::sendEvent(MainWindow, &QKeyEvent(QEvent::KeyPress,Qt::Key_A,Qt::NoModifier));
  QStringList BlockedTools = Settings->GetStringList("BlockedTools");
  for (int i = 0; i < CurrentHiddenTools.size(); i++) {
    if (ProposedHiddenTools.contains(CurrentHiddenTools.at(i))) {
      ProposedHiddenTools.removeOne(CurrentHiddenTools.at(i));
      continue;
    }
    if (Settings->ToolIsActive(CurrentHiddenTools.at(i))) {
      CurrentHiddenTools.removeOne(CurrentHiddenTools.at(i));
    }
  }
  Settings->SetValue("HiddenTools",CurrentHiddenTools);
  // proposed hidden tools, get not hidden, but may disable active tools
  for (int i = 0; i < ProposedHiddenTools.size(); i++) {
    if (Settings->ToolIsActive(ProposedHiddenTools.at(i)))
      BlockedTools.append(ProposedHiddenTools.at(i));
  }
  BlockedTools.removeDuplicates();
  Settings->SetValue("BlockedTools",BlockedTools);
  if (Settings->GetInt("JobMode") == 0) MainWindow->UpdateToolBoxes();

  // next processor stage
  if (JobSettings.contains("NextPhase"))
    NextPhase = JobSettings.value("NextPhase").toInt();
  // restore the manual curves
  for (int i = 0; i < CurveKeys.size(); i++) {
    if (!JobSettings.contains(CurveKeys.at(i))) continue;
    if (JobSettings.value(CurveKeys.at(i)).toInt() == ptCurveChoice_Manual) {
      Curve[i]->m_NrAnchors = JobSettings.value(CurveKeys.at(i) + "Counter").toInt();
      for (int j = 0; j < Curve[i]->m_NrAnchors; j++) {
        Curve[i]->m_XAnchor[j] = JobSettings.value(CurveKeys.at(i) + "X" + QString::number(j)).toDouble();
        Curve[i]->m_YAnchor[j] = JobSettings.value(CurveKeys.at(i) + "Y" + QString::number(j)).toDouble();
      }
      Curve[i]->m_IntType = JobSettings.value(CurveKeys.at(i) + "Type").toInt();
      Curve[i]->SetCurveFromAnchors();
      if (Settings->GetInt("JobMode") == 0) CurveWindow[i]->UpdateView(Curve[i]);
    }
  }

  // Update display of comboboxes
  // This also clears non-existing files
  QStringList ListCombos;
  ListCombos << CurveKeys << "ChannelMixer";
  for (int i = 0; i < ListCombos.size(); i++ )
    UpdateComboboxes(ListCombos.at(i));

  // Load curves and channelmixer
  QStringList CurveFileNames;
  for (int Channel = 0; Channel < CurveKeys.size(); Channel++) {
    CurveFileNames = Settings->GetStringList(CurveFileNamesKeys[Channel]);
    // If we have a curve, go for reading it.
    if (Settings->GetInt(CurveKeys[Channel]) >= ptCurveChoice_File) {
      if (!Curve[Channel]) Curve[Channel] = new(ptCurve);
      if (Curve[Channel]->
          ReadCurve(CurveFileNames[Settings->GetInt(CurveKeys[Channel])-ptCurveChoice_File].
                    toAscii().data())){
        assert(0);
      }
    }

    if (Settings->GetInt(CurveKeys[Channel]) == ptCurveChoice_None) {
      Curve[Channel]->SetNullCurve(Channel);
    }

    // Update the View.
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    if (Settings->GetInt("JobMode") == 0) CurveWindow[Channel]->UpdateView(Curve[Channel]);
  }

  if (Settings->GetInt("ChannelMixer")>1) {
    if (ChannelMixer->ReadChannelMixer(
         (Settings->GetStringList("ChannelMixerFileNames"))
           [Settings->GetInt("ChannelMixer")-ptChannelMixerChoice_File].
             toAscii().data())) {
      assert(0);
    }
  }

  // ChannelMixer
  ChannelMixer->m_Mixer[0][0] = Settings->GetDouble("ChannelMixerR2R");
  ChannelMixer->m_Mixer[0][1] = Settings->GetDouble("ChannelMixerG2R");
  ChannelMixer->m_Mixer[0][2] = Settings->GetDouble("ChannelMixerB2R");
  ChannelMixer->m_Mixer[1][0] = Settings->GetDouble("ChannelMixerR2G");
  ChannelMixer->m_Mixer[1][1] = Settings->GetDouble("ChannelMixerG2G");
  ChannelMixer->m_Mixer[1][2] = Settings->GetDouble("ChannelMixerB2G");
  ChannelMixer->m_Mixer[2][0] = Settings->GetDouble("ChannelMixerR2B");
  ChannelMixer->m_Mixer[2][1] = Settings->GetDouble("ChannelMixerG2B");
  ChannelMixer->m_Mixer[2][2] = Settings->GetDouble("ChannelMixerB2B");

  // Color space transformations precalc
  if (NeedRecalcTransforms == 1) PreCalcTransforms();

  // clean up non-existing files in settings file
  // currently, we completely preserve the settings file
  /*
  for (int i = 0; i < ListCombos.size(); i++) {
    JobSettings.setValue(ListCombos.at(i), Settings->GetInt(ListCombos.at(i)));
    JobSettings.setValue(Locations.at(i), Settings->GetStringList(Locations.at(i)));
  }
  JobSettings.setValue("CameraColorProfile", Settings->GetString("CameraColorProfile"));*/

  JobSettings.sync();
  if (JobSettings.status() == QSettings::NoError) {
    QFile::remove(TempName);
    SettingsFile.close();
    return 0;
  }
  assert(JobSettings.status() == QSettings::NoError); // TODO
  QFile::remove(TempName);
  SettingsFile.close();
  return ptError_FileFormat;
}

////////////////////////////////////////////////////////////////////////////////
//
// Utility : Update GuiSettings->m_?Multiplier according
// to the ColorTemperature/GreenIntensity in the GuiSettings.
//
////////////////////////////////////////////////////////////////////////////////

void CalculateMultipliersFromTemperature() {

  // m_D65Multipliers are supposed to be D65
  // (setting Pre to the ratio of the D65 delivers
  // rgbWB = (x,x,x) and 6500 temperature).

  double RefRGB[3];
  TemperatureToRGB(Settings->GetInt("ColorTemperature"),RefRGB);
  RefRGB[1] /= Settings->GetDouble("GreenIntensity");
  if (TheDcRaw->m_RawColorPhotivo) {
   Settings->SetValue("RMultiplier",
                      VALUE(TheDcRaw->m_D65Multipliers[0])/RefRGB[0]);
   Settings->SetValue("GMultiplier",
                      VALUE(TheDcRaw->m_D65Multipliers[1])/RefRGB[1]);
   Settings->SetValue("BMultiplier",
                      VALUE(TheDcRaw->m_D65Multipliers[2])/RefRGB[2]);
  } else {
    // If not raw color, calculate RefRGB in sRGB back to the camera RGB
    for (short c=0; c<TheDcRaw->m_Colors; c++) {
      double InverseMultiplier = 0;
      for (short cc=0; cc<3; cc++) {
        InverseMultiplier += 1/VALUE(TheDcRaw->m_D65Multipliers[c])
                              * TheDcRaw->m_MatrixSRGBToCamRGB[c][cc]
                              * RefRGB[cc];
      }
      switch (c) {
        case 0 :
          Settings->SetValue("RMultiplier",1/InverseMultiplier);
          break;
        case 1 :
          Settings->SetValue("GMultiplier",1/InverseMultiplier);
          break;
        case 2 :
          Settings->SetValue("BMultiplier",1/InverseMultiplier);
          break;
        default :
          assert(0);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Utility function : Normalize the Multipliers to largest 1.0
// If Max is set, normalize to lowest 1.0
//
////////////////////////////////////////////////////////////////////////////////

void NormalizeMultipliers(const short Max = 0) {
  if (Max == 0) {
    double Maximum = Settings->GetDouble("RMultiplier");
    if (Settings->GetDouble("GMultiplier") > Maximum)
      Maximum = Settings->GetDouble("GMultiplier");
    if (Settings->GetDouble("BMultiplier") > Maximum)
      Maximum = Settings->GetDouble("BMultiplier");
    assert (Maximum > 0.0);
    Settings->SetValue("RMultiplier",Settings->GetDouble("RMultiplier")/Maximum);
    Settings->SetValue("GMultiplier",Settings->GetDouble("GMultiplier")/Maximum);
    Settings->SetValue("BMultiplier",Settings->GetDouble("BMultiplier")/Maximum);
  } else {
    double Minimum = Settings->GetDouble("RMultiplier");
    if (Settings->GetDouble("GMultiplier") < Minimum)
      Minimum = Settings->GetDouble("GMultiplier");
    if (Settings->GetDouble("BMultiplier") < Minimum)
      Minimum = Settings->GetDouble("BMultiplier");
    assert (Minimum > 0.0);
    Settings->SetValue("RMultiplier",Settings->GetDouble("RMultiplier")/Minimum);
    Settings->SetValue("GMultiplier",Settings->GetDouble("GMultiplier")/Minimum);
    Settings->SetValue("BMultiplier",Settings->GetDouble("BMultiplier")/Minimum);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// For convenience...
//
////////////////////////////////////////////////////////////////////////////////

void UpdateSettings() {
  MainWindow->UpdateSettings();
}

////////////////////////////////////////////////////////////////////////////////
//
// Callback pertaining to the Tabs structure (switching page)
//
////////////////////////////////////////////////////////////////////////////////

void CB_Tabs(const short) {
  // If we are previewing according to Tab, we now have to update.
  if (Settings->GetInt("PreviewMode") == ptPreviewMode_Tab) {
    Update(ptProcessorPhase_NULL);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Menu structure.
// (inclusive 'X' pressing).
//
////////////////////////////////////////////////////////////////////////////////

void CB_MenuFileOpen(const short HaveFile) {
  QStringList OldInputFileNameList = Settings->GetStringList("InputFileNameList");
  QString InputFileName;
  if (HaveFile) {
    InputFileName = ImageFileToOpen;
  } else {
    InputFileName =
      QFileDialog::getOpenFileName(NULL,
                                   QObject::tr("Open Raw"),
                                   Settings->GetString("RawsDirectory"),
                                   RawPattern);
  }

  if (InputFileName == "") {
      return;
  } else {
      if (!QFile::exists(InputFileName)) {
          ptMessageBox::warning(NULL,
                  QObject::tr("File not found"),
                  QObject::tr("Input file does not exist.") + "\n\n" + InputFileName);
          return;
      }
  }

  QFileInfo PathInfo(InputFileName);
  Settings->SetValue("RawsDirectory",PathInfo.absolutePath());
  QStringList InputFileNameList;
  InputFileNameList << PathInfo.absoluteFilePath();

  Settings->SetValue("InputFileNameList",InputFileNameList);

  // Feedback to the user
  ReportProgress(QObject::tr("Reading file"));

  // Test if we can handle the file
  DcRaw* TestDcRaw = new(DcRaw);
  Settings->ToDcRaw(TestDcRaw);
  int OpenError = 0;
  uint16_t InputWidth = 0;
  uint16_t InputHeight = 0;
  if (TestDcRaw->Identify()){ // Bitmap
    try {
      Magick::Image image;

      image.ping(InputFileNameList[0].toAscii().data());

      InputWidth = image.columns();
      InputHeight = image.rows();
    } catch (Magick::Exception &Error) {
      OpenError = 1;
    }
    if (OpenError == 0) {
      Settings->SetValue("IsRAW",0);
      Settings->SetValue("ExposureNormalization",0.0);
    }
  } else {
    Settings->SetValue("IsRAW",1);
  }
  if (OpenError == 1) {
    // We don't have a RAW or a bitmap!
    QString ErrorMessage = QObject::tr("Cannot decode")
                         + " '"
                         + InputFileNameList[0]
                         + "'" ;
    ptMessageBox::warning(MainWindow,"Decode error",ErrorMessage);
    Settings->SetValue("InputFileNameList",OldInputFileNameList);
    delete TestDcRaw;
    return;
  }
  if (Settings->GetInt("HaveImage")==1) {
    if ( Settings->GetInt("SaveConfirmation") == 0 ) {
      if (ImageCleanUp == 1) {
        // clean up the input file if we got just a temp file
        QFile::remove(OldInputFileNameList.at(0));
      }
    } else {
      ptMessageBox msgBox;
      msgBox.setIcon(QMessageBox::Question);
      msgBox.setWindowTitle("Open new image");
      msgBox.setText("Do you want to save the current image?");
      msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Save);
      int ret = msgBox.exec();
      switch (ret) {
        case QMessageBox::Save:
          // Save was clicked
          Settings->SetValue("InputFileNameList",OldInputFileNameList);
          CB_WritePipeButton();
          Settings->SetValue("InputFileNameList",InputFileNameList);
          if (ImageCleanUp == 1) {
            // clean up the input file if we got just a temp file
            QFile::remove(OldInputFileNameList.at(0));
          }
          return;
        case QMessageBox::Discard:
          // Don't Save was clicked
          if (ImageCleanUp == 1) {
            // clean up the input file if we got just a temp file
            QFile::remove(OldInputFileNameList.at(0));
          }
          break;
        case QMessageBox::Cancel:
           // Cancel was clicked
          Settings->SetValue("InputFileNameList",OldInputFileNameList);
          delete TestDcRaw;
          return;
          break;
        default:
           // should never be reached
           break;
      }
    }
    // Catch 1:1 pipe size when opening
    if (Settings->GetInt("PipeSize") == 0)
      Settings->SetValue("PipeSize",1);
  }

  // TODO mike: need to delete the processor here?
  delete TheDcRaw;
  delete TheProcessor;
  // Load user settings
  if (Settings->GetInt("StartupSettings")==1 &&
      Settings->GetInt("StartupSettingsReset")==1 &&
      Settings->GetInt("HaveImage")==1) {
    Settings->SetValue("HaveImage",0);
    CB_OpenSettingsFile(Settings->GetString("StartupSettingsFile"));
    // clean up
    QStringList Temp;
    Temp << "CropX" << "CropY" << "CropW" << "CropH";
    Temp << "RotateW" << "RotateH";
    for (int i = 0; i < Temp.size(); i++) Settings->SetValue(Temp.at(i),0);
  }
  // clean up possible detail view cache
  if (Settings->GetInt("DetailViewActive")==1) {
    Settings->SetValue("DetailViewActive",0);
    Settings->SetValue("DetailViewCropX", 0);
    Settings->SetValue("DetailViewCropY", 0);
    Settings->SetValue("DetailViewCropW", 0);
    Settings->SetValue("DetailViewCropH", 0);
    Settings->ToDcRaw(TestDcRaw);
  }

  TheDcRaw = TestDcRaw;
  if (Settings->GetInt("IsRAW")==0) {
    Settings->SetValue("ImageW",InputWidth);
    Settings->SetValue("ImageH",InputHeight);
  } else {
    Settings->SetValue("ImageW",TheDcRaw->m_Width);
    Settings->SetValue("ImageH",TheDcRaw->m_Height);
  }

  if (Settings->GetInt("StartupSwitchAR")) {
    // portrait image
    if ((Settings->GetInt("IsRAW")==0 &&
         Settings->GetInt("ImageW") < Settings->GetInt("ImageH")) ||
        TheDcRaw->m_Flip & 4) {
      if (Settings->GetInt("AspectRatioW") > Settings->GetInt("AspectRatioH"))
        CB_CropOrientationButton();
    } else { // landscape
      if (Settings->GetInt("AspectRatioW") < Settings->GetInt("AspectRatioH"))
        CB_CropOrientationButton();
    }
  }

  // reflect RAW or bitmap in GUI
  MainWindow->UpdateToolBoxes();

  TheProcessor = new ptProcessor(ReportProgress);
  TheProcessor->m_DcRaw = TheDcRaw;

  Settings->SetValue("HaveImage", 1);
  if(Settings->GetInt("CameraColor")==ptCameraColor_Adobe_Matrix)
    Settings->SetValue("CameraColor",ptCameraColor_Adobe_Profile);
  short OldRunMode = Settings->GetInt("RunMode");
  Settings->SetValue("RunMode",0);


  if (Settings->GetInt("AutomaticPipeSize") && Settings->ToolIsActive("TabResize")) {
    CalculatePipeSize(true);
  }

  Update(ptProcessorPhase_Raw,ptProcessorPhase_Load,0);

  MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  Settings->SetValue("PerspectiveFocalLength",Settings->GetDouble("FocalLengthIn35mmFilm"));
  Settings->SetValue("DefishFocalLength",Settings->GetDouble("FocalLengthIn35mmFilm"));
  Settings->SetValue("LfunFocal", Settings->GetDouble("FocalLengthIn35mmFilm"));
  double TmpAprt = Settings->GetDouble("ApertureFromExif");
  Settings->SetValue("LfunAperture", (TmpAprt==0.0) ? 8.0 : TmpAprt);
  MainWindow->UpdateFilenameInfo(Settings->GetStringList("InputFileNameList"));

  #ifdef Q_OS_WIN32
    MainWindow->setWindowTitle(QString((Settings->GetStringList("InputFileNameList"))[0]).replace(QString("/"), QString("\\")) + " - Photivo");
  #else
    MainWindow->setWindowTitle((Settings->GetStringList("InputFileNameList"))[0]+ " - Photivo");
  #endif
  Settings->SetValue("RunMode",OldRunMode);
}

void CB_MenuFileSaveOutput(QString OutputName = "") {

  if (Settings->GetInt("HaveImage")==0) return;

  QStringList InputFileNameList = Settings->GetStringList("InputFileNameList");
  QFileInfo PathInfo(InputFileNameList[0]);
  QString SuggestedFileName = PathInfo.dir().path() + "/" + PathInfo.baseName();
  if (!Settings->GetInt("IsRAW")) SuggestedFileName += "-new";
  QString Pattern;

  switch(Settings->GetInt("SaveFormat")) {
    case ptSaveFormat_JPEG :
      SuggestedFileName += ".jpg";
      Pattern = QObject::tr("Jpg images (*.jpg *.jpeg);;All files (*.*)");
      break;
    case ptSaveFormat_PNG :
    case ptSaveFormat_PNG16 :
      SuggestedFileName += ".png";
      Pattern = QObject::tr("PNG images(*.png);;All files (*.*)");
      break;
    case ptSaveFormat_TIFF8 :
    case ptSaveFormat_TIFF16 :
      SuggestedFileName += ".tif";
      Pattern = QObject::tr("Tiff images (*.tif *.tiff);;All files (*.*)");
      break;
    default :
      SuggestedFileName += ".ppm";
      Pattern = QObject::tr("Ppm images (*.ppm);;All files (*.*)");
      break;
  }

  QString FileName = OutputName;
  if (FileName == "")
    FileName = QFileDialog::getSaveFileName(NULL,
                                            QObject::tr("Save File"),
                                            SuggestedFileName,
                                            Pattern);

  if (0 == FileName.size()) return; // Operation cancelled.

  Settings->SetValue("OutputFileName",FileName);

  short OldRunMode = Settings->GetInt("RunMode");
  Settings->SetValue("RunMode",0);

  // Processing the job.
  delete TheDcRaw;
  delete TheProcessor;
  TheDcRaw = new(DcRaw);
  TheProcessor = new ptProcessor(ReportProgress);
  Settings->SetValue("JobMode",1); // Disable caching to save memory
  TheProcessor->m_DcRaw = TheDcRaw;
  Settings->ToDcRaw(TheDcRaw);
  // Run the graphical pipe in full format mode to recreate the image.
  Settings->SetValue("FullOutput",1);
  TheProcessor->Run(ptProcessorPhase_Raw,ptProcessorPhase_Load,1,1);
  Settings->SetValue("FullOutput",0);

  // Write out (maybe after applying gamma).
  Update(ptProcessorPhase_WriteOut);

  delete TheDcRaw;
  delete TheProcessor;
  TheDcRaw = new(DcRaw);
  TheProcessor = new ptProcessor(ReportProgress);
  Settings->SetValue("JobMode",0);
  TheProcessor->m_DcRaw = TheDcRaw;
  Settings->ToDcRaw(TheDcRaw);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Load,0);
  MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  Settings->SetValue("RunMode",OldRunMode);
  ImageSaved = 1;
}

void CB_MenuFileWriteJob(const short) {
  if (Settings->GetInt("HaveImage")==0) return;
  // First a dialog to obtain the input for the job.

  QStringList InputFileNames = QFileDialog::getOpenFileNames(
                              NULL,
                              QObject::tr("Select input file(s)"),
                              Settings->GetString("RawsDirectory"),
                              RawPattern);

  // Operation cancelled.
  if (InputFileNames.isEmpty()) return;

  Settings->SetValue("InputFileNameList",InputFileNames);

  // Then a dialog to obtain the output directory

  QString Directory =
    QFileDialog::getExistingDirectory(NULL,
                                      QObject::tr("Select output directory"),
                                      Settings->GetString("OutputDirectory"));

  // Operation cancelled.
  if (Directory.size() == 0) return;

  Settings->SetValue("OutputDirectory",Directory);

  // And finally a dialog to obtain the output job file.

  QFileInfo PathInfo(InputFileNames[0]);
  QString SuggestedJobFileName = PathInfo.dir().path() + "/" + PathInfo.baseName() + ".ptj";

  QString JobFileName =
    QFileDialog::getSaveFileName(NULL,
                                 QObject::tr("Select job file"),
                                 SuggestedJobFileName,
                                 JobFilePattern);


  // Operation cancelled.
  if (JobFileName.size() == 0) return;

  WriteSettingsFile(JobFileName, 1);
}

void CB_MenuFileWriteSettings() {
  if (Settings->GetInt("HaveImage")==0) return;
  QStringList InputFileNameList = Settings->GetStringList("InputFileNameList");
  QFileInfo PathInfo(InputFileNameList[0]);
  QString SuggestedFileName = PathInfo.dir().path() + "/" + PathInfo.baseName() + ".pts";

  QString FileName;

  FileName = QFileDialog::getSaveFileName(NULL,
                                          QObject::tr("Settings File"),
                                          SuggestedFileName,
                                          SettingsFilePattern);

  if (FileName.size() == 0) return;
  WriteSettingsFile(FileName);
}


void CB_MenuFileExit(const short) {
  if (Settings->GetInt("HaveImage")==1 && ImageSaved == 0 && Settings->GetInt("SaveConfirmation")==1) {
    ptMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("Close Photivo");
    msgBox.setText("Do you want to save the current image?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    msgBox.move((MainWindow->pos()).x()+(MainWindow->size()).width()/2-msgBox.size().width()/4,
                (MainWindow->pos()).y()+(MainWindow->size()).height()/2-msgBox.size().height()/4);
    int ret = msgBox.exec();
    switch (ret) {
      case QMessageBox::Save:
         // Save was clicked
         CB_WritePipeButton();
         break;
      case QMessageBox::Discard:
         // Don't Save was clicked
         break;
      case QMessageBox::Cancel:
         // Cancel was clicked

        return;
        break;
      default:
         // should never be reached
         break;
    }
  }
  // clean up the input file if we got just a temp file
  if (Settings->GetInt("HaveImage")==1 && ImageCleanUp == 1) {
    QString OldInputFileName = Settings->GetStringList("InputFileNameList")[0];
    QFile::remove(OldInputFileName);
  }

  // Delete backup settingsfile
  if (Settings->GetInt("WriteBackupSettings"))
    QFile::remove(Settings->GetString("UserDirectory")+"/backup.pts");

  // Disable manual curves when closing
  for (int i = 0; i < CurveKeys.size(); i++) {
    if (Settings->GetInt(CurveKeys.at(i))==ptCurveChoice_Manual)
      Settings->SetValue(CurveKeys.at(i),ptCurveChoice_None);
  }

  printf("Saving settings ...\n");

  // Store the position of the splitter and main window
  Settings->m_IniSettings->
    setValue("MainSplitter",MainWindow->MainSplitter->saveState());
  Settings->m_IniSettings->
    setValue("ControlSplitter",MainWindow->ControlSplitter->saveState());
  Settings->m_IniSettings->setValue("MainWindowPos",MainWindow->pos());
  Settings->m_IniSettings->setValue("MainWindowSize",MainWindow->size());
  Settings->m_IniSettings->setValue("IsMaximized", MainWindow->windowState() == Qt::WindowMaximized);
  // Store the version of the settings and files
  Settings->m_IniSettings->setValue("SettingsVersion",PhotivoSettingsVersion);

  // Explicitly. The destructor of it cares for persistent settings.
  delete Settings;



  ALLOCATED(10000000);
  printf("Exiting Photivo.\n");
  QCoreApplication::exit(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the gimp
//
////////////////////////////////////////////////////////////////////////////////

void CB_ExportToGimpCheck(const QVariant Check) {
  Settings->SetValue("ExportToGimp",Check);
}

void GimpExport(const short UsePipe) {

  if (Settings->GetInt("HaveImage")==0) return;

  ReportProgress(QObject::tr("Writing tmp image for gimp"));

  short OldRunMode = Settings->GetInt("RunMode");

  ptImage* ImageForGimp = new ptImage;

  if (UsePipe == 1)
    ImageForGimp->Set(TheProcessor->m_Image_AfterEyeCandy);
  else {
    Settings->SetValue("RunMode",0);

    // Processing the job.
    delete TheDcRaw;
    delete TheProcessor;
    TheDcRaw = new(DcRaw);
    TheProcessor = new ptProcessor(ReportProgress);
    Settings->SetValue("JobMode",1); // Disable caching to save memory
    TheProcessor->m_DcRaw = TheDcRaw;
    Settings->ToDcRaw(TheDcRaw);
    // Run the graphical pipe in full format mode to recreate the image.
    Settings->SetValue("FullOutput",1);
    TheProcessor->Run(ptProcessorPhase_Raw,ptProcessorPhase_Load,1,1);
    Settings->SetValue("FullOutput",0);

    ImageForGimp = TheProcessor->m_Image_AfterEyeCandy; // no cache
  }

  BeforeGamma(ImageForGimp);

  cmsHPROFILE OutputColorProfile = NULL;

  ReportProgress(QObject::tr("Converting to output space"));

  // Prepare and open an output profile.
  OutputColorProfile = cmsOpenProfileFromFile(
    Settings->GetString("OutputColorProfile").toAscii().data(),
    "r");
  if (!OutputColorProfile) {
    ptLogError(ptError_FileOpen,
   Settings->GetString("OutputColorProfile").toAscii().data());
    assert(OutputColorProfile);
  }

  // Color space conversion
  ptImage* ReturnValue = ImageForGimp->lcmsRGBToRGB(
    OutputColorProfile,
    Settings->GetInt("OutputColorProfileIntent"),
    Settings->GetInt("CMQuality"));
  if (!ReturnValue) {
    ptLogError(ptError_lcms,"lcmsRGBToRGB(OutputColorProfile)");
    assert(ReturnValue);
  }

  AfterAll(ImageForGimp);

  if (Settings->ToolIsActive("TabOutWiener"))
    EndSharpen(ImageForGimp, OutputColorProfile, Settings->GetInt("OutputColorProfileIntent"));
  // Close the output profile.
  cmsCloseProfile(OutputColorProfile);

  QTemporaryFile ImageFile;
  ImageFile.setFileTemplate(QDir::tempPath()+"/XXXXXX.ppm");
  assert (ImageFile.open());
  QString ImageFileName = ImageFile.fileName();
  ImageFile.setAutoRemove(false);
  ImageFile.close();
  printf("(%s,%d) '%s'\n",
         __FILE__,__LINE__,ImageFileName.toAscii().data());
  ImageForGimp->WriteAsPpm(ImageFileName.toAscii().data(),16);

  ReportProgress(QObject::tr("Writing tmp exif for gimp"));

  QTemporaryFile ExifFile;
  assert (ExifFile.open());
  QString ExifFileName = ExifFile.fileName();
  ExifFile.setAutoRemove(false);
  printf("(%s,%d) '%s'\n",
         __FILE__,__LINE__,ExifFileName.toAscii().data());
  QDataStream ExifOut(&ExifFile);
  ExifOut.writeRawData((char *) TheProcessor->m_ExifBuffer,
                       TheProcessor->m_ExifBufferLength);
  ExifFile.close();

  ReportProgress(QObject::tr("Writing tmp icc for gimp"));

  QTemporaryFile ICCFile;
  assert (ICCFile.open());
  QString ICCFileName = ICCFile.fileName();
  ICCFile.setAutoRemove(false);
  printf("(%s,%d) '%s'\n",
         __FILE__,__LINE__,ICCFileName.toAscii().data());
  QDataStream ICCOut(&ICCFile);
  FILE* pFile = fopen ( Settings->GetString("OutputColorProfile").toAscii().data(), "rb" );
  if (pFile==NULL) {
    ptLogError(ptError_FileOpen,Settings->GetString("OutputColorProfile").toAscii().data());
    exit(EXIT_FAILURE);
  }
  fseek (pFile , 0 , SEEK_END);
  long lSize = ftell (pFile);
  rewind (pFile);

  char* pchBuffer = (char*) CALLOC2(lSize,sizeof(uint8_t));
  ptMemoryError(pchBuffer,__FILE__,__LINE__);

  size_t RV = fread (pchBuffer, 1, lSize, pFile);
  if (RV != (size_t) lSize) {
    ptLogError(ptError_FileOpen,Settings->GetString("OutputColorProfile").toAscii().data());
    exit(EXIT_FAILURE);
  }
  ICCOut.writeRawData(pchBuffer, lSize);

  FCLOSE (pFile);
  FREE (pchBuffer);
  ICCFile.close();

  ReportProgress(QObject::tr("Writing gimp interface file"));

  QTemporaryFile GimpFile;
  GimpFile.setFileTemplate(QDir::tempPath()+"/XXXXXX.ptg");
  assert (GimpFile.open());
  QString GimpFileName = GimpFile.fileName();
  GimpFile.setAutoRemove(false);
  printf("(%s,%d) '%s'\n",
         __FILE__,__LINE__,GimpFileName.toAscii().data());
  QTextStream Out(&GimpFile);
  Out << ImageFileName << "\n";
  Out << ExifFileName << "\n";
  Out << ICCFileName << "\n";
  GimpFile.close();

  QString GimpExeCommand = Settings->GetString("GimpExecCommand");
  QStringList GimpArguments;
  GimpArguments << GimpFileName;
  QProcess* GimpProcess = new QProcess();
  GimpProcess->startDetached(GimpExeCommand,GimpArguments);

  // clean up
  if (UsePipe == 1) {
    delete ImageForGimp;
  } else {
    delete TheDcRaw;
    delete TheProcessor;
    TheDcRaw = new(DcRaw);
    TheProcessor = new ptProcessor(ReportProgress);
    Settings->SetValue("JobMode",0);
    TheProcessor->m_DcRaw = TheDcRaw;
    Settings->ToDcRaw(TheDcRaw);
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Load,0);
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
    Settings->SetValue("RunMode",OldRunMode);
  }
  ReportProgress(QObject::tr("Ready"));
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Zoom function
//
////////////////////////////////////////////////////////////////////////////////

void CB_ZoomInput(const QVariant Value) {
  ViewWindow->ZoomTo(Value.toFloat() / 100.0);
  // TODO SR: updatesettings doesnt seem to be necessary
//    MainWindow->UpdateSettings(); // To reflect maybe new zoom
}

void CB_ZoomFitButton() {
  ViewWindow->ZoomToFit();
  //MainWindow->UpdateSettings(); // To reflect maybe new zoom
}

void CB_ZoomFullButton() {
  CB_ZoomInput(100);
}


void CB_FullScreenButton(const int State) {
  if (State == 1) {
    MainWindow->showFullScreen();
    Settings->SetValue("FullscreenActive", 1);
  } else {
    MainWindow->showNormal();
    Settings->SetValue("FullscreenActive", 0);
  }
  MainWindow->FullScreenButton->setChecked(State);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Generic Tab
//
////////////////////////////////////////////////////////////////////////////////

void CB_CameraColorChoice(const QVariant Choice) {
  int PreviousChoice = Settings->GetInt("CameraColor");
  Settings->SetValue("CameraColor",Choice);
  if (Choice.toInt() == ptCameraColor_Profile) {
    if (!Settings->GetString("CameraColorProfile").size()) {
      ptMessageBox::warning(MainWindow,
                           QObject::tr("Please load a profile first"),
                           QObject::tr("Please load a profile first"));
      Settings->SetValue("CameraColor",PreviousChoice);
    }
  }

  if (Settings->GetInt("CameraColor") == ptCameraColor_Embedded) {
    // TODO
    ptMessageBox::warning(MainWindow,
                        QObject::tr("Not yet implemented"),
                        QObject::tr("Not yet implemented. Reverting to Adobe."));
    Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
  }

  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

void CB_CameraColorProfileButton() {
  QString ProfileFileName = QFileDialog::getOpenFileName(
                 NULL,
                 QObject::tr("Open Profile"),
                 Settings->GetString("CameraColorProfilesDirectory"),
                 ProfilePattern);

  if (0 == ProfileFileName.size() ) {
    // Canceled just return
    return;
  } else {
    QFileInfo PathInfo(ProfileFileName);
    Settings->SetValue("CameraColorProfilesDirectory",PathInfo.absolutePath());
    Settings->SetValue("CameraColorProfile",PathInfo.absoluteFilePath());
    Settings->SetValue("CameraColor",ptCameraColor_Profile);
  }

  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

void CB_CameraColorProfileIntentChoice(const QVariant Choice) {
  Settings->SetValue("CameraColorProfileIntent",Choice);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

void CB_CameraColorGammaChoice(const QVariant Choice) {
  Settings->SetValue("CameraColorGamma",Choice);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

void CB_WorkColorChoice(const QVariant Choice) {
  Settings->SetValue("WorkColor",Choice);
  PreCalcTransforms();
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

void CB_CMQualityChoice(const QVariant Choice) {
  Settings->SetValue("CMQuality",Choice);
  MainWindow->UpdateSettings();
  PreCalcTransforms();
  Update(ptProcessorPhase_NULL);
}

void CB_PreviewColorProfileButton() {
  QString ProfileFileName = QFileDialog::getOpenFileName(
                 NULL,
                 QObject::tr("Open Profile"),
                 Settings->GetString("PreviewColorProfilesDirectory"),
                 ProfilePattern);

  if (0 == ProfileFileName.size() ) {
    // Canceled just return
    return;
  } else {
    QFileInfo PathInfo(ProfileFileName);
    Settings->SetValue("PreviewColorProfilesDirectory",PathInfo.absolutePath());
    Settings->SetValue("PreviewColorProfile",PathInfo.absoluteFilePath());
  }

  // Reflect in gui.
  MainWindow->UpdateSettings();

  // Close old profile and open new one.
  cmsCloseProfile(PreviewColorProfile);
  PreviewColorProfile = cmsOpenProfileFromFile(
          Settings->GetString("PreviewColorProfile").toAscii().data(),
          "r");
  if (!PreviewColorProfile) {
    ptLogError(ptError_FileOpen,
         Settings->GetString("PreviewColorProfile").toAscii().data());
    assert(PreviewColorProfile);
  }
  PreCalcTransforms();
  // And update the preview.
  Update(ptProcessorPhase_NULL);
}

void CB_PreviewColorProfileIntentChoice(const QVariant Choice) {
  Settings->SetValue("PreviewColorProfileIntent",Choice);
  PreCalcTransforms();
  Update(ptProcessorPhase_NULL);
}

void CB_OutputColorProfileButton() {
  QString ProfileFileName = QFileDialog::getOpenFileName(
                 NULL,
                 QObject::tr("Open Profile"),
                 Settings->GetString("OutputColorProfilesDirectory"),
                 ProfilePattern);

  if (0 == ProfileFileName.size() ) {
    // Canceled just return
    return;
  } else {
    QFileInfo PathInfo(ProfileFileName);
    Settings->SetValue("OutputColorProfilesDirectory",PathInfo.absolutePath());
    Settings->SetValue("OutputColorProfile",PathInfo.absoluteFilePath());
  }

  // Reflect in gui.
  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output) {
    if (Settings->GetInt("IndicateExposure")==1) {
      Update(ptProcessorPhase_NULL);
    } else {
      Update(ptProcessorPhase_OnlyHistogram);
    }
  }
  MainWindow->UpdateSettings();
}

void CB_OutputColorProfileResetButton() {
  Settings->SetValue("OutputColorProfile",
                     (Settings->GetString("UserDirectory") + "Profiles/Output/sRGB.icc").toAscii().data());
  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output) {
    if (Settings->GetInt("IndicateExposure")==1) {
      Update(ptProcessorPhase_NULL);
    } else {
      Update(ptProcessorPhase_OnlyHistogram);
    }
  }
  MainWindow->UpdateSettings();
}

void CB_OutputColorProfileIntentChoice(const QVariant Choice) {
  Settings->SetValue("OutputColorProfileIntent",Choice);
  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output) {
    if (Settings->GetInt("IndicateExposure")==1) {
      Update(ptProcessorPhase_NULL);
    } else {
      Update(ptProcessorPhase_OnlyHistogram);
    }
  }
  MainWindow->UpdateSettings();
}

void CB_StyleChoice(const QVariant Choice) {
  Settings->SetValue("Style",Choice);
  if (Settings->GetInt("Style") == ptStyle_None) {
    Settings->SetValue("CustomCSSFile","");
    Theme->Reset();
  } else if (Settings->GetInt("Style") == ptStyle_Normal) {
    Theme->Normal(Settings->GetInt("StyleHighLight"));
  } else if (Settings->GetInt("Style") == ptStyle_50Grey) {
    Theme->MidGrey(Settings->GetInt("StyleHighLight"));
  } else if (Settings->GetInt("Style") == ptStyle_VeryDark) {
    Theme->VeryDark(Settings->GetInt("StyleHighLight"));
  } else {
    Theme->DarkGrey(Settings->GetInt("StyleHighLight"));
  }
#ifdef Q_OS_MAC
//dirty hack to make theming work
  MainWindow->MainSplitter->setStyleSheet("");
#endif

  MainWindow->MainTabBook->setStyle(Theme->ptStyle);
  MainWindow->ProcessingTabBook->setStyle(Theme->ptStyle);
  MainWindow->BottomContainer->setStyle(Theme->ptStyle);
  MainWindow->PipeControlWidget->setStyle(Theme->ptStyle);
  MainWindow->MainSplitter->setStyle(Theme->ptStyle);
  MainWindow->ControlSplitter->setStyle(Theme->ptStyle);
  MainWindow->ViewSplitter->setStyle(Theme->ptStyle);
  MainWindow->ViewStartPage->setStyle(Theme->ptStyle);

  TheApplication->setPalette(Theme->ptPalette);

  MainWindow->MainTabBook->setStyleSheet(Theme->ptStyleSheet);
  MainWindow->BottomContainer->setStyleSheet(Theme->ptStyleSheet);
  MainWindow->PipeControlWidget->setStyleSheet(Theme->ptStyleSheet);
  MainWindow->StatusWidget->setStyleSheet(Theme->ptStyleSheet);
  MainWindow->SearchWidget->setStyleSheet(Theme->ptStyleSheet);
  MainWindow->ViewStartPageFrame->setStyleSheet(Theme->ptStyleSheet);
#ifdef Q_OS_MAC
//dirty hack to make theming work
  if(Theme->MacStyleFlag){
  MainWindow->MainSplitter->setStyleSheet("background-color:"+Theme->MacBackGround+";");
  }
#endif
  MainWindow->UpdateToolBoxes();
  SetBackgroundColor(Settings->GetInt("BackgroundColor"));
  CB_SliderWidthInput(Settings->GetInt("SliderWidth"));
}

void CB_StyleHighLightChoice(const QVariant Choice) {
  Settings->SetValue("StyleHighLight",Choice);
  CB_StyleChoice(Settings->GetInt("Style"));
}

void CB_LoadStyleButton() {
  QString FileName;

  FileName = QFileDialog::getOpenFileName(NULL,
    QObject::tr("Open Image"),
    Settings->GetString("UserDirectory"),
    QObject::tr("CSS files (*.css *.qss);;All files(*.*)")
  );

  if (FileName.size() == 0) {
    return;
  } else {
    Settings->SetValue("CustomCSSFile", FileName);
    Theme->SetCustomCSS(FileName);
    CB_StyleChoice(Settings->GetInt("Style"));
  }
}


void CB_SaveConfirmationCheck(const QVariant Check) {
  Settings->SetValue("SaveConfirmation",Check);
}

void CB_ResetSettingsConfirmationCheck(const QVariant Check) {
  Settings->SetValue("ResetSettingsConfirmation",Check);
}

void CB_FullPipeConfirmationCheck(const QVariant Check) {
  Settings->SetValue("FullPipeConfirmation",Check);
}


void CB_WriteBackupSettingsCheck(const QVariant Check) {
  Settings->SetValue("WriteBackupSettings",Check);
}

void CB_MemoryTestInput(const QVariant Value) {
  Settings->SetValue("MemoryTest",0);
  if (Value.toInt()>0)
    if (ptMessageBox::question(MainWindow,
        QObject::tr("Are you sure?"),
        QObject::tr("If you don't stop me, I will waste %1 MB of memory.").arg(Value.toInt()),
        QMessageBox::Ok,QMessageBox::Cancel)==QMessageBox::Ok){
      // allocate orphaned memory for testing
      char (*Test) = (char (*)) CALLOC2(Value.toInt()*1024*1024,1);
      memset(Test, '\0', Value.toInt()*1024*1024);
      ptMessageBox::critical(0,"Feedback","Memory wasted ;-)");
    }
}

void SetDetailViewRect(const ptStatus, QRect rect) {
  DetailViewRect = rect;
}
void CB_PipeSizeChoice(const QVariant Choice) {

  short PreviousPipeSize = Settings->GetInt("PipeSize");

  if (Choice == ptPipeSize_Full &&
      Settings->GetInt("FullPipeConfirmation")==1 &&
      (Settings->GetInt("ImageH") > 2000 ||
       Settings->GetInt("ImageW") > 2000)) {
    ptMessageBox msgBox;
    msgBox.setWindowTitle(QObject::tr("Really switch to 1:1 pipe?"));
    msgBox.setText(QObject::tr("Switching to 1:1 pipe will increase memory usage and processing time greatly.\nAre you sure?"));

    QPushButton *DetailButton = msgBox.addButton(QObject::tr("Detail view"), QMessageBox::ActionRole);
    QPushButton *CancelButton = msgBox.addButton(QMessageBox::Cancel);
    msgBox.addButton(QMessageBox::Ok);

    msgBox.exec();

    if (msgBox.clickedButton() == CancelButton ||
        (msgBox.clickedButton() == DetailButton &&
         Settings->GetInt("HaveImage")==0)) {
      Settings->SetValue("PipeSize",PreviousPipeSize);
      return;
    } else if (msgBox.clickedButton() == DetailButton &&
               Settings->GetInt("HaveImage")==1) {
      uint16_t Width = 0;
      uint16_t Height = 0;
      short OldZoom = 0;
      short OldZoomMode = 0;
      if (Settings->GetInt("DetailViewActive")==0) {
        Settings->SetValue("DetailViewScale", PreviousPipeSize);
        if (TheProcessor->m_Image_DetailPreview == NULL)
          TheProcessor->m_Image_DetailPreview = new ptImage();
        // save a full image if we have several detail views without full view
        if (Settings->GetInt("IsRAW")==0) {
          TheProcessor->m_Image_DetailPreview->SetScaled(TheProcessor->m_Image_AfterDcRaw,
                                                         Settings->GetInt("Scaled"));
        } else {
          TheProcessor->m_Image_DetailPreview->Set(TheProcessor->m_Image_AfterDcRaw);
        }
      }
      // display the cache image and start crop mode

      // First : make sure we have Image_AfterDcRaw in the view window.
      // Anything else might have undergone geometric transformations that are
      // impossible to calculate reverse to a spot in dcraw.
      Width = TheProcessor->m_Image_DetailPreview->m_Width;
      Height = TheProcessor->m_Image_DetailPreview->m_Height;
      OldZoom = Settings->GetInt("Zoom");
      OldZoomMode = Settings->GetInt("ZoomMode");
      ViewWindow->ZoomToFit();
      UpdatePreviewImage(TheProcessor->m_Image_DetailPreview);

      // Allow to be selected in the view window. And deactivate main.
      BlockTools(1);
      ViewWindow->ShowStatus(QObject::tr("Detail view"));
      ViewWindow->StartSimpleRect(SetDetailViewRect);
      while (ViewWindow->interaction() == iaSelectRect) {
        QApplication::processEvents();
      }

      // Selection is done at this point. Disallow it further and activate main.
      BlockTools(0);

      if (DetailViewRect.width() >>4 <<4 > 19 &&
          DetailViewRect.height() >>4 <<4 > 19) {
        Settings->SetValue("DetailViewActive",1);
        short CachedPipeSize = Settings->GetInt("DetailViewScale");
        Settings->SetValue("DetailViewCropX", ((DetailViewRect.left() >>4) <<4) << CachedPipeSize);
        Settings->SetValue("DetailViewCropY", ((DetailViewRect.top() >>4) <<4) << CachedPipeSize);
        Settings->SetValue("DetailViewCropW", ((DetailViewRect.width() >>4) <<4) << CachedPipeSize);
        Settings->SetValue("DetailViewCropH", ((DetailViewRect.height() >>4) <<4) << CachedPipeSize);
        Settings->ToDcRaw(TheDcRaw);
      } else {
        ptMessageBox::information(NULL,"No crop","Too small. Please try again!");
        //ViewWindow->Zoom(OldZoom,0);    // TODOSR: re-enable
        Settings->SetValue("ZoomMode",OldZoomMode);
        Update(ptProcessorPhase_NULL);
        if (Settings->GetInt("DetailViewActive")==0) {
          delete TheProcessor->m_Image_DetailPreview;
          TheProcessor->m_Image_DetailPreview = NULL;
        }
        Settings->SetValue("PipeSize",PreviousPipeSize);
        return;
      }

      //ViewWindow->Zoom(OldZoom,0);    // TODOSR: re-enable
      Settings->SetValue("ZoomMode",OldZoomMode);

    } else { // 1:1 full mode
      // clean up possible detail view cache
      if (TheProcessor->m_Image_DetailPreview != NULL) {
        delete TheProcessor->m_Image_DetailPreview;
        TheProcessor->m_Image_DetailPreview = NULL;
        Settings->SetValue("DetailViewActive",0);
        Settings->SetValue("DetailViewCropX", 0);
        Settings->SetValue("DetailViewCropY", 0);
        Settings->SetValue("DetailViewCropW", 0);
        Settings->SetValue("DetailViewCropH", 0);
        Settings->ToDcRaw(TheDcRaw);
      }
    }
  } else {
    // clean up possible detail view cache
    if (TheProcessor->m_Image_DetailPreview != NULL) {
      delete TheProcessor->m_Image_DetailPreview;
      TheProcessor->m_Image_DetailPreview = NULL;
      Settings->SetValue("DetailViewActive",0);
      Settings->SetValue("DetailViewCropX", 0);
      Settings->SetValue("DetailViewCropY", 0);
      Settings->SetValue("DetailViewCropW", 0);
      Settings->SetValue("DetailViewCropH", 0);
      Settings->ToDcRaw(TheDcRaw);
    }
  }

  MainWindow->UpdateToolBoxes();

  Settings->SetValue("PipeSize",Choice);
  short PipeSize = Settings->GetInt("PipeSize");
  short Expansion = PreviousPipeSize-PipeSize;

  if (Settings->GetInt("DetailViewActive")==0) {
    // Following adaptation is needed for the case spot WB is in place.
    if (Expansion > 0) {
      Settings->SetValue("VisualSelectionX",
                         Settings->GetInt("VisualSelectionX")<<Expansion);
      Settings->SetValue("VisualSelectionY",
                         Settings->GetInt("VisualSelectionY")<<Expansion);
      Settings->SetValue("VisualSelectionWidth",
                         Settings->GetInt("VisualSelectionWidth")<<Expansion);
      Settings->SetValue("VisualSelectionHeight",
                         Settings->GetInt("VisualSelectionHeight")<<Expansion);
    } else {
      Expansion = -Expansion;
      Settings->SetValue("VisualSelectionX",
                         Settings->GetInt("VisualSelectionX")>>Expansion);
      Settings->SetValue("VisualSelectionY",
                         Settings->GetInt("VisualSelectionY")>>Expansion);
      Settings->SetValue("VisualSelectionWidth",
                         Settings->GetInt("VisualSelectionWidth")>>Expansion);
      Settings->SetValue("VisualSelectionHeight",
                         Settings->GetInt("VisualSelectionHeight")>>Expansion);
    }
  }

  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);

  ALLOCATED(10000000);
}


void CB_RunModeCheck(const QVariant Check) {
  Settings->SetValue("RunMode",Check);
  MainWindow->UpdateSettings();
  if (Settings->GetInt("RunMode")==0) {
    Update(ptProcessorPhase_Output);
  }
}

void CB_PreviewModeButton(const QVariant State) {
  Settings->SetValue("PreviewTabMode",State);
  if (Settings->GetInt("PreviewTabMode")) {
    Settings->SetValue("PreviewMode",ptPreviewMode_Tab);
    MainWindow->PreviewModeButton->setChecked(1);
  } else {
    Settings->SetValue("PreviewMode",ptPreviewMode_End);
    MainWindow->PreviewModeButton->setChecked(0);
  }
  Update(ptProcessorPhase_NULL);
}

void CB_RunButton() {
  short OldRunMode = Settings->GetInt("RunMode");
  Settings->SetValue("RunMode",0);
  Update(ptProcessorPhase_Output);
  Settings->SetValue("RunMode",OldRunMode);
  MainWindow->UpdateSettings();
}

void ResetButtonHandler(const short mode) {
  int DoOpen = 1;
  if (mode == ptResetMode_Full) { // full reset
    if ( Settings->GetInt("ResetSettingsConfirmation")==1 ) {
      ptMessageBox msgBox;
      msgBox.setIcon(QMessageBox::Question);
      msgBox.setWindowTitle(QObject::tr("Reset?"));
      msgBox.setText(QObject::tr("Reset to neutral values?\n"));
      msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Ok);
      if ( msgBox.exec()==QMessageBox::Cancel ) {
        DoOpen = 0;
      }
    }
    if ( DoOpen==1 ) {
      CB_OpenSettingsFile(Settings->GetString("PresetDirectory") + "/Neutral_absolute.pts");
    }
  } else if (mode == ptResetMode_User) { // reset to startup settings
    if ( Settings->GetInt("ResetSettingsConfirmation")==1 ) {
      ptMessageBox msgBox;
      msgBox.setIcon(QMessageBox::Question);
      msgBox.setWindowTitle(QObject::tr("Reset?"));
      msgBox.setText(QObject::tr("Reset to start up settings?\n"));
      msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Ok);
      if ( msgBox.exec()==QMessageBox::Cancel ) {
        DoOpen = 0;
      }
    }
    if ( DoOpen==1 ) {
      CB_OpenSettingsFile(Settings->GetString("StartupSettingsFile"));
    }
  } else if (mode == ptResetMode_OpenPreset) { // open preset file
    CB_OpenPresetFileButton();
  } else { // open settings file
    CB_OpenSettingsFileButton();
  }
}

void CB_ResetButton() {
  ResetButtonHandler(Settings->GetInt("ResetButtonMode"));
}

void CB_SpecialPreviewChoice(const QVariant Choice) {
  Settings->SetValue("SpecialPreview",Choice);
  Update(ptProcessorPhase_NULL);
}

void CB_GimpExecCommandButton() {

  QString GimpExecCommandString = QFileDialog::getOpenFileName(NULL,
    QObject::tr("Get Gimp command"),
    Settings->GetString("UserDirectory"),
    "All files (*.*)");

  if (0 == GimpExecCommandString.size() ) {
    // Canceled just return
    return;
  } else {
    QFileInfo PathInfo(GimpExecCommandString);
    Settings->SetValue("GimpExecCommand",PathInfo.absoluteFilePath());
  }

  // Reflect in gui.
  MainWindow->UpdateSettings();
}

void CB_RememberSettingLevelChoice(const QVariant Choice) {
  Settings->SetValue("RememberSettingLevel",Choice);
  MainWindow->UpdateSettings();
}

void CB_StartupSettingsCheck(const QVariant State) {
  Settings->SetValue("StartupSettings",State);
}

void CB_StartupSettingsResetCheck(const QVariant State) {
  Settings->SetValue("StartupSettingsReset",State);
}

void CB_StartupSettingsButton() {
  QString StartupSettingsString = QFileDialog::getOpenFileName(NULL,
    QObject::tr("Get preset file"),
    Settings->GetString("PresetDirectory"),
    SettingsFilePattern);

  if (0 == StartupSettingsString.size() ) {
    // Canceled just return
    return;
  } else {
    QFileInfo PathInfo(StartupSettingsString);
    Settings->SetValue("PresetDirectory",PathInfo.absolutePath());
    Settings->SetValue("StartupSettingsFile",PathInfo.absoluteFilePath());
  }

  // Reflect in gui.
  MainWindow->UpdateSettings();
}

void CB_StartupUIModeChoice(const QVariant Choice) {
  Settings->SetValue("StartupUIMode",Choice);
}

void CB_StartupPipeSizeChoice(const QVariant Choice) {
  Settings->SetValue("StartupPipeSize", Choice);
}

void CB_CropInitialZoomChoice(const QVariant Choice) {
  Settings->SetValue("CropInitialZoom",Choice);
}

void CB_InputsAddPowerLawCheck(const QVariant State) {
  Settings->SetValue("InputsAddPowerLaw",State);
  if (Settings->GetInt("InputsAddPowerLaw")) {
    Settings->SetValue("InputPowerFactor",2.2);
  } else {
    Settings->SetValue("InputPowerFactor",1.0);
  }
  Update(ptProcessorPhase_RGB);
}

void CB_ToolBoxModeCheck(const QVariant State) {
  Settings->SetValue("ToolBoxMode",State);
  MainWindow->OnToolBoxesEnabledTriggered(Settings->GetInt("ToolBoxMode"));
}

void CB_TabStatusIndicatorInput(const QVariant Value) {
  Settings->SetValue("TabStatusIndicator",Value);
  MainWindow->UpdateSettings();
}

void CB_PreviewTabModeCheck(const QVariant State) {
  Settings->SetValue("PreviewTabMode",State);
  if (Settings->GetInt("PreviewTabMode")) {
    Settings->SetValue("PreviewMode",ptPreviewMode_Tab);
  } else {
    Settings->SetValue("PreviewMode",ptPreviewMode_End);
  }
  Update(ptProcessorPhase_NULL);
}

void CB_BackgroundColorCheck(const QVariant State) {
  Settings->SetValue("BackgroundColor",State);
  SetBackgroundColor(Settings->GetInt("BackgroundColor"));
}

void CB_BackgroundColorButton() {
  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("BackgroundRed"));
  Color.setGreen(Settings->GetInt("BackgroundGreen"));
  Color.setBlue(Settings->GetInt("BackgroundBlue"));
  QColorDialog Dialog(Color,NULL);
  Dialog.setStyle(Theme->ptSystemStyle);
  Dialog.setPalette(Theme->ptSystemPalette);
  Dialog.exec();
  QColor TestColor = Dialog.selectedColor();
  if (TestColor.isValid()) {
    Color = TestColor;
    Settings->SetValue("BackgroundRed",Color.red());
    Settings->SetValue("BackgroundGreen",Color.green());
    Settings->SetValue("BackgroundBlue",Color.blue());
    Pix.fill(Color);
    MainWindow->BackgroundColorButton->setIcon(Pix);
    if (Settings->GetInt("BackgroundColor")){
      SetBackgroundColor(1);
    }
  }
}

void SetBackgroundColor(int SetIt) {
  if (SetIt) {
    QPalette BGPal;
    BGPal.setColor(QPalette::Background, QColor(Settings->GetInt("BackgroundRed"),
                                                Settings->GetInt("BackgroundGreen"),
                                                Settings->GetInt("BackgroundBlue")));
    ViewWindow->setPalette(BGPal);
    MainWindow->ViewStartPage->setPalette(BGPal);
  } else {
    ViewWindow->setPalette(Theme->ptPalette);
    MainWindow->ViewStartPage->setPalette(Theme->ptPalette);
  }
}

void CB_SliderWidthInput(const QVariant Value) {
  Settings->SetValue("SliderWidth",Value);
  if (Settings->GetInt("SliderWidth") == 0)
//  maximum slider width is equal to the toolbar width
    MainWindow->setStyleSheet("ptSlider { max-width: " + QString("%1").arg(10000) + "px; }");
  else
//  maximum slider width is equal to "SliderWidth"
    MainWindow->setStyleSheet("ptSlider { max-width: " + QString("%1").arg(Settings->GetInt("SliderWidth")) + "px; }");
}

void CB_SaveButtonModeChoice(const QVariant Choice) {
  Settings->SetValue("SaveButtonMode",Choice);
  SaveButtonToolTip(Settings->GetInt("SaveButtonMode"));
}

void CB_ResetButtonModeChoice(const QVariant Value) {
  Settings->SetValue("ResetButtonMode",Value);
}

void CB_SearchBarEnableCheck(const QVariant State) {
  Settings->SetValue("SearchBarEnable",State);
  MainWindow->SearchWidget->setVisible(Settings->GetInt("SearchBarEnable"));
}

void SaveButtonToolTip(const short mode) {
  if (mode==ptOutputMode_Full) {
    MainWindow->WritePipeButton->setToolTip(QObject::tr("Save full size image"));
  } else if (mode==ptOutputMode_Pipe) {
    MainWindow->WritePipeButton->setToolTip(QObject::tr("Save current pipe"));
  } else if (mode==ptOutputMode_Jobfile) {
    MainWindow->WritePipeButton->setToolTip(QObject::tr("Save job file"));
  } else if (mode==ptOutputMode_Settingsfile) {
    MainWindow->WritePipeButton->setToolTip(QObject::tr("Save settings file"));
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Camera Tab
// Partim Generic Corrections.
//
////////////////////////////////////////////////////////////////////////////////

void CB_OpenFileButton() {
  CB_MenuFileOpen(0);
}

void CB_OpenSettingsFile(QString SettingsFileName) {
  if (0 == SettingsFileName.size()) return;
  short NextPhase = 1;
  short ReturnValue = ReadSettingsFile(SettingsFileName, NextPhase);
  if (ReturnValue) {
    ptMessageBox::critical(0,"Error","No valid settings file!\n" + SettingsFileName);
    return;
  }
  if (NextPhase == 1) {
    if (Settings->GetInt("HaveImage")==1) {
      CalculateMultipliersFromTemperature();
      NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
    }
    if (Settings->GetInt("AutomaticPipeSize") && Settings->ToolIsActive("TabResize")) {
      if (!CalculatePipeSize())
        Update(ptProcessorPhase_Raw,ptProcessorPhase_Load);
    } else {
      Update(ptProcessorPhase_Raw,ptProcessorPhase_Load);
    }
  } else {
    //TODO: check Nextphase against automatic pipesize
    Update(NextPhase);
  }
}

void CB_OpenSettingsFileButton() {
  QString SettingsFilePattern =
    QObject::tr("Settings files (*.pts *.ptj);;All files (*.*)");
  QString SettingsFileName =
    QFileDialog::getOpenFileName(NULL,
                                 QObject::tr("Open setting file"),
                                 Settings->GetString("RawsDirectory"),
                                 SettingsFilePattern);
  if (0 == SettingsFileName.size()) return;
  CB_OpenSettingsFile(SettingsFileName);
}

void CB_OpenPresetFileButton() {
  QString SettingsFilePattern =
    QObject::tr("Settings files (*.pts *.ptj);;All files (*.*)");
  QString SettingsFileName =
    QFileDialog::getOpenFileName(NULL,
                                 QObject::tr("Open preset"),
                                 Settings->GetString("PresetDirectory"),
                                 SettingsFilePattern);
  if (0 == SettingsFileName.size()) return;
  CB_OpenSettingsFile(SettingsFileName);
}

void CB_BadPixelsChoice(const QVariant Choice) {
  Settings->SetValue("HaveBadPixels",Choice);
  short Cancelled = 0;
  if (Choice.toInt() == 1) {
    // Request to load one.
    QString BadPixelsFileName =
      QFileDialog::getOpenFileName(NULL,
                                   QObject::tr("Open 'bad pixels' file"),
                                   NULL);
    if (0 == Settings->GetString("BadPixelsFileName").size() ) {
      // A cancel we'll interprate as no bad pixel
      Settings->SetValue("HaveBadPixels",0);
      Cancelled = 1;
    } else {
      Settings->SetValue("HaveBadPixels",2);
      Settings->SetValue("BadPixelsFileName",BadPixelsFileName);
    }
  }
  if (!Cancelled) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Load);
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  }
}

void CB_DarkFrameChoice(const QVariant Choice) {
  Settings->SetValue("HaveDarkFrame",Choice);
  short Cancelled = 0;
  if (Choice.toInt() == 1) {
    // Request to load one.
    QString DarkFrameFileName =
      QFileDialog::getOpenFileName(NULL,
                                   QObject::tr("Open 'dark frame' file"),
                                   NULL);
    if (0 == Settings->GetString("DarkFrameFileName").size() ) {
      // A cancel we'll interprate as no bad pixel
      Settings->SetValue("HaveDarkFrame",0);
      Cancelled = 1;
    } else {
      Settings->SetValue("HaveDarkFrame",2);
      Settings->SetValue("DarkFrameFileName",DarkFrameFileName);
    }
  }
  if (!Cancelled) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Load);
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Camera Tab
// Partim White Balance.
//
////////////////////////////////////////////////////////////////////////////////
void SelectSpotWBDone(const ptStatus ExitStatus, const QRect SelectionRect);

void CB_WhiteBalanceChoice(const QVariant Choice) {
  Settings->SetValue("WhiteBalance",Choice);
  if (Settings->GetInt("HaveImage") == 0 || Settings->GetInt("IsRAW") == 0) return;

  switch (Choice.toInt()) {
    case ptWhiteBalance_Camera :
    case ptWhiteBalance_Auto :
    case ptWhiteBalance_Manual :
      // In fact all of above just translates to settings into
      // DcRaw handled via GuiSettingsToDcRaw. Nothing more to do.
      Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
      break;

    case ptWhiteBalance_Spot: {
      // First : make sure we have Image_AfterDcRaw in the view window.
      // Anything else might have undergone geometric transformations that are
      // impossible to calculate reverse to a spot in dcraw.
      ViewWindow->SaveZoom();
      ViewWindow->ZoomToFit();
      UpdatePreviewImage(TheProcessor->m_Image_AfterDcRaw);

      // Allow to be selected in the view window. And deactivate main.
      BlockTools(1);
      ViewWindow->ShowStatus(QObject::tr("Spot WB"));
      ReportProgress(QObject::tr("Spot WB"));
      ViewWindow->StartSimpleRect(SelectSpotWBDone);
      ViewWindow->setFocus();
      break;
    }

    default :
      // Here we have presets selected from ptWhiteBalances.
      // GuiSettings->m_WhiteBalance should point
      // to the right index into the array.
      Settings->SetValue("RMultiplier",
                         ptWhiteBalances[Choice.toInt()].m_Multipliers[0]);
      Settings->SetValue("GMultiplier",
                         ptWhiteBalances[Choice.toInt()].m_Multipliers[1]);
      Settings->SetValue("BMultiplier",
                         ptWhiteBalances[Choice.toInt()].m_Multipliers[2]);
      Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

void SelectSpotWBDone(const ptStatus ExitStatus, const QRect SelectionRect) {
  // Selection is done at this point. Disallow it further and activate main.
  BlockTools(0);

  if (ExitStatus == stSuccess) {
    Settings->SetValue("VisualSelectionX", SelectionRect.left());
    Settings->SetValue("VisualSelectionY", SelectionRect.top());
    Settings->SetValue("VisualSelectionWidth", SelectionRect.width());
    Settings->SetValue("VisualSelectionHeight", SelectionRect.height());

    TRACEKEYVALS("Selection X","%d",
                 Settings->GetInt("VisualSelectionX"));
    TRACEKEYVALS("Selection Y","%d",
                 Settings->GetInt("VisualSelectionY"));
    TRACEKEYVALS("Selection W","%d",
                 Settings->GetInt("VisualSelectionWidth"));
    TRACEKEYVALS("Selection H","%d",
                 Settings->GetInt("VisualSelectionHeight"));
  }

  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  ViewWindow->RestoreZoom();
}

void CB_SpotWBButton() {
  CB_WhiteBalanceChoice(ptWhiteBalance_Spot);
}

void CB_ColorTemperatureInput(const QVariant Value) {
  Settings->SetValue("ColorTemperature",Value);
  Settings->SetValue("WhiteBalance",ptWhiteBalance_Manual);
  if (!TheDcRaw) return;
  CalculateMultipliersFromTemperature();
  NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_GreenIntensityInput(const QVariant Value) {
  Settings->SetValue("GreenIntensity",Value);
  Settings->SetValue("WhiteBalance",ptWhiteBalance_Manual);
  if (!TheDcRaw) return;
  CalculateMultipliersFromTemperature();
  NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_MultiplierEnhanceCheck(const QVariant State) {
  Settings->SetValue("MultiplierEnhance",State);
  if (!TheDcRaw) return;
  TheDcRaw->m_UserSetting_MaxMultiplier = Settings->GetInt("MultiplierEnhance");
  NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_RMultiplierInput(const QVariant Value) {
  Settings->SetValue("RMultiplier",Value);
  Settings->SetValue("WhiteBalance",ptWhiteBalance_Manual);
  NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_GMultiplierInput(const QVariant Value) {
  Settings->SetValue("GMultiplier",Value);
  Settings->SetValue("WhiteBalance",ptWhiteBalance_Manual);
  NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_BMultiplierInput(const QVariant Value) {
  Settings->SetValue("BMultiplier",Value);
  Settings->SetValue("WhiteBalance",ptWhiteBalance_Manual);
  NormalizeMultipliers(Settings->GetInt("MultiplierEnhance"));
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_ManualBlackPointCheck(const QVariant State) {
  short OldState = Settings->GetInt("ManualBlackPoint");
  Settings->SetValue("ManualBlackPoint",State);
  if (OldState) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  } else {
    MainWindow->UpdateSettings();
  }
}

void CB_BlackPointInput(const QVariant Value) {
  Settings->SetValue("BlackPoint",Value);
  if (Settings->GetInt("ManualBlackPoint")) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

void CB_ManualWhitePointCheck(const QVariant State) {
  short OldState = Settings->GetInt("ManualWhitePoint");
  Settings->SetValue("ManualWhitePoint",State);
  if (OldState) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  } else {
    MainWindow->UpdateSettings();
  }
}

void CB_WhitePointInput(const QVariant Value) {
  Settings->SetValue("WhitePoint",Value);
  if (Settings->GetInt("ManualWhitePoint")) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Camera Tab
// Partim Demosaicing.
//
////////////////////////////////////////////////////////////////////////////////

void CB_CaCorrectChoice(const QVariant Choice) {
  Settings->SetValue("CaCorrect",Choice);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_CaRedInput(const QVariant Value) {
  Settings->SetValue("CaRed",Value);
  if (Settings->GetInt("CaCorrect")==ptCACorrect_Manual)
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_CaBlueInput(const QVariant Value) {
  Settings->SetValue("CaBlue",Value);
  if (Settings->GetInt("CaCorrect")==ptCACorrect_Manual)
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_GreenEquilInput(const QVariant Value) {
  Settings->SetValue("GreenEquil",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_CfaLineDenoiseInput(const QVariant Value) {
  Settings->SetValue("CfaLineDenoise",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_AdjustMaximumThresholdInput(const QVariant Value) {
  Settings->SetValue("AdjustMaximumThreshold",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_RawDenoiseThresholdInput(const QVariant Value) {
  Settings->SetValue("RawDenoiseThreshold",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_HotpixelReductionInput(const QVariant Value) {
  Settings->SetValue("HotpixelReduction",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_BayerDenoiseChoice(const QVariant Choice) {
  Settings->SetValue("BayerDenoise",Choice);
  if (!Settings->GetInt("PipeSize")) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

void CB_InterpolationChoice(const QVariant Choice) {
  Settings->SetValue("Interpolation",Choice);
  MainWindow->UpdateSettings();
  if (!Settings->GetInt("PipeSize")) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

void CB_InterpolationPassesInput(const QVariant Value) {
  Settings->SetValue("InterpolationPasses",Value);
  if (!Settings->GetInt("PipeSize") &&
      (Settings->GetInt("Interpolation")==ptInterpolation_DCB ||
       Settings->GetInt("Interpolation")==ptInterpolation_DCBSoft ||
       Settings->GetInt("Interpolation")==ptInterpolation_DCBSharp)) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

void CB_MedianPassesInput(const QVariant Value) {
  Settings->SetValue("MedianPasses",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

void CB_ESMedianPassesInput(const QVariant Value) {
  Settings->SetValue("ESMedianPasses",Value);
  if (!Settings->GetInt("PipeSize")) {
    Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
  }
}

void CB_EeciRefineCheck(const QVariant State) {
  Settings->SetValue("EeciRefine",State);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Demosaic);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Camera Tab
// Partim Highlight.
//
////////////////////////////////////////////////////////////////////////////////

void CB_ClipModeChoice(const QVariant Choice) {
  Settings->SetValue("ClipMode",Choice);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

void CB_ClipParameterInput(const QVariant Value) {
  Settings->SetValue("ClipParameter",Value);
  Update(ptProcessorPhase_Raw,ptProcessorPhase_Highlights);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to Lensfun (Geometry tab)
//
////////////////////////////////////////////////////////////////////////////////

// General tab

void CB_LfunFocalInput(const QVariant Value) {
  Settings->SetValue("LfunFocal", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunApertureInput(const QVariant Value) {
  Settings->SetValue("LfunAperture", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistanceInput(const QVariant Value) {
  Settings->SetValue("LfunDistance", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunScaleInput(const QVariant Value) {
  Settings->SetValue("LfunScale", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunAutoScaleCheck(const QVariant State) {
  Settings->SetValue("LfunAutoScale",State);
  MainWindow->LfunScaleWidget->setEnabled(!State.toBool());
  Update(ptProcessorPhase_Geometry);
}


// CA and Vignette tab

void CB_LfunCAModelChoice(const QVariant Choice) {
  Settings->SetValue("LfunCAModel", Choice);
  MainWindow->UpdateLfunCAUI();
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCALinearKrInput(const QVariant Value) {
  Settings->SetValue("LfunCALinearKr", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCALinearKbInput(const QVariant Value) {
  Settings->SetValue("LfunCALinearKb", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCAPoly3VrInput(const QVariant Value) {
  Settings->SetValue("LfunCAPoly3Vr", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCAPoly3VbInput(const QVariant Value) {
  Settings->SetValue("LfunCAPoly3Vb", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCAPoly3CrInput(const QVariant Value) {
  Settings->SetValue("LfunCAPoly3Cr", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCAPoly3CbInput(const QVariant Value) {
  Settings->SetValue("LfunCAPoly3Cb", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCAPoly3BrInput(const QVariant Value) {
  Settings->SetValue("LfunCAPoly3Br", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunCAPoly3BbInput(const QVariant Value) {
  Settings->SetValue("LfunCAPoly3Bb", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunVignetteModelChoice(const QVariant Choice) {
  Settings->SetValue("LfunVignetteModel", Choice);
  MainWindow->UpdateLfunVignetteUI();
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunVignettePoly6K1Input(const QVariant Value) {
  Settings->SetValue("LfunVignettePoly6K1", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunVignettePoly6K2Input(const QVariant Value) {
  Settings->SetValue("LfunVignettePoly6K2", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunVignettePoly6K3Input(const QVariant Value) {
  Settings->SetValue("LfunVignettePoly6K3", Value);
  Update(ptProcessorPhase_Geometry);
}


// Lens correction tab

void CB_LfunSrcGeoChoice(const QVariant Choice) {
  Settings->SetValue("LfunSrcGeo", Choice);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunTargetGeoChoice(const QVariant Choice) {
  Settings->SetValue("LfunTargetGeo", Choice);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistModelChoice(const QVariant Choice) {
  Settings->SetValue("LfunDistModel", Choice);
  MainWindow->UpdateLfunDistUI();
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistPoly3K1Input(const QVariant Value) {
  Settings->SetValue("LfunDistPoly3K1", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistPoly5K1Input(const QVariant Value) {
  Settings->SetValue("LfunDistPoly5K1", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistPoly5K2Input(const QVariant Value) {
  Settings->SetValue("LfunDistPoly5K2", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistFov1OmegaInput(const QVariant Value) {
  Settings->SetValue("LfunDistFov1Omega", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistPTLensAInput(const QVariant Value) {
  Settings->SetValue("LfunDistPTLensA", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistPTLensBInput(const QVariant Value) {
  Settings->SetValue("LfunDistPTLensB", Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_LfunDistPTLensCInput(const QVariant Value) {
  Settings->SetValue("LfunDistPTLensC", Value);
  Update(ptProcessorPhase_Geometry);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Geometry Tab
// Partim Defish
//
////////////////////////////////////////////////////////////////////////////////

void CB_DefishCheck(const QVariant State) {
  Settings->SetValue("Defish",State);
  Update(ptProcessorPhase_Geometry);
}

void CB_DefishFocalLengthInput(const QVariant Value) {
  Settings->SetValue("DefishFocalLength", Value);
  if (Settings->GetInt("Defish"))
    Update(ptProcessorPhase_Geometry);
}

void CB_DefishScaleInput(const QVariant Value) {
  Settings->SetValue("DefishScale", Value);
  if (Settings->GetInt("Defish"))
    Update(ptProcessorPhase_Geometry);
}

void CB_DefishAutoScaleCheck(const QVariant State) {
  Settings->SetValue("DefishAutoScale",State);
  MainWindow->DefishScaleWidget->setEnabled(!State.toBool());
  Update(ptProcessorPhase_Geometry);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Geometry Tab
// Partim Rotate
//
////////////////////////////////////////////////////////////////////////////////

void CB_RotateLeftButton() {
  double Value = Settings->GetDouble("Rotate");
  Value -= 90.0;
  if (Value < -180.0) Value += 360.0;
  Settings->SetValue("Rotate",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_RotateRightButton() {
  double Value = Settings->GetDouble("Rotate");
  Value += 90.0;
  if (Value > 180.0) Value -= 360.0;
  Settings->SetValue("Rotate",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_RotateAngleButton() {
  if (Settings->GetInt("HaveImage")==0) {
    ptMessageBox::information(MainWindow,
      QObject::tr("No selection"),
      QObject::tr("Open an image first."));
    return;
  }

  // Rerun the part of geometry stage before rotate to get correct preview
  // image in TheProcessor->m_Image_AfterGeometry
  TheProcessor->RunGeometry(ptProcessorStopBefore_Rotate);
  ViewWindow->SaveZoom();
  ViewWindow->ZoomToFit();
  UpdatePreviewImage(TheProcessor->m_Image_AfterGeometry); // Calculate in any case.

  // Allow to be selected in the view window. And deactivate main.
  ViewWindow->ShowStatus(QObject::tr("Get angle"));
  ReportProgress(QObject::tr("Get angle"));

  BlockTools(1);
  ViewWindow->StartLine();
  ViewWindow->setFocus();
}

void RotateAngleDetermined(const ptStatus ExitStatus, double RotateAngle) {
  // Selection is done at this point. Disallow it further and activate main.
  BlockTools(0);

  if (ExitStatus == stSuccess) {
    if (RotateAngle < -45.0) {
      RotateAngle += 180.0;
    }
    if (fabs(fabs(RotateAngle) - 90.0) < 45.0) {
      RotateAngle -= 90.0;
    }
    Settings->SetValue("Rotate",RotateAngle);
  }

  ViewWindow->RestoreZoom();
  Update(ptProcessorPhase_Geometry);
}

void CB_RotateInput(const QVariant Value) {
  Settings->SetValue("Rotate",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_PerspectiveFocalLengthInput(const QVariant Value) {
  Settings->SetValue("PerspectiveFocalLength",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_PerspectiveTiltInput(const QVariant Value) {
  Settings->SetValue("PerspectiveTilt",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_PerspectiveTurnInput(const QVariant Value) {
  Settings->SetValue("PerspectiveTurn",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_PerspectiveScaleXInput(const QVariant Value) {
  Settings->SetValue("PerspectiveScaleX",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_PerspectiveScaleYInput(const QVariant Value) {
  Settings->SetValue("PerspectiveScaleY",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_GridCheck(const QVariant State) {
  Settings->SetValue("Grid",State);
  ViewWindow->setGrid(Settings->GetInt("Grid"), Settings->GetInt("GridX"), Settings->GetInt("GridY"));
  Update(ptProcessorPhase_NULL);
}

void CB_GridXInput(const QVariant Value) {
  Settings->SetValue("GridX",Value);
  if (Settings->GetInt("Grid")) {
    ViewWindow->setGrid(Settings->GetInt("Grid"), Settings->GetInt("GridX"), Settings->GetInt("GridY"));
    Update(ptProcessorPhase_NULL);
  }
}

void CB_GridYInput(const QVariant Value) {
  Settings->SetValue("GridY",Value);
  if (Settings->GetInt("Grid")) {
    ViewWindow->setGrid(Settings->GetInt("Grid"), Settings->GetInt("GridX"), Settings->GetInt("GridY"));
    Update(ptProcessorPhase_NULL);
  }
}

void CB_FlipModeChoice(const QVariant Value) {
  Settings->SetValue("FlipMode",Value);
  Update(ptProcessorPhase_Geometry);
}

void CB_GeometryBlockCheck(const QVariant State) {
  Settings->SetValue("GeometryBlock",State);
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Geometry Tab
// Partim Crop
//
////////////////////////////////////////////////////////////////////////////////

void CB_FixedAspectRatioCheck(const QVariant Check) {
  Settings->SetValue("FixedAspectRatio", Check);
  MainWindow->UpdateCropToolUI();
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->setAspectRatio(Settings->GetInt("FixedAspectRatio"),
                                       Settings->GetInt("AspectRatioW"),
                                       Settings->GetInt("AspectRatioH"));
    ViewWindow->setFocus();
  }
}

void CB_AspectRatioWChoice(const QVariant Value) {
  Settings->SetValue("AspectRatioW",Value);
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->setAspectRatio(Settings->GetInt("FixedAspectRatio"),
                                       Settings->GetInt("AspectRatioW"),
                                       Settings->GetInt("AspectRatioH"));
    ViewWindow->setFocus();
  }
}

void CB_AspectRatioHChoice(const QVariant Value) {
  Settings->SetValue("AspectRatioH",Value);
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->setAspectRatio(Settings->GetInt("FixedAspectRatio"),
                                       Settings->GetInt("AspectRatioW"),
                                       Settings->GetInt("AspectRatioH"));
    ViewWindow->setFocus();
  }
}

void CB_CropExposureInput(const QVariant Value) {
  Settings->SetValue("CropExposure", Value);

  if (ViewWindow->interaction() == iaCrop) {
    if (GBusy) return;
    else GBusy = true;
    UpdatePreviewImage(TheProcessor->m_Image_AfterGeometry); // Calculate in any case.
    GBusy = false;
  }
}

void CB_CropOrientationButton() {
  int w = Settings->GetInt("AspectRatioW");
  int h = Settings->GetInt("AspectRatioH");

  if (w != h) {
    Settings->SetValue("AspectRatioW", h);
    Settings->SetValue("AspectRatioH", w);
    if (ViewWindow->interaction() == iaCrop) {
      ViewWindow->crop()->flipAspectRatio();
      ViewWindow->setFocus();
    }
  }
}

void CB_CropCenterHorButton() {
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->moveToCenter(1, 0);
    ViewWindow->setFocus();
  }
}

void CB_CropCenterVertButton() {
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->moveToCenter(0, 1);
    ViewWindow->setFocus();
  }
}

void CB_CropGuidelinesChoice(const QVariant Choice) {
  Settings->SetValue("CropGuidelines",Choice);
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->setGuidelines(Choice.toInt());
    ViewWindow->setFocus();
  }
}

void CB_LightsOutChoice(const QVariant Choice) {
  // View window takes care of Settings
  if (ViewWindow->interaction() == iaCrop) {
    ViewWindow->crop()->setLightsOut(Choice.toInt());
    ViewWindow->setFocus();
  }
}


// Prepare and start image crop interaction
void CB_MakeCropButton() {
  if (Settings->GetInt("HaveImage")==0) {
    ptMessageBox::information(MainWindow,
      QObject::tr("No crop possible"),
      QObject::tr("Open an image first."));
    return;
  }

  ViewWindow->ShowStatus(QObject::tr("Prepare"));
  ReportProgress(QObject::tr("Prepare for cropping"));

  // Rerun the part of geometry stage before crop to get correct preview
  // image in TheProcessor->m_Image_AfterGeometry
  TheProcessor->RunGeometry(ptProcessorStopBefore_Crop);
  UpdatePreviewImage(TheProcessor->m_Image_AfterGeometry); // Calculate in any case.

  // Allow to be selected in the view window. And deactivate main.
  ViewWindow->ShowStatus(QObject::tr("Crop"));
  ReportProgress(QObject::tr("Crop"));
  BlockTools(2);

  switch (Settings->GetInt("CropInitialZoom")) {
    case ptZoomLevel_Current:
      // nothing to do
      break;

    case ptZoomLevel_Fit:
      ViewWindow->ZoomToFit(0);
      break;

    default:
      ViewWindow->ZoomTo((float)Settings->GetInt("CropInitialZoom") / 100);
      break;
  }

  ViewWindow->StartCrop();          // always start the interaction first,
  MainWindow->UpdateCropToolUI();   // *then* update main window
  ViewWindow->setFocus();
}


// After-crop processing and cleanup.
void CleanupAfterCrop(const ptStatus CropStatus, const QRect CropRect) {
  BlockTools(0);

  if (CropStatus == stSuccess) {
    // Account for the pipesize factor.
    int XScale = 1<<Settings->GetInt("PipeSize");
    int YScale = 1<<Settings->GetInt("PipeSize");

    if ((CropRect.width() * XScale < 4) || (CropRect.height() * YScale < 4)) {
      ptMessageBox::information(MainWindow,
          QObject::tr("Crop too small"),
          QObject::tr("Crop rectangle needs to be at least 4x4 pixels in size.\nNo crop, try again."));

      if ((Settings->GetInt("CropW") < 4) || (Settings->GetInt("CropH") < 4)) {
        Settings->SetValue("Crop", 0);
        QCheckBox(MainWindow->CropWidget).setCheckState(Qt::Unchecked);
      }

      if(Settings->GetInt("RunMode")==1) {
        // we're in manual mode!
        Update(ptProcessorPhase_NULL);
      }
    } else {
      Settings->SetValue("Crop",1);
      Settings->SetValue("CropX",CropRect.left() * XScale);
      Settings->SetValue("CropY",CropRect.top() * YScale);
      Settings->SetValue("CropW",CropRect.width() * XScale);
      Settings->SetValue("CropH",CropRect.height() * YScale);
//      QCheckBox(MainWindow->CropWidget).setCheckState(Qt::Checked);
    }

    TRACEKEYVALS("PreviewImageW","%d",PreviewImage->m_Width);
    TRACEKEYVALS("PreviewImageH","%d",PreviewImage->m_Height);
    TRACEKEYVALS("XScale","%d",XScale);
    TRACEKEYVALS("YScale","%d",YScale);
    TRACEKEYVALS("CropX","%d",Settings->GetInt("CropX"));
    TRACEKEYVALS("CropY","%d",Settings->GetInt("CropY"));
    TRACEKEYVALS("CropW","%d",Settings->GetInt("CropW"));
    TRACEKEYVALS("CropH","%d",Settings->GetInt("CropH"));


  // Crop aborted, disable crop checkbox when no previous crop present
  } else {
    if ((Settings->GetInt("CropW") < 4) || (Settings->GetInt("CropH") < 4)) {
      Settings->SetValue("Crop", 0);
//      QCheckBox(MainWindow->CropWidget).setCheckState(Qt::Unchecked);
    }
  }

  Update(ptProcessorPhase_Geometry);
  MainWindow->UpdateCropToolUI();
}


void CB_ConfirmCropButton() {
  ViewWindow->crop()->stop(stSuccess);    // user confirmed crop
}

void CB_CancelCropButton() {
  ViewWindow->crop()->stop(stFailure);    // user cancelled crop
}


// En/disable an existing crop in the pipe
void CB_CropCheck(const QVariant State) {
  Settings->SetValue("Crop",State);

  if (State.toInt() != 0 &&
      (Settings->GetInt("CropW") <= 4 || Settings->GetInt("CropH") <= 4))
  {
    ptMessageBox::information(MainWindow,
        QObject::tr("No previous crop found"),
        QObject::tr("Set a crop rectangle now."));

    CB_MakeCropButton();
    return;
  }

  Update(ptProcessorPhase_Geometry);
}


////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Geometry Tab
// Partim Liquid rescale
//
////////////////////////////////////////////////////////////////////////////////

void CB_LqrEnergyChoice(const QVariant Choice) {
  Settings->SetValue("LqrEnergy",Choice);
  Update(ptProcessorPhase_Geometry);
}

void CB_LqrScalingChoice(const QVariant Choice) {
  Settings->SetValue("LqrScaling",Choice);
  MainWindow->UpdateLiquidRescaleUI();
  if (Settings->ToolIsActive("TabLiquidRescale"))
    Update(ptProcessorPhase_Geometry);
}

void CB_LqrHorScaleInput(const QVariant Value) {
  Settings->SetValue("LqrHorScale",Value);
  if (Settings->ToolIsActive("TabLiquidRescale"))
    Update(ptProcessorPhase_Geometry);
}

void CB_LqrVertScaleInput(const QVariant Value) {
  Settings->SetValue("LqrVertScale",Value);
  if (Settings->ToolIsActive("TabLiquidRescale"))
    Update(ptProcessorPhase_Geometry);
}

void CB_LqrWidthInput(const QVariant Value) {
  Settings->SetValue("LqrWidth",Value);
  if (Settings->ToolIsActive("TabLiquidRescale"))
    Update(ptProcessorPhase_Geometry);
}

void CB_LqrHeightInput(const QVariant Value) {
  Settings->SetValue("LqrHeight",Value);
  if (Settings->ToolIsActive("TabLiquidRescale"))
    Update(ptProcessorPhase_Geometry);
}

void CB_LqrVertFirstCheck(const QVariant Check) {
  Settings->SetValue("LqrVertFirst",Check);
  if (Settings->ToolIsActive("TabLiquidRescale"))
    Update(ptProcessorPhase_Geometry);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the Geometry Tab
// Partim Resize
//
////////////////////////////////////////////////////////////////////////////////

void CalculatePipeSizeHelper(const short Size, const bool NewImage) {
  if (NewImage) {
    Settings->SetValue("PipeSize",Size);
  } else {
    CB_PipeSizeChoice(Size);
  }
}

// returns 1 if pipe was updated
int CalculatePipeSize(const bool NewImage /* = False */) {
  uint16_t InSize = 0;
  if (Settings->GetInt("HaveImage")==0) return 0;
  if (Settings->GetInt("Crop")==0) {
    InSize = MAX(Settings->GetInt("ImageW"),Settings->GetInt("ImageH"));
  } else {
    InSize = MAX(Settings->GetInt("CropW"),Settings->GetInt("CropH"));
  }
  int s = MAX(0,((int)floor(logf((float)InSize*0.9/(float)Settings->GetInt("ResizeScale"))/logf(2))));
  if (s < Settings->GetInt("PipeSize")) {
    if (Settings->GetInt("RunMode") != 1) {// not manual mode
      ImageSaved = 1; // bad hack to check what happens in the next step
      CalculatePipeSizeHelper(s, NewImage);
      if (ImageSaved == 1 && !NewImage) {
        if (Settings->GetInt("PipeSize")==1) {
          ptMessageBox::information(NULL,"Failure!","Could not run on full size!\nWill stay on half size instead!");
          ImageSaved = 0;
          return 0;
        } else {
          ptMessageBox::information(NULL,"Failure!","Could not run on full size!\nWill run on half size instead!");
          CalculatePipeSizeHelper(1, NewImage);
        }
      }
    } else {
      CalculatePipeSizeHelper(s, NewImage);
    }
    return 1;
  }
  return 0;
}

void CB_ResizeCheck(const QVariant Check) {
  Settings->SetValue("Resize",Check);
  if (Settings->GetInt("AutomaticPipeSize") && Settings->GetInt("Resize")) {
    if (!CalculatePipeSize())
      Update(ptProcessorPhase_Geometry);
  } else {
    Update(ptProcessorPhase_Geometry);
  }
  MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
}

void CB_ResizeDimensionChoice(const QVariant Choice) {
  Settings->SetValue("ResizeDimension",Choice);
  if (Settings->GetInt("Resize")) {
    if (Settings->GetInt("AutomaticPipeSize")) {
      if (!CalculatePipeSize())
        Update(ptProcessorPhase_Geometry);
    } else {
      Update(ptProcessorPhase_Geometry);
    }
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  }
}

void CB_ResizeScaleInput(const QVariant Value) {
  Settings->SetValue("ResizeScale",Value);
  if (Settings->GetInt("Resize")) {
    if (Settings->GetInt("AutomaticPipeSize")) {
      if (!CalculatePipeSize())
        Update(ptProcessorPhase_Geometry);
    } else {
      Update(ptProcessorPhase_Geometry);
    }
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  }
}

void CB_ResizeFilterChoice(const QVariant Choice) {
  Settings->SetValue("ResizeFilter",Choice);
  if (Settings->GetInt("Resize")) {
    if (Settings->GetInt("AutomaticPipeSize")) {
      if (!CalculatePipeSize())
        Update(ptProcessorPhase_Geometry);
    } else {
      Update(ptProcessorPhase_Geometry);
    }
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  }
}

void CB_AutomaticPipeSizeCheck(const QVariant Check) {
  Settings->SetValue("AutomaticPipeSize",Check);
  if (Settings->GetInt("Resize")) {
    if (Settings->GetInt("AutomaticPipeSize")) {
      if (!CalculatePipeSize())
        Update(ptProcessorPhase_Geometry);
    } else {
      Update(ptProcessorPhase_Geometry);
    }
    MainWindow->UpdateExifInfo(TheProcessor->m_ExifData);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim Levels
//
////////////////////////////////////////////////////////////////////////////////

void CB_LevelsBlackPointInput(const QVariant Value) {
  Settings->SetValue("LevelsBlackPoint",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_LevelsWhitePointInput(const QVariant Value) {
  Settings->SetValue("LevelsWhitePoint",Value);
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim ChannelMixer
//
////////////////////////////////////////////////////////////////////////////////

void CB_ChannelMixerOpenButton() {

  QStringList ChannelMixerFileNames =
    Settings->GetStringList("ChannelMixerFileNames");
  int Index = ChannelMixerFileNames.count();

  QString ChannelMixerFileName = QFileDialog::getOpenFileName(
                        NULL,
                        QObject::tr("Open Channelmixer"),
                        Settings->GetString("ChannelMixersDirectory"),
                        ChannelMixerFilePattern);

  if (0 == ChannelMixerFileName.size() ) {
    return;
  } else {
    QFileInfo PathInfo(ChannelMixerFileName);
    Settings->SetValue("ChannelMixersDirectory",PathInfo.absolutePath());
    ChannelMixerFileNames.append(PathInfo.absoluteFilePath());
  }
  if (ChannelMixer->
      ReadChannelMixer(
                  ChannelMixerFileNames[Index].toAscii().data())) {
    QString ErrorMessage = QObject::tr("Cannot read channelmixer ")
                           + " '"
                           + ChannelMixerFileNames[Index]
                           + "'" ;
    ptMessageBox::warning(MainWindow,
                         QObject::tr("Channelmixer read error"),
                         ErrorMessage);
    // Remove last invalid and return.
    ChannelMixerFileNames.removeLast();
    // Store the change.
    Settings->SetValue("ChannelMixerFileNames",ChannelMixerFileNames);
    return;
  }
  // Store the change.
  Settings->SetValue("ChannelMixerFileNames",ChannelMixerFileNames);

  // Small routine that lets Shortfilename point to the basename.
  QFileInfo PathInfo(ChannelMixerFileNames[Index]);
  QString ShortFileName = PathInfo.baseName().left(18);

  // TODO Protection against double loading ?
  Settings->AddOrReplaceOption("ChannelMixer",
                               ShortFileName,
                               ptChannelMixerChoice_File+Index);
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_File+Index);

  // And process now as if just chosen
  CB_ChannelMixerChoice(QVariant(ptChannelMixerChoice_File+Index));
}

void CB_ChannelMixerSaveButton() {
  QString ChannelMixerFileName = QFileDialog::getSaveFileName(
                       NULL,
                       QObject::tr("Save Channelmixer"),
                       Settings->GetString("ChannelMixersDirectory"),
                       ChannelMixerFilePattern);
  if (0 == ChannelMixerFileName.size() ) return;

  QString Header =
    ";\n"
    "; photivo Channelmixer File\n"
    ";\n"
    "; This curve was written from within photivo\n"
    ";\n";

  bool Success;
  QString Explanation =
    QInputDialog::getText(NULL,
                          QObject::tr("Save Channelmixer"),
                          QObject::tr("Give a description"),
                          QLineEdit::Normal,
                          NULL,
                          &Success);
  if (Success && !Explanation.isEmpty()) {
    Header += "; " + Explanation + "\n;\n";
  }

  ChannelMixer->WriteChannelMixer(ChannelMixerFileName.toAscii().data(),
                                  Header.toAscii().data());
}

void CB_ChannelMixerChoice(const QVariant Choice) {
  Settings->SetValue("ChannelMixer",Choice);
  // If we have a channelmixer, go for reading it.
  if (Choice.toInt() >= ptChannelMixerChoice_File) {
    // At this stage, as we have checked on loading the curves
    // we assume the curve can be read. OK, if user has meanwhile
    // removed it this might go wrong, but then we simply die.
    if (ChannelMixer->
        ReadChannelMixer(
         (Settings->GetStringList("ChannelMixerFileNames"))
           [Choice.toInt()-ptChannelMixerChoice_File].
           toAscii().data())){
      assert(0);
    }
  }

  if (Choice == ptChannelMixerChoice_None) {
    for (short i=0; i<3; i++) {
      for (short j=0; j<3; j++) {
        ChannelMixer->m_Mixer[i][j] = (i==j)?1.0:0.0;
      }
    }
  }

  if (!InStartup) Update(ptProcessorPhase_RGB);
  if (!InStartup) UpdatePreviewImage();
}

void CB_ChannelMixerR2RInput(const QVariant Value) {
  ChannelMixer->m_Mixer[0][0] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerG2RInput(const QVariant Value) {
  ChannelMixer->m_Mixer[0][1] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerB2RInput(const QVariant Value) {
  ChannelMixer->m_Mixer[0][2] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerR2GInput(const QVariant Value) {
  ChannelMixer->m_Mixer[1][0] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerG2GInput(const QVariant Value) {
  ChannelMixer->m_Mixer[1][1] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerB2GInput(const QVariant Value) {
  ChannelMixer->m_Mixer[1][2] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerR2BInput(const QVariant Value) {
  ChannelMixer->m_Mixer[2][0] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerG2BInput(const QVariant Value) {
  ChannelMixer->m_Mixer[2][1] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ChannelMixerB2BInput(const QVariant Value) {
  ChannelMixer->m_Mixer[2][2] = Value.toDouble();
  Settings->SetValue("ChannelMixer",ptChannelMixerChoice_Manual);
  if (Settings->GetInt("ChannelMixer")) {
    Update(ptProcessorPhase_RGB);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim Highlights
//
////////////////////////////////////////////////////////////////////////////////

void CB_HighlightsRInput(const QVariant Value) {
  Settings->SetValue("HighlightsR",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_HighlightsGInput(const QVariant Value) {
  Settings->SetValue("HighlightsG",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_HighlightsBInput(const QVariant Value) {
  Settings->SetValue("HighlightsB",Value);
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim Brightness
//
////////////////////////////////////////////////////////////////////////////////

void CB_ExposureGainInput(const QVariant Value) {
  Settings->SetValue("ExposureGain",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_CatchWhiteInput(const QVariant Value) {
  Settings->SetValue("CatchWhite",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_CatchBlackInput(const QVariant Value) {
  Settings->SetValue("CatchBlack",Value);
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim Exposure
//
////////////////////////////////////////////////////////////////////////////////

void CB_AutoExposureChoice(const QVariant Value) {
  Settings->SetValue("AutoExposure",Value);
  MainWindow->UpdateSettings();
  if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Auto) {
    if(Settings->GetInt("HaveImage") == 1) {
      TheProcessor->m_AutoExposureValue = TheProcessor->CalculateAutoExposure(TheProcessor->m_Image_AfterGeometry);
      Settings->SetValue("Exposure",TheProcessor->m_AutoExposureValue);
      Update(ptProcessorPhase_RGB);
    }
  } else if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Ufraw) {
    Settings->SetValue("Exposure",Settings->GetDouble("ExposureNormalization"));
    Update(ptProcessorPhase_RGB);
  } else if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Zero) {
    Settings->SetValue("Exposure",0.0);
    Update(ptProcessorPhase_RGB);
  }
}

void CB_WhiteFractionInput(const QVariant Value) {
  Settings->SetValue("WhiteFraction",Value);
  if (Settings->GetInt("AutoExposure")) {
    TheProcessor->m_AutoExposureValue = TheProcessor->CalculateAutoExposure(TheProcessor->m_Image_AfterGeometry);
    Settings->SetValue("Exposure",TheProcessor->m_AutoExposureValue);
    Update(ptProcessorPhase_RGB);
  }
}

void CB_WhiteLevelInput(const QVariant Value) {
  Settings->SetValue("WhiteLevel",Value);
  if (Settings->GetInt("AutoExposure")) {
    TheProcessor->m_AutoExposureValue = TheProcessor->CalculateAutoExposure(TheProcessor->m_Image_AfterGeometry);
    Settings->SetValue("Exposure",TheProcessor->m_AutoExposureValue);
    Update(ptProcessorPhase_RGB);
  }
}

void CB_ExposureInput(const QVariant Value) {
  // The Gui element is expressed in EV.
  Settings->SetValue("Exposure",Value);
  Settings->SetValue("AutoExposure",ptAutoExposureMode_Manual);
  Update(ptProcessorPhase_RGB);
}

void CB_ExposureClipModeChoice(const QVariant Value) {
  Settings->SetValue("ExposureClipMode",Value);
  // if (!Settings->ToolIsBlocked(MainWindow->ExposureWidget->parent()->parent()->parent()->objectName()))
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim RGB Reinhard05
//
////////////////////////////////////////////////////////////////////////////////

void CB_Reinhard05Check(const QVariant State) {
  Settings->SetValue("Reinhard05",State);
  Update(ptProcessorPhase_RGB);
}

void CB_Reinhard05BrightnessInput(const QVariant Value) {
  Settings->SetValue("Reinhard05Brightness",Value);
  if (Settings->GetInt("Reinhard05"))
    Update(ptProcessorPhase_RGB);
}

void CB_Reinhard05ChromaInput(const QVariant Value) {
  Settings->SetValue("Reinhard05Chroma",Value);
  if (Settings->GetInt("Reinhard05"))
    Update(ptProcessorPhase_RGB);
}

void CB_Reinhard05LightInput(const QVariant Value) {
  Settings->SetValue("Reinhard05Light",Value);
  if (Settings->GetInt("Reinhard05"))
    Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim RGB Gamma
//
////////////////////////////////////////////////////////////////////////////////

void CB_RGBGammaAmountInput(const QVariant Value) {
  Settings->SetValue("RGBGammaAmount",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_RGBGammaLinearityInput(const QVariant Value) {
  Settings->SetValue("RGBGammaLinearity",Value);
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim RGB Normalization
//
////////////////////////////////////////////////////////////////////////////////

void CB_NormalizationOpacityInput(const QVariant Value) {
  Settings->SetValue("NormalizationOpacity",Value);
  Update(ptProcessorPhase_RGB);
}

//void CB_NormalizationBlackPointInput(const QVariant Value) {
//  Settings->SetValue("NormalizationBlackPoint",Value);
//  if (Settings->GetDouble("NormalizationOpacity")) {
//    Update(ptProcessorPhase_RGB);
//    MainWindow->UpdateSettings();
//    UpdatePreviewImage();
//  }
//}
//
//void CB_NormalizationWhitePointInput(const QVariant Value) {
//  Settings->SetValue("NormalizationWhitePoint",Value);
//  if (Settings->GetDouble("NormalizationOpacity")) {
//    Update(ptProcessorPhase_RGB);
//    MainWindow->UpdateSettings();
//    UpdatePreviewImage();
//  }
//}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim RGB Contrast
//
////////////////////////////////////////////////////////////////////////////////

void CB_RGBContrastAmountInput(const QVariant Value) {
  Settings->SetValue("RGBContrastAmount",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_RGBContrastThresholdInput(const QVariant Value) {
  Settings->SetValue("RGBContrastThreshold",Value);
  if (Settings->GetDouble("RGBContrastAmount")) {
    Update(ptProcessorPhase_RGB);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Partim Intensitytool
//
////////////////////////////////////////////////////////////////////////////////

void CB_VibranceInput(const QVariant Value) {
  Settings->SetValue("Vibrance",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_IntensityRedInput(const QVariant Value) {
  Settings->SetValue("IntensityRed",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_IntensityGreenInput(const QVariant Value) {
  Settings->SetValue("IntensityGreen",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_IntensityBlueInput(const QVariant Value) {
  Settings->SetValue("IntensityBlue",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_ColorEnhanceShadowsInput(const QVariant Value) {
  Settings->SetValue("ColorEnhanceShadows",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_ColorEnhanceHighlightsInput(const QVariant Value) {
  Settings->SetValue("ColorEnhanceHighlights",Value);
  Update(ptProcessorPhase_RGB);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab AND Lab Tab : Curves
//
////////////////////////////////////////////////////////////////////////////////

void CB_CurveOpenButton(const int Channel) {

  QStringList CurveFileNames =
    Settings->GetStringList(CurveFileNamesKeys[Channel]);
  int Index = CurveFileNames.count();

  QString CurveFileName = QFileDialog::getOpenFileName(
                            NULL,
                            QObject::tr("Open Curve"),
                            Settings->GetString("CurvesDirectory"),
                            CurveFilePattern);

  if (0 == CurveFileName.size() ) {
    return;
  } else {
    QFileInfo PathInfo(CurveFileName);
    Settings->SetValue("CurvesDirectory",PathInfo.absolutePath());
    CurveFileNames.append(PathInfo.absoluteFilePath());
  }
  if (!Curve[Channel]) Curve[Channel] = new(ptCurve);
  if (Curve[Channel]-> ReadCurve(CurveFileNames[Index].toAscii().data())) {
    QString ErrorMessage = QObject::tr("Cannot read curve ")
                           + " '"
                           + CurveFileNames[Index]
                           + "'" ;
    ptMessageBox::warning(MainWindow,
                         QObject::tr("Curve read error"),
                         ErrorMessage);
    // Remove last invalid and return.
    CurveFileNames.removeLast();
    // Write it back to our settings.
    Settings->SetValue(CurveFileNamesKeys[Channel],CurveFileNames);
    return;
  }
  if (Curve[Channel]->m_IntendedChannel != Channel) {
    const QString IntendedChannel[17] = {"RGB","R","G","B","L","a","b",
      "Saturation","Base","After gamma","L by hue","Texture",
                                         "Shadows / Highlights","Denoise","Denoise 2","Hue","Outline"};
    QString Message = QObject::tr("This curve is meant for channel ") +
                        IntendedChannel[Curve[Channel]->m_IntendedChannel] +
                        QObject::tr(". Continue anyway ?");
    if (QMessageBox::No == ptMessageBox::question(
                   MainWindow,
                   QObject::tr("Incompatible curve"),
                   Message,
                   QMessageBox::No|QMessageBox::Yes)) {
      // Remove last invalid and return.
      CurveFileNames.removeLast();
      // Write it back to our settings.
      Settings->SetValue(CurveFileNamesKeys[Channel],CurveFileNames);
      return;
    }
  }

  // Remember the maybe changed CurveFileNames list.
  Settings->SetValue(CurveFileNamesKeys[Channel],CurveFileNames);

  // At this moment we have a valid CurveFileName. We have to add it now
  // to the list of possible selections.

  // Small routine that lets Shortfilename point to the basename.
  QFileInfo PathInfo(CurveFileNames[Index]);
  QString ShortFileName = PathInfo.baseName().left(18);
  Settings->AddOrReplaceOption(CurveKeys[Channel],
                               ShortFileName,
                               ptCurveChoice_File+Index);
  Settings->SetValue(CurveKeys[Channel],ptCurveChoice_File+Index);

  // And process now as if just chosen
  CB_CurveChoice(Channel,ptCurveChoice_File+Index);
}

void CB_CurveRGBOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_RGB);
}

void CB_CurveROpenButton() {
  CB_CurveOpenButton(ptCurveChannel_R);
}

void CB_CurveGOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_G);
}

void CB_CurveBOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_B);
}

void CB_CurveLOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_L);
}

void CB_CurveaOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_a);
}

void CB_CurvebOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_b);
}

void CB_CurveOutlineOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Outline);
}

void CB_CurveLByHueOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_LByHue);
}

void CB_CurveHueOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Hue);
}

void CB_CurveTextureOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Texture);
}

void CB_CurveShadowsHighlightsOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_ShadowsHighlights);
}

void CB_CurveDenoiseOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Denoise);
}

void CB_CurveDenoise2OpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Denoise2);
}

void CB_CurveSaturationOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Saturation);
}

void CB_BaseCurveOpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Base);
}

void CB_BaseCurve2OpenButton() {
  CB_CurveOpenButton(ptCurveChannel_Base2);
}

void CB_CurveSaveButton(const int Channel) {
  QString CurveFileName = QFileDialog::getSaveFileName(
                            NULL,
                            QObject::tr("Save Curve"),
                            Settings->GetString("CurvesDirectory"),
                            CurveFilePattern);
  if (0 == CurveFileName.size() ) return;

  QString Header =
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This curve was written from within photivo\n"
    ";\n";

  bool Success;
  QString Explanation =
    QInputDialog::getText(NULL,
                          QObject::tr("Save Curve"),
                          QObject::tr("Give a description"),
                          QLineEdit::Normal,
                          NULL,
                          &Success);
  if (Success && !Explanation.isEmpty()) {
    Header += "; " + Explanation + "\n;\n";
  }

  Curve[Channel]->WriteCurve(CurveFileName.toAscii().data(),
                             Header.toAscii().data());
}

void CB_CurveRGBSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_RGB);
}

void CB_CurveRSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_R);
}

void CB_CurveGSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_G);
}

void CB_CurveBSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_B);
}

void CB_CurveLSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_L);
}

void CB_CurveaSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_a);
}

void CB_CurvebSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_b);
}

void CB_CurveOutlineSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Outline);
}

void CB_CurveLByHueSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_LByHue);
}

void CB_CurveHueSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Hue);
}

void CB_CurveTextureSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Texture);
}

void CB_CurveShadowsHighlightsSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_ShadowsHighlights);
}

void CB_CurveDenoiseSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Denoise);
}

void CB_CurveDenoise2SaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Denoise2);
}

void CB_CurveSaturationSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Saturation);
}

void CB_BaseCurveSaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Base);
}

void CB_BaseCurve2SaveButton() {
  CB_CurveSaveButton(ptCurveChannel_Base2);
}
void CB_CurveChoice(const int Channel, const int Choice) {
  short PreviousActiveState = Settings->ToolIsActive(CurveToolNameKeys[Channel]);

  // Save the old Curve
  if (Settings->GetInt(CurveKeys.at(Channel))==ptCurveChoice_Manual) {
    if (!BackupCurve[Channel]) BackupCurve[Channel] = new ptCurve();
    BackupCurve[Channel]->Set(Curve[Channel]);
  }

  // Restore the saved curve
  if (Settings->GetInt(CurveKeys.at(Channel))!=ptCurveChoice_Manual &&
      Choice == ptCurveChoice_Manual) {
    if (BackupCurve[Channel])
      Curve[Channel]->Set(BackupCurve[Channel]);
    else
      Curve[Channel]->SetNullCurve(Channel);
  }

  // Set the newly choosen curve.
  Settings->SetValue(CurveKeys[Channel],Choice);

  QStringList CurveFileNames =
    Settings->GetStringList(CurveFileNamesKeys[Channel]);

  // If we have a curve, go for reading it.
  if (Choice >= ptCurveChoice_File) {
    if (!Curve[Channel]) Curve[Channel] = new(ptCurve);
    // At this stage, as we have checked on loading the curves
    // we assume the curve can be read. OK, if user has meanwhile
    // removed it this might go wrong, but then we simply die.
    if (Curve[Channel]->
        ReadCurve(CurveFileNames[Choice-ptCurveChoice_File].
                  toAscii().data())){
      assert(0);
    }
  }

  if (Choice == ptCurveChoice_None) {
    Curve[Channel]->SetNullCurve(Channel);
  }

  // Update the View.
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  if (Settings->GetInt("JobMode") == 0)
    CurveWindow[Channel]->UpdateView(Curve[Channel]);

  CB_CurveWindowRecalc(Channel, PreviousActiveState);
}

void CB_CurveRGBChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_RGB,Choice.toInt());
}

void CB_CurveRChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_R,Choice.toInt());
}

void CB_CurveGChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_G,Choice.toInt());
}

void CB_CurveBChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_B,Choice.toInt());
}

void CB_CurveLChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_L,Choice.toInt());
}

void CB_CurveLaChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_a,Choice.toInt());
}

void CB_CurveLbChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_b,Choice.toInt());
}

void CB_CurveOutlineChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Outline,Choice.toInt());
}

void CB_CurveLByHueChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_LByHue,Choice.toInt());
}

void CB_CurveHueChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Hue,Choice.toInt());
}

void CB_CurveTextureChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Texture,Choice.toInt());
}

void CB_CurveShadowsHighlightsChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_ShadowsHighlights,Choice.toInt());
}

void CB_CurveDenoiseChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Denoise,Choice.toInt());
}

void CB_CurveDenoise2Choice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Denoise2,Choice.toInt());
}

void CB_CurveSaturationChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Saturation,Choice.toInt());
}

void CB_BaseCurveChoice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Base,Choice.toInt());
}

void CB_BaseCurve2Choice(const QVariant Choice) {
  CB_CurveChoice(ptCurveChannel_Base2,Choice.toInt());
}

void CB_CurveWindowRecalc(const short Channel, const short ForceUpdate /* =0 */) {
  if (!InStartup) {
    short NewActiveState = Settings->ToolIsActive(CurveToolNameKeys[Channel]);
    MainWindow->m_GroupBox->value(CurveToolNameKeys[Channel])->SetActive(NewActiveState);
    // Run the graphical pipe according to a changed curve.
    if (ForceUpdate || NewActiveState) {
      Update(CurveToolNameKeys[Channel]);
    }
  }
}

void CB_CurveWindowManuallyChanged(const short Channel) {
  // Combobox and curve choice has to be adapted to manual.
  Settings->SetValue(CurveKeys[Channel],ptCurveChoice_Manual);
  // Run the graphical pipe according to a changed curve.
  CB_CurveWindowRecalc(Channel);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Local Exposure
//
////////////////////////////////////////////////////////////////////////////////

void CB_LMHLightRecovery1MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LMHLightRecovery1MaskType",Choice);
  Update(ptProcessorPhase_RGB);
}

void CB_LMHLightRecovery1AmountInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery1Amount",Value);
  if (Settings->GetInt("LMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery1LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LMHLightRecovery1UpperLimit")-0.01));
  if (Settings->GetInt("LMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery1UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LMHLightRecovery1LowerLimit")+0.01));
  if (Settings->GetInt("LMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery1SoftnessInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery1Softness",Value);
  if (Settings->GetInt("LMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery2MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LMHLightRecovery2MaskType",Choice);
  Update(ptProcessorPhase_RGB);
}

void CB_LMHLightRecovery2AmountInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery2Amount",Value);
  if (Settings->GetInt("LMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery2LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LMHLightRecovery2UpperLimit")-0.01));
  if (Settings->GetInt("LMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery2UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LMHLightRecovery2LowerLimit")+0.01));
  if (Settings->GetInt("LMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_LMHLightRecovery2SoftnessInput(const QVariant Value) {
  Settings->SetValue("LMHLightRecovery2Softness",Value);
  if (Settings->GetInt("LMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Texturecontrast
//
////////////////////////////////////////////////////////////////////////////////

void CB_RGBTextureContrastAmountInput(const QVariant Value) {
  Settings->SetValue("RGBTextureContrastAmount",Value);
  Update(ptProcessorPhase_RGB);
}

void CB_RGBTextureContrastThresholdInput(const QVariant Value) {
  Settings->SetValue("RGBTextureContrastThreshold",Value);
  if (Settings->GetDouble("RGBTextureContrastAmount")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_RGBTextureContrastOpacityInput(const QVariant Value) {
  Settings->SetValue("RGBTextureContrastOpacity",Value);
  if (Settings->GetDouble("RGBTextureContrastAmount")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_RGBTextureContrastSoftnessInput(const QVariant Value) {
  Settings->SetValue("RGBTextureContrastSoftness",Value);
  if (Settings->GetDouble("RGBTextureContrastAmount")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_RGBTextureContrastEdgeControlInput(const QVariant Value) {
  Settings->SetValue("RGBTextureContrastEdgeControl",Value);
  if (Settings->GetDouble("RGBTextureContrastAmount")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_RGBTextureContrastMaskingInput(const QVariant Value) {
  Settings->SetValue("RGBTextureContrastMasking",Value);
  if (Settings->GetDouble("RGBTextureContrastAmount")) {
    Update(ptProcessorPhase_RGB);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the RGB Tab
// Microcontrast
//
////////////////////////////////////////////////////////////////////////////////

void CB_Microcontrast1MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("Microcontrast1MaskType",Choice);
  Update(ptProcessorPhase_RGB);
}

void CB_Microcontrast1RadiusInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1Radius",Value);
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast1AmountInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1Amount",Value);
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast1OpacityInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1Opacity",Value);
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast1HaloControlInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1HaloControl",Value);
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("Microcontrast1UpperLimit")-0.01));
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("Microcontrast1LowerLimit")+0.01));
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast1SoftnessInput(const QVariant Value) {
  Settings->SetValue("Microcontrast1Softness",Value);
  if (Settings->GetInt("Microcontrast1MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("Microcontrast2MaskType",Choice);
  Update(ptProcessorPhase_RGB);
}

void CB_Microcontrast2RadiusInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2Radius",Value);
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2AmountInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2Amount",Value);
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2OpacityInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2Opacity",Value);
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2HaloControlInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2HaloControl",Value);
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("Microcontrast2UpperLimit")-0.01));
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("Microcontrast2LowerLimit")+0.01));
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

void CB_Microcontrast2SoftnessInput(const QVariant Value) {
  Settings->SetValue("Microcontrast2Softness",Value);
  if (Settings->GetInt("Microcontrast2MaskType")) {
    Update(ptProcessorPhase_RGB);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabCC Tab
// LAB Transform
//
////////////////////////////////////////////////////////////////////////////////

void CB_LABTransformChoice(const QVariant Choice) {
  Settings->SetValue("LABTransform",Choice);
  Update(ptProcessorPhase_LabCC);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabCC Tab
// Shadows Highlights
//
////////////////////////////////////////////////////////////////////////////////

void CB_ShadowsHighlightsFineInput(const QVariant Value) {
  Settings->SetValue("ShadowsHighlightsFine",Value);
  Update(ptProcessorPhase_LabCC);
}

void CB_ShadowsHighlightsCoarseInput(const QVariant Value) {
  Settings->SetValue("ShadowsHighlightsCoarse",Value);
  Update(ptProcessorPhase_LabCC);
}

void CB_ShadowsHighlightsRadiusInput(const QVariant Value) {
  Settings->SetValue("ShadowsHighlightsRadius",Value);
  if (Settings->GetDouble("ShadowsHighlightsFine") ||
      Settings->GetDouble("ShadowsHighlightsCoarse") ||
      Settings->GetInt("CurveShadowsHighlights")) {
    Update(ptProcessorPhase_LabCC);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabCC Tab
// Dynamic Range Compression
//
////////////////////////////////////////////////////////////////////////////////

void CB_DRCBetaInput(const QVariant Value) {
  Settings->SetValue("DRCBeta",Value);
  Update(ptProcessorPhase_LabCC);
}

void CB_DRCAlphaInput(const QVariant Value) {
  Settings->SetValue("DRCAlpha",Value);
  if (Settings->GetDouble("DRCBeta")!=1.0) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_DRCColorInput(const QVariant Value) {
  Settings->SetValue("DRCColor",Value);
  if (Settings->GetDouble("DRCBeta")!=1.0) {
    Update(ptProcessorPhase_LabCC);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabCC Tab
// Local Exposure
//
////////////////////////////////////////////////////////////////////////////////

void CB_LabLMHLightRecovery1MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LabLMHLightRecovery1MaskType",Choice);
  Update(ptProcessorPhase_LabCC);
}

void CB_LabLMHLightRecovery1AmountInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery1Amount",Value);
  if (Settings->GetInt("LabLMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery1LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LabLMHLightRecovery1UpperLimit")-0.01));
  if (Settings->GetInt("LabLMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery1UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LabLMHLightRecovery1LowerLimit")+0.01));
  if (Settings->GetInt("LabLMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery1SoftnessInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery1Softness",Value);
  if (Settings->GetInt("LabLMHLightRecovery1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery2MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LabLMHLightRecovery2MaskType",Choice);
  Update(ptProcessorPhase_LabCC);
}

void CB_LabLMHLightRecovery2AmountInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery2Amount",Value);
  if (Settings->GetInt("LabLMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery2LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LabLMHLightRecovery2UpperLimit")-0.01));
  if (Settings->GetInt("LabLMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery2UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LabLMHLightRecovery2LowerLimit")+0.01));
  if (Settings->GetInt("LabLMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabLMHLightRecovery2SoftnessInput(const QVariant Value) {
  Settings->SetValue("LabLMHLightRecovery2Softness",Value);
  if (Settings->GetInt("LabLMHLightRecovery2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LABCC Tab
// Texturecontrast
//
////////////////////////////////////////////////////////////////////////////////

void CB_TextureContrast1AmountInput(const QVariant Value) {
  Settings->SetValue("TextureContrast1Amount",Value);
  Update(ptProcessorPhase_LabCC);
}

void CB_TextureContrast1ThresholdInput(const QVariant Value) {
  Settings->SetValue("TextureContrast1Threshold",Value);
  if (Settings->GetDouble("TextureContrast1Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast1OpacityInput(const QVariant Value) {
  Settings->SetValue("TextureContrast1Opacity",Value);
  if (Settings->GetDouble("TextureContrast1Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast1SoftnessInput(const QVariant Value) {
  Settings->SetValue("TextureContrast1Softness",Value);
  if (Settings->GetDouble("TextureContrast1Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast1EdgeControlInput(const QVariant Value) {
  Settings->SetValue("TextureContrast1EdgeControl",Value);
  if (Settings->GetDouble("TextureContrast1Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast1MaskingInput(const QVariant Value) {
  Settings->SetValue("TextureContrast1Masking",Value);
  if (Settings->GetDouble("TextureContrast1Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast2AmountInput(const QVariant Value) {
  Settings->SetValue("TextureContrast2Amount",Value);
  Update(ptProcessorPhase_LabCC);
}

void CB_TextureContrast2ThresholdInput(const QVariant Value) {
  Settings->SetValue("TextureContrast2Threshold",Value);
  if (Settings->GetDouble("TextureContrast2Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast2OpacityInput(const QVariant Value) {
  Settings->SetValue("TextureContrast2Opacity",Value);
  if (Settings->GetDouble("TextureContrast2Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast2SoftnessInput(const QVariant Value) {
  Settings->SetValue("TextureContrast2Softness",Value);
  if (Settings->GetDouble("TextureContrast2Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast2EdgeControlInput(const QVariant Value) {
  Settings->SetValue("TextureContrast2EdgeControl",Value);
  if (Settings->GetDouble("TextureContrast2Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_TextureContrast2MaskingInput(const QVariant Value) {
  Settings->SetValue("TextureContrast2Masking",Value);
  if (Settings->GetDouble("TextureContrast2Amount")) {
    Update(ptProcessorPhase_LabCC);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LABCC Tab
// Microcontrast
//
////////////////////////////////////////////////////////////////////////////////

void CB_LabMicrocontrast1MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LabMicrocontrast1MaskType",Choice);
  Update(ptProcessorPhase_LabCC);
}

void CB_LabMicrocontrast1RadiusInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1Radius",Value);
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast1AmountInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1Amount",Value);
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast1OpacityInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1Opacity",Value);
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast1HaloControlInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1HaloControl",Value);
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LabMicrocontrast1UpperLimit")-0.01));
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LabMicrocontrast1LowerLimit")+0.01));
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast1SoftnessInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast1Softness",Value);
  if (Settings->GetInt("LabMicrocontrast1MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LabMicrocontrast2MaskType",Choice);
  Update(ptProcessorPhase_LabCC);
}

void CB_LabMicrocontrast2RadiusInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2Radius",Value);
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2AmountInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2Amount",Value);
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2OpacityInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2Opacity",Value);
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2HaloControlInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2HaloControl",Value);
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LabMicrocontrast2UpperLimit")-0.01));
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LabMicrocontrast2LowerLimit")+0.01));
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

void CB_LabMicrocontrast2SoftnessInput(const QVariant Value) {
  Settings->SetValue("LabMicrocontrast2Softness",Value);
  if (Settings->GetInt("LabMicrocontrast2MaskType")) {
    Update(ptProcessorPhase_LabCC);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Edge avoiding wavelets
//
////////////////////////////////////////////////////////////////////////////////

void CB_EAWMasterInput(const QVariant Value) {
  if (Value.toDouble() == 0) { // Reset
    Settings->SetValue("EAWLevel1",0.0);
    Settings->SetValue("EAWLevel2",0.0);
    Settings->SetValue("EAWLevel3",0.0);
    Settings->SetValue("EAWLevel4",0.0);
    Settings->SetValue("EAWLevel5",0.0);
    Settings->SetValue("EAWLevel6",0.0);
  } else if (fabs(Value.toDouble()) <= 0.1) {
    Settings->SetValue("EAWLevel1",Value);
    Settings->SetValue("EAWLevel2",0.0);
    Settings->SetValue("EAWLevel3",0.0);
    Settings->SetValue("EAWLevel4",0.0);
    Settings->SetValue("EAWLevel5",0.0);
    Settings->SetValue("EAWLevel6",0.0);
  } else if (fabs(Value.toDouble()) <= 0.2) {
    Settings->SetValue("EAWLevel1",Value);
    Settings->SetValue("EAWLevel2",Value.toDouble()/2);
    Settings->SetValue("EAWLevel3",0.0);
    Settings->SetValue("EAWLevel4",0.0);
    Settings->SetValue("EAWLevel5",0.0);
    Settings->SetValue("EAWLevel6",0.0);
  } else if (fabs(Value.toDouble()) <= 0.3) {
    Settings->SetValue("EAWLevel1",Value);
    Settings->SetValue("EAWLevel2",Value.toDouble()/3*2);
    Settings->SetValue("EAWLevel3",Value.toDouble()/3);
    Settings->SetValue("EAWLevel4",0.0);
    Settings->SetValue("EAWLevel5",0.0);
    Settings->SetValue("EAWLevel6",0.0);
  } else if (fabs(Value.toDouble()) <= 0.4) {
    Settings->SetValue("EAWLevel1",Value);
    Settings->SetValue("EAWLevel2",Value.toDouble()/4*3);
    Settings->SetValue("EAWLevel3",Value.toDouble()/4*2);
    Settings->SetValue("EAWLevel4",Value.toDouble()/4);
    Settings->SetValue("EAWLevel5",0.0);
    Settings->SetValue("EAWLevel6",0.0);
  } else if (fabs(Value.toDouble()) <= 0.5) {
    Settings->SetValue("EAWLevel1",Value);
    Settings->SetValue("EAWLevel2",Value.toDouble()/5*4);
    Settings->SetValue("EAWLevel3",Value.toDouble()/5*3);
    Settings->SetValue("EAWLevel4",Value.toDouble()/5*2);
    Settings->SetValue("EAWLevel5",Value.toDouble()/5);
    Settings->SetValue("EAWLevel6",0.0);
  } else {
    Settings->SetValue("EAWLevel1",Value);
    Settings->SetValue("EAWLevel2",Value.toDouble()/6*5);
    Settings->SetValue("EAWLevel3",Value.toDouble()/6*4);
    Settings->SetValue("EAWLevel4",Value.toDouble()/6*3);
    Settings->SetValue("EAWLevel5",Value.toDouble()/6*2);
    Settings->SetValue("EAWLevel6",Value.toDouble()/6);
  }
  Update(ptProcessorPhase_LabSN);
}

void CB_EAWLevel1Input(const QVariant Value) {
  Settings->SetValue("EAWLevel1",Value);
  Settings->SetValue("EAWMaster",0.0);
  Update(ptProcessorPhase_LabSN);
}

void CB_EAWLevel2Input(const QVariant Value) {
  Settings->SetValue("EAWLevel2",Value);
  Settings->SetValue("EAWMaster",0.0);
  Update(ptProcessorPhase_LabSN);
}

void CB_EAWLevel3Input(const QVariant Value) {
  Settings->SetValue("EAWLevel3",Value);
  Settings->SetValue("EAWMaster",0.0);
  Update(ptProcessorPhase_LabSN);
}

void CB_EAWLevel4Input(const QVariant Value) {
  Settings->SetValue("EAWLevel4",Value);
  Settings->SetValue("EAWMaster",0.0);
  Update(ptProcessorPhase_LabSN);
}

void CB_EAWLevel5Input(const QVariant Value) {
  Settings->SetValue("EAWLevel5",Value);
  Settings->SetValue("EAWMaster",0.0);
  Update(ptProcessorPhase_LabSN);
}

void CB_EAWLevel6Input(const QVariant Value) {
  Settings->SetValue("EAWLevel6",Value);
  Settings->SetValue("EAWMaster",0.0);
  Update(ptProcessorPhase_LabSN);
}


////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim GreyCStoration
//
////////////////////////////////////////////////////////////////////////////////

void CB_GREYCLabChoice(const QVariant State) {
  Settings->SetValue("GREYCLab",State);
  Update(ptProcessorPhase_LabSN);
}

void CB_GREYCLabMaskTypeChoice(const QVariant State) {
  Settings->SetValue("GREYCLabMaskType",State);
  if (Settings->GetInt("GREYCLab")>1) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabFastCheck(const QVariant State) {
  Settings->SetValue("GREYCLabFast",State);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabInterpolationChoice(const QVariant Choice) {
  Settings->SetValue("GREYCLabInterpolation",Choice);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabOpacityInput(const QVariant Value) {
  Settings->SetValue("GREYCLabOpacity",Value);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabAmplitudeInput(const QVariant Value) {
  Settings->SetValue("GREYCLabAmplitude",Value);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabIterationsInput(const QVariant Value) {
  Settings->SetValue("GREYCLabIterations",Value);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabSharpnessInput(const QVariant Value) {
  Settings->SetValue("GREYCLabSharpness",Value);
  if (Settings->GetInt("GREYCLab")>1) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabAnisotropyInput(const QVariant Value) {
  Settings->SetValue("GREYCLabAnisotropy",Value);
  if (Settings->GetInt("GREYCLab")>1) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabAlphaInput(const QVariant Value) {
  Settings->SetValue("GREYCLabAlpha",Value);
  if (Settings->GetInt("GREYCLab")>1) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabSigmaInput(const QVariant Value) {
  Settings->SetValue("GREYCLabSigma",Value);
  if (Settings->GetInt("GREYCLab")>1) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabGaussPrecisionInput(const QVariant Value) {
  Settings->SetValue("GREYCLabGaussPrecision",Value);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabdlInput(const QVariant Value) {
  Settings->SetValue("GREYCLabdl",Value);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_GREYCLabdaInput(const QVariant Value) {
  Settings->SetValue("GREYCLabda",Value);
  if (Settings->GetInt("GREYCLab")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Defringe
//
////////////////////////////////////////////////////////////////////////////////

void CB_DefringeRadiusInput(const QVariant Value) {
  Settings->SetValue("DefringeRadius",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_DefringeThresholdInput(const QVariant Value) {
  Settings->SetValue("DefringeThreshold",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeShiftInput(const QVariant Value) {
  Settings->SetValue("DefringeShift",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeColor1Check(const QVariant Value) {
  Settings->SetValue("DefringeColor1",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeColor2Check(const QVariant Value) {
  Settings->SetValue("DefringeColor2",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeColor3Check(const QVariant Value) {
  Settings->SetValue("DefringeColor3",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeColor4Check(const QVariant Value) {
  Settings->SetValue("DefringeColor4",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeColor5Check(const QVariant Value) {
  Settings->SetValue("DefringeColor5",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

void CB_DefringeColor6Check(const QVariant Value) {
  Settings->SetValue("DefringeColor6",Value);
  if (Settings->GetDouble("DefringeRadius"))
    Update(ptProcessorPhase_LabSN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Pyramid Denoise
//
////////////////////////////////////////////////////////////////////////////////

void CB_PyrDenoiseLAmountInput(const QVariant Value) {
  Settings->SetValue("PyrDenoiseLAmount",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_PyrDenoiseABAmountInput(const QVariant Value) {
  Settings->SetValue("PyrDenoiseABAmount",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_PyrDenoiseGammaInput(const QVariant Value) {
  Settings->SetValue("PyrDenoiseGamma",Value);
  if (Settings->GetInt("PyrDenoiseLAmount")||
      Settings->GetInt("PyrDenoiseABAmount")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_PyrDenoiseLevelsInput(const QVariant Value) {
  Settings->SetValue("PyrDenoiseLevels",Value);
  if (Settings->GetInt("PyrDenoiseLAmount")||
      Settings->GetInt("PyrDenoiseABAmount")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Bilateral Denoise
//
////////////////////////////////////////////////////////////////////////////////

void CB_BilateralLOpacityInput(const QVariant Value) {
  Settings->SetValue("BilateralLOpacity",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_BilateralLUseMaskInput(const QVariant Value) {
  Settings->SetValue("BilateralLUseMask",Value);
  if (Settings->GetDouble("BilateralLOpacity")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_BilateralLSigmaSInput(const QVariant Value) {
  Settings->SetValue("BilateralLSigmaS",Value);
  if (Settings->GetDouble("BilateralLOpacity")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_BilateralLSigmaRInput(const QVariant Value) {
  Settings->SetValue("BilateralLSigmaR",Value);
  if (Settings->GetDouble("BilateralLOpacity")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_BilateralASigmaRInput(const QVariant Value) {
  Settings->SetValue("BilateralASigmaR",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_BilateralASigmaSInput(const QVariant Value) {
  Settings->SetValue("BilateralASigmaS",Value);
  if (Settings->GetDouble("BilateralASigmaR")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_BilateralBSigmaRInput(const QVariant Value) {
  Settings->SetValue("BilateralBSigmaR",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_BilateralBSigmaSInput(const QVariant Value) {
  Settings->SetValue("BilateralBSigmaS",Value);
  if (Settings->GetDouble("BilateralBSigmaR")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Wavelet Denoise
//
////////////////////////////////////////////////////////////////////////////////

void CB_WaveletDenoiseLInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseL",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_WaveletDenoiseLSoftnessInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseLSoftness",Value);
  if (Settings->GetDouble("WaveletDenoiseL")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WaveletDenoiseLSharpnessInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseLSharpness",Value);
  if (Settings->GetDouble("WaveletDenoiseL")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WaveletDenoiseLAnisotropyInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseLAnisotropy",Value);
  if (Settings->GetDouble("WaveletDenoiseL")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WaveletDenoiseLAlphaInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseLAlpha",Value);
  if (Settings->GetDouble("WaveletDenoiseL")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WaveletDenoiseLSigmaInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseLSigma",Value);
  if (Settings->GetDouble("WaveletDenoiseL")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WaveletDenoiseAInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseA",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_WaveletDenoiseASoftnessInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseASoftness",Value);
  if (Settings->GetDouble("WaveletDenoiseA")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WaveletDenoiseBInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseB",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_WaveletDenoiseBSoftnessInput(const QVariant Value) {
  Settings->SetValue("WaveletDenoiseBSoftness",Value);
  if (Settings->GetDouble("WaveletDenoiseB")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Detail Curve
//
////////////////////////////////////////////////////////////////////////////////

void CB_DenoiseCurveSigmaRInput(const QVariant Value) {
  Settings->SetValue("DenoiseCurveSigmaR",Value);
  if (Settings->GetInt("CurveDenoise2")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_DenoiseCurveSigmaSInput(const QVariant Value) {
  Settings->SetValue("DenoiseCurveSigmaS",Value);
  if (Settings->GetInt("CurveDenoise2")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Detail Curve
//
////////////////////////////////////////////////////////////////////////////////

void CB_DetailCurveScalingInput(const QVariant Value) {
  Settings->SetValue("DetailCurveScaling",Value);
  if (Settings->GetInt("CurveDenoise")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_DetailCurveWeightInput(const QVariant Value) {
  Settings->SetValue("DetailCurveWeight",Value);
  if (Settings->GetInt("CurveDenoise")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_DetailCurveHotpixelInput(const QVariant Value) {
  Settings->SetValue("DetailCurveHotpixel",Value);
  Update(ptProcessorPhase_LabSN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Gradient Sharpen
//
////////////////////////////////////////////////////////////////////////////////

void CB_GradientSharpenPassesInput(const QVariant Value) {
  Settings->SetValue("GradientSharpenPasses",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_GradientSharpenStrengthInput(const QVariant Value) {
  Settings->SetValue("GradientSharpenStrength",Value);
  if (Settings->GetInt("GradientSharpenPasses")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_MLMicroContrastStrengthInput(const QVariant Value) {
  Settings->SetValue("MLMicroContrastStrength",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_MLMicroContrastScalingInput(const QVariant Value) {
  Settings->SetValue("MLMicroContrastScaling",Value);
  if (Settings->GetDouble("MLMicroContrastStrength")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_MLMicroContrastWeightInput(const QVariant Value) {
  Settings->SetValue("MLMicroContrastWeight",Value);
  if (Settings->GetDouble("MLMicroContrastStrength")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_LabHotpixelInput(const QVariant Value) {
  Settings->SetValue("LabHotpixel",Value);
  Update(ptProcessorPhase_LabSN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Wiener Filter Sharpen
//
////////////////////////////////////////////////////////////////////////////////

void CB_WienerFilterCheck(const QVariant State) {
  Settings->SetValue("WienerFilter",State);
  Update(ptProcessorPhase_LabSN);
}

void CB_WienerFilterAmountInput(const QVariant Value) {
  Settings->SetValue("WienerFilterAmount",Value);
  if (Settings->GetInt("WienerFilter")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WienerFilterUseEdgeMaskCheck(const QVariant State) {
  Settings->SetValue("WienerFilterUseEdgeMask",State);
  if (Settings->GetInt("WienerFilter")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WienerFilterGaussianInput(const QVariant Value) {
  Settings->SetValue("WienerFilterGaussian",Value);
  if (Settings->GetInt("WienerFilter")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WienerFilterBoxInput(const QVariant Value) {
  Settings->SetValue("WienerFilterBox",Value);
  if (Settings->GetInt("WienerFilter")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_WienerFilterLensBlurInput(const QVariant Value) {
  Settings->SetValue("WienerFilterLensBlur",Value);
  if (Settings->GetInt("WienerFilter")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Inverse Diffusion Sharpen
//
////////////////////////////////////////////////////////////////////////////////

void CB_InverseDiffusionIterationsInput(const QVariant Value) {
  Settings->SetValue("InverseDiffusionIterations",Value);
  Update(ptProcessorPhase_LabSN);
}

void CB_InverseDiffusionAmplitudeInput(const QVariant Value) {
  Settings->SetValue("InverseDiffusionAmplitude",Value);
  if (Settings->GetInt("InverseDiffusionIterations")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_InverseDiffusionUseEdgeMaskCheck(const QVariant State) {
  Settings->SetValue("InverseDiffusionUseEdgeMask",State);
  if (Settings->GetInt("InverseDiffusionIterations")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim USM
//
////////////////////////////////////////////////////////////////////////////////

void CB_USMChoice(const QVariant Choice) {
  Settings->SetValue("USM",Choice);
  Update(ptProcessorPhase_LabSN);
}

void CB_USMRadiusInput(const QVariant Value) {
  Settings->SetValue("USMRadius",Value);
  if (Settings->GetInt("USM")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}


void CB_USMAmountInput(const QVariant Value) {
  Settings->SetValue("USMAmount",Value);
  if (Settings->GetInt("USM")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_USMThresholdInput(const QVariant Value) {
  Settings->SetValue("USMThreshold",Value);
  if (Settings->GetInt("USM")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Highpass
//
////////////////////////////////////////////////////////////////////////////////

void CB_HighpassChoice(const QVariant Choice) {
  Settings->SetValue("Highpass",Choice);
  Update(ptProcessorPhase_LabSN);
}

void CB_HighpassRadiusInput(const QVariant Value) {
  Settings->SetValue("HighpassRadius",Value);
  if (Settings->GetInt("Highpass")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_HighpassAmountInput(const QVariant Value) {
  Settings->SetValue("HighpassAmount",Value);
  if (Settings->GetInt("Highpass")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_HighpassDenoiseInput(const QVariant Value) {
  Settings->SetValue("HighpassDenoise",Value);
  if (Settings->GetInt("Highpass")==2) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim Grain
//
////////////////////////////////////////////////////////////////////////////////

void CB_Grain1MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("Grain1MaskType",Choice);
  Update(ptProcessorPhase_LabSN);
}

void CB_Grain1ModeChoice(const QVariant Choice) {
  Settings->SetValue("Grain1Mode",Choice);
  if (Settings->GetInt("Grain1MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain1StrengthInput(const QVariant Value) {
  Settings->SetValue("Grain1Strength",Value);
  if (Settings->GetInt("Grain1MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain1RadiusInput(const QVariant Value) {
  Settings->SetValue("Grain1Radius",Value);
  if (Settings->GetInt("Grain1MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain1OpacityInput(const QVariant Value) {
  Settings->SetValue("Grain1Opacity",Value);
  if (Settings->GetInt("Grain1MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("Grain1LowerLimit",Value);
  if (Settings->GetInt("Grain1MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("Grain1UpperLimit",Value);
  if (Settings->GetInt("Grain1MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain2MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("Grain2MaskType",Choice);
  Update(ptProcessorPhase_LabSN);
}

void CB_Grain2ModeChoice(const QVariant Choice) {
  Settings->SetValue("Grain2Mode",Choice);
  if (Settings->GetInt("Grain2MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain2StrengthInput(const QVariant Value) {
  Settings->SetValue("Grain2Strength",Value);
  if (Settings->GetInt("Grain2MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain2RadiusInput(const QVariant Value) {
  Settings->SetValue("Grain2Radius",Value);
  if (Settings->GetInt("Grain2MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain2OpacityInput(const QVariant Value) {
  Settings->SetValue("Grain2Opacity",Value);
  if (Settings->GetInt("Grain2MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("Grain2LowerLimit",Value);
  if (Settings->GetInt("Grain2MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

void CB_Grain2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("Grain2UpperLimit",Value);
  if (Settings->GetInt("Grain2MaskType")) {
    Update(ptProcessorPhase_LabSN);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabSN Tab
// Partim ViewLAB
//
////////////////////////////////////////////////////////////////////////////////

void CB_ViewLABChoice(const QVariant Choice) {
  Settings->SetValue("ViewLAB",Choice);
  Update(ptProcessorPhase_LabSN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LABEyeCandy Tab
// Colorcontrast
//
////////////////////////////////////////////////////////////////////////////////

void CB_ColorcontrastRadiusInput(const QVariant Value) {
  Settings->SetValue("ColorcontrastRadius",Value);
  if (Settings->GetDouble("ColorcontrastOpacity")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_ColorcontrastAmountInput(const QVariant Value) {
  Settings->SetValue("ColorcontrastAmount",Value);
  if (Settings->GetDouble("ColorcontrastOpacity")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_ColorcontrastOpacityInput(const QVariant Value) {
  Settings->SetValue("ColorcontrastOpacity",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_ColorcontrastHaloControlInput(const QVariant Value) {
  Settings->SetValue("ColorcontrastHaloControl",Value);
  if (Settings->GetDouble("ColorcontrastOpacity")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabEyeCandy Tab
// Partim LAB Tone Adjust
//
////////////////////////////////////////////////////////////////////////////////

void CB_LABToneAdjust1MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LABToneAdjust1MaskType",Choice);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneAdjust1AmountInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust1Amount",Value);
  if (Settings->GetInt("LABToneAdjust1MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust1HueInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust1Hue",Value);
  if (Settings->GetInt("LABToneAdjust1MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust1SaturationInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust1Saturation",Value);
  if (Settings->GetInt("LABToneAdjust1MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust1LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LABToneAdjust1UpperLimit")-0.01));
  if (Settings->GetInt("LABToneAdjust1MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust1UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LABToneAdjust1LowerLimit")+0.01));
  if (Settings->GetInt("LABToneAdjust1MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust1SoftnessInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust1Softness",Value);
  if (Settings->GetInt("LABToneAdjust1MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust2MaskTypeChoice(const QVariant Choice) {
  Settings->SetValue("LABToneAdjust2MaskType",Choice);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneAdjust2AmountInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust2Amount",Value);
  if (Settings->GetInt("LABToneAdjust2MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust2HueInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust2Hue",Value);
  if (Settings->GetInt("LABToneAdjust2MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust2SaturationInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust2Saturation",Value);
  if (Settings->GetInt("LABToneAdjust2MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust2LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("LABToneAdjust2UpperLimit")-0.01));
  if (Settings->GetInt("LABToneAdjust2MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust2UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("LABToneAdjust2LowerLimit")+0.01));
  if (Settings->GetInt("LABToneAdjust2MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneAdjust2SoftnessInput(const QVariant Value) {
  Settings->SetValue("LABToneAdjust2Softness",Value);
  if (Settings->GetInt("LABToneAdjust2MaskType")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabEyeCandy Tab
// Partim LAB Tone
//
////////////////////////////////////////////////////////////////////////////////

void CB_LABToneSaturationInput(const QVariant Value) {
  Settings->SetValue("LABToneSaturation",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneAmountInput(const QVariant Value) {
  Settings->SetValue("LABToneAmount",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneHueInput(const QVariant Value) {
  Settings->SetValue("LABToneHue",Value);
  if (Settings->GetDouble("LABToneAmount")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneSSaturationInput(const QVariant Value) {
  Settings->SetValue("LABToneSSaturation",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneSAmountInput(const QVariant Value) {
  Settings->SetValue("LABToneSAmount",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneSHueInput(const QVariant Value) {
  Settings->SetValue("LABToneSHue",Value);
  if (Settings->GetDouble("LABToneSAmount")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneMSaturationInput(const QVariant Value) {
  Settings->SetValue("LABToneMSaturation",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneMAmountInput(const QVariant Value) {
  Settings->SetValue("LABToneMAmount",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneMHueInput(const QVariant Value) {
  Settings->SetValue("LABToneMHue",Value);
  if (Settings->GetDouble("LABToneMAmount")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

void CB_LABToneHSaturationInput(const QVariant Value) {
  Settings->SetValue("LABToneHSaturation",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneHAmountInput(const QVariant Value) {
  Settings->SetValue("LABToneHAmount",Value);
  Update(ptProcessorPhase_LabEyeCandy);
}

void CB_LABToneHHueInput(const QVariant Value) {
  Settings->SetValue("LABToneHHue",Value);
  if (Settings->GetDouble("LABToneHAmount")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the LabEyeCandy Tab
// Partim Vignette
//
////////////////////////////////////////////////////////////////////////////////

void CB_LabVignetteInnerRadiusInput(const QVariant Value) {
  Settings->SetValue("LabVignetteInnerRadius",MIN(Value.toDouble(), Settings->GetDouble("LabVignetteOuterRadius")));
  if (Settings->ToolIsActive("TabLABVignette")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}


void CB_LabVignetteOuterRadiusInput(const QVariant Value) {
  Settings->SetValue("LabVignetteOuterRadius",MAX(Value.toDouble(), Settings->GetDouble("LabVignetteInnerRadius")));
  if (Settings->ToolIsActive("TabLABVignette")) {
    Update(ptProcessorPhase_LabEyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the EyeCandy Tab
// Partim Black & White Styler
//
////////////////////////////////////////////////////////////////////////////////

void CB_BWStylerFilmTypeChoice(const QVariant Choice) {
  Settings->SetValue("BWStylerFilmType",Choice);
  if (Settings->ToolIsActive("TabBW")) {
    Update(ptProcessorPhase_EyeCandy);
  } else {
    MainWindow->UpdateSettings();
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the EyeCandy Tab
// Partim Toning
//
////////////////////////////////////////////////////////////////////////////////

void CB_Tone1ColorButton() {
  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("Tone1ColorRed"));
  Color.setGreen(Settings->GetInt("Tone1ColorGreen"));
  Color.setBlue(Settings->GetInt("Tone1ColorBlue"));
  QColorDialog Dialog(Color,NULL);
  Dialog.setStyle(Theme->ptSystemStyle);
  Dialog.setPalette(Theme->ptSystemPalette);
  Dialog.exec();
  QColor TestColor = Dialog.selectedColor();
  if (TestColor.isValid()) {
    Color = TestColor;
    Settings->SetValue("Tone1ColorRed",Color.red());
    Settings->SetValue("Tone1ColorGreen",Color.green());
    Settings->SetValue("Tone1ColorBlue",Color.blue());
    Pix.fill(Color);
    MainWindow->Tone1ColorButton->setIcon(Pix);
    if (Settings->ToolIsActive("TabRGBTone1")){
      Update(ptProcessorPhase_EyeCandy);
    }
  }
}

void CB_Tone1LowerLimitInput(const QVariant Value) {
  Settings->SetValue("Tone1LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("Tone1UpperLimit")-0.01));
  if (Settings->ToolIsActive("TabRGBTone1")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

void CB_Tone1UpperLimitInput(const QVariant Value) {
  Settings->SetValue("Tone1UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("Tone1LowerLimit")+0.01));
  if (Settings->ToolIsActive("TabRGBTone1")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

void CB_Tone2ColorButton() {
  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("Tone2ColorRed"));
  Color.setGreen(Settings->GetInt("Tone2ColorGreen"));
  Color.setBlue(Settings->GetInt("Tone2ColorBlue"));
  QColorDialog Dialog(Color,NULL);
  Dialog.setStyle(Theme->ptSystemStyle);
  Dialog.setPalette(Theme->ptSystemPalette);
  Dialog.exec();
  QColor TestColor = Dialog.selectedColor();
  if (TestColor.isValid()) {
    Color = TestColor;
    Settings->SetValue("Tone2ColorRed",Color.red());
    Settings->SetValue("Tone2ColorGreen",Color.green());
    Settings->SetValue("Tone2ColorBlue",Color.blue());
    Pix.fill(Color);
    MainWindow->Tone2ColorButton->setIcon(Pix);
    if (Settings->ToolIsActive("TabRGBTone1")){
      Update(ptProcessorPhase_EyeCandy);
    }
  }
}

void CB_Tone2LowerLimitInput(const QVariant Value) {
  Settings->SetValue("Tone2LowerLimit",MIN(Value.toDouble(), Settings->GetDouble("Tone2UpperLimit")-0.01));
  if (Settings->ToolIsActive("TabRGBTone1")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

void CB_Tone2UpperLimitInput(const QVariant Value) {
  Settings->SetValue("Tone2UpperLimit",MAX(Value.toDouble(), Settings->GetDouble("Tone2LowerLimit")+0.01));
  if (Settings->ToolIsActive("TabRGBTone1")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the EyeCandy Tab
// Partim Texture Overlay
//
////////////////////////////////////////////////////////////////////////////////

void CB_TextureOverlayMaskChoice(const QVariant Choice) {
  Settings->SetValue("TextureOverlayMask",Choice);
  if (Settings->ToolIsActive("TabTextureOverlay")) {
    Update(ptProcessorPhase_EyeCandy);
  } else {
    MainWindow->UpdateSettings();
  }
}

void CB_TextureOverlayButton() {
  QString Directory = "";
  if (Settings->GetString("TextureOverlayFile")!="") {
    QFileInfo PathInfo(Settings->GetString("TextureOverlayFile"));
    Directory = PathInfo.absolutePath();
  } else {
    Directory = Settings->GetString("UserDirectory");
  }

  QString TextureOverlayString = QFileDialog::getOpenFileName(NULL,
    QObject::tr("Get texture bitmap file"),
    Directory,
    BitmapPattern);

  if (0 == TextureOverlayString.size() ) {
    // Canceled just return
    return;
  } else {
    QFileInfo PathInfo(TextureOverlayString);
    Settings->SetValue("TextureOverlayFile",PathInfo.absoluteFilePath());
  }

  // Reflect in gui.
  if (Settings->ToolIsActive("TabTextureOverlay")) {
    // free old one
    if (TheProcessor->m_Image_TextureOverlay) {
      delete TheProcessor->m_Image_TextureOverlay;
      TheProcessor->m_Image_TextureOverlay = NULL;
    }
    Update(ptProcessorPhase_EyeCandy);
  } else {
    MainWindow->UpdateSettings();
  }
}

void CB_TextureOverlayClearButton() {
  if (TheProcessor->m_Image_TextureOverlay) {
    delete TheProcessor->m_Image_TextureOverlay;
    TheProcessor->m_Image_TextureOverlay = NULL;
  }
  Settings->SetValue("TextureOverlayFile","");
  if (Settings->ToolIsActive("TabTextureOverlay")) {
    Settings->SetValue("TextureOverlayMode",0);
    Update(ptProcessorPhase_EyeCandy);
  } else {
    MainWindow->UpdateSettings();
  }
}

void CB_TextureOverlayInnerRadiusInput(const QVariant Value) {
  Settings->SetValue("TextureOverlayInnerRadius",MIN(Value.toDouble(), Settings->GetDouble("TextureOverlayOuterRadius")));
  if (Settings->ToolIsActive("TabTextureOverlay")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

void CB_TextureOverlayOuterRadiusInput(const QVariant Value) {
  Settings->SetValue("TextureOverlayOuterRadius",MAX(Value.toDouble(), Settings->GetDouble("TextureOverlayInnerRadius")));
  if (Settings->ToolIsActive("TabTextureOverlay")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the EyeCandy Tab
// Partim Gradual Overlay
//
////////////////////////////////////////////////////////////////////////////////

void CB_GradualOverlay1ColorButton() {
  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("GradualOverlay1ColorRed"));
  Color.setGreen(Settings->GetInt("GradualOverlay1ColorGreen"));
  Color.setBlue(Settings->GetInt("GradualOverlay1ColorBlue"));
  QColorDialog Dialog(Color,NULL);
  Dialog.setStyle(Theme->ptSystemStyle);
  Dialog.setPalette(Theme->ptSystemPalette);
  Dialog.exec();
  QColor TestColor = Dialog.selectedColor();
  if (TestColor.isValid()) {
    Color = TestColor;
    Settings->SetValue("GradualOverlay1ColorRed",Color.red());
    Settings->SetValue("GradualOverlay1ColorGreen",Color.green());
    Settings->SetValue("GradualOverlay1ColorBlue",Color.blue());
    Pix.fill(Color);
    MainWindow->GradualOverlay1ColorButton->setIcon(Pix);
    if (Settings->ToolIsActive("TabGradualOverlay1")){
      Update(ptProcessorPhase_EyeCandy);
    }
  }
}

void CB_GradualOverlay1LowerLevelInput(const QVariant Value) {
  Settings->SetValue("GradualOverlay1LowerLevel",MIN(Value.toDouble(), Settings->GetDouble("GradualOverlay1UpperLevel")));
  if (Settings->ToolIsActive("TabGradualOverlay1")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}


void CB_GradualOverlay1UpperLevelInput(const QVariant Value) {
  Settings->SetValue("GradualOverlay1UpperLevel",MAX(Value.toDouble(), Settings->GetDouble("GradualOverlay1LowerLevel")));
  if (Settings->ToolIsActive("TabGradualOverlay1")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

void CB_GradualOverlay2ColorButton() {
  QPixmap Pix(80, 14);
  QColor  Color;
  Color.setRed(Settings->GetInt("GradualOverlay2ColorRed"));
  Color.setGreen(Settings->GetInt("GradualOverlay2ColorGreen"));
  Color.setBlue(Settings->GetInt("GradualOverlay2ColorBlue"));
  QColorDialog Dialog(Color,NULL);
  Dialog.setStyle(Theme->ptSystemStyle);
  Dialog.setPalette(Theme->ptSystemPalette);
  Dialog.exec();
  QColor TestColor = Dialog.selectedColor();
  if (TestColor.isValid()) {
    Color = TestColor;
    Settings->SetValue("GradualOverlay2ColorRed",Color.red());
    Settings->SetValue("GradualOverlay2ColorGreen",Color.green());
    Settings->SetValue("GradualOverlay2ColorBlue",Color.blue());
    Pix.fill(Color);
    MainWindow->GradualOverlay2ColorButton->setIcon(Pix);
    if (Settings->ToolIsActive("TabGradualOverlay2")){
      Update(ptProcessorPhase_EyeCandy);
    }
  }
}

void CB_GradualOverlay2LowerLevelInput(const QVariant Value) {
  Settings->SetValue("GradualOverlay2LowerLevel",MIN(Value.toDouble(), Settings->GetDouble("GradualOverlay2UpperLevel")));
  if (Settings->ToolIsActive("TabGradualOverlay2")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

void CB_GradualOverlay2UpperLevelInput(const QVariant Value) {
  Settings->SetValue("GradualOverlay2UpperLevel",MAX(Value.toDouble(), Settings->GetDouble("GradualOverlay2LowerLevel")));
  if (Settings->ToolIsActive("TabGradualOverlay2")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to the EyeCandy Tab
// Partim Vignette
//
////////////////////////////////////////////////////////////////////////////////

void CB_VignetteInnerRadiusInput(const QVariant Value) {
  Settings->SetValue("VignetteInnerRadius",MIN(Value.toDouble(), Settings->GetDouble("VignetteOuterRadius")));
  if (Settings->ToolIsActive("TabRGBVignette")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}


void CB_VignetteOuterRadiusInput(const QVariant Value) {
  Settings->SetValue("VignetteOuterRadius",MAX(Value.toDouble(), Settings->GetDouble("VignetteInnerRadius")));
  if (Settings->ToolIsActive("TabRGBVignette")) {
    Update(ptProcessorPhase_EyeCandy);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Callbacks pertaining to Out Tab
//
////////////////////////////////////////////////////////////////////////////////

/* TODO MIKE: dead?
void CB_OutputExposureInput(const QVariant Value) {
  // The Gui element is expressed in EV.
  Settings->SetValue("OutputExposure",Value);
  Update(ptProcessorPhase_Output);
}

void CB_OutputExposureClipModeChoice(const QVariant Value) {
  Settings->SetValue("OutputExposureClipMode",Value);
  Update(ptProcessorPhase_Output);
}
*/

void CB_SaveFormatChoice(const QVariant Choice) {
  Settings->SetValue("SaveFormat",Choice);
  MainWindow->UpdateSettings();
}

void CB_EraseExifThumbnailCheck(const QVariant State) {
  Settings->SetValue("EraseExifThumbnail",State);
  TheProcessor->ReadExifBuffer();
}

void CB_ImageRatingInput(const QVariant Value) {
  Settings->SetValue("ImageRating",Value);
  TheProcessor->ReadExifBuffer();
}

inline void BlockExport(bool Block) {
  if (!Block) QApplication::processEvents();
  MainWindow->ToGimpButton->setEnabled(!Block);
  QApplication::processEvents();
}

inline void BlockSave(bool Block) {
  if (!Block) QApplication::processEvents();
  MainWindow->WritePipeButton->setEnabled(!Block);
  QApplication::processEvents();
}

void SaveOutput(const short mode) {
  BlockSave(true);
  if (mode==ptOutputMode_Full) {
    CB_MenuFileSaveOutput();
  } else if (mode==ptOutputMode_Pipe) {
    WritePipe();
  } else if (mode==ptOutputMode_Jobfile) {
    CB_MenuFileWriteJob(1);
  } else if (mode==ptOutputMode_Settingsfile) {
    CB_MenuFileWriteSettings();
  }
  BlockSave(false);
}

void Export(const short mode) {
  BlockExport(true);
  if (mode==ptExportMode_GimpPipe && Settings->GetInt("ExportToGimp")) {
    GimpExport(1);
    BlockExport(false);
    return;
  } else if (mode==ptExportMode_GimpFull && Settings->GetInt("ExportToGimp")) {
    GimpExport(0);
    BlockExport(false);
    return;
  }

  // regular export without plugin
  QTemporaryFile ImageFile;
  ImageFile.setFileTemplate(QDir::tempPath()+"/XXXXXX.tiff");
  assert (ImageFile.open());
  QString ImageFileName = ImageFile.fileName();
  ImageFile.setAutoRemove(false);

  short Temp = Settings->GetInt("SaveFormat");
  Settings->SetValue("SaveFormat", ptSaveFormat_TIFF16);

  if (mode==ptExportMode_GimpPipe && !Settings->GetInt("ExportToGimp")) {
    WritePipe(ImageFileName);
  } else if (mode==ptExportMode_GimpFull && !Settings->GetInt("ExportToGimp")) {
    CB_MenuFileSaveOutput(ImageFileName);
  }

  Settings->SetValue("SaveFormat", Temp);

  QString ExportCommand = Settings->GetString("GimpExecCommand");
  QStringList ExportArguments;
  ExportArguments << ImageFileName;
  QProcess* ExportProcess = new QProcess();
  ExportProcess->startDetached(ExportCommand,ExportArguments);
  BlockExport(false);
}

void CB_WriteOutputButton() {
  SaveOutput(Settings->GetInt("OutputMode"));
}

void CB_WritePipeButton() {
  SaveOutput(Settings->GetInt("SaveButtonMode"));
}

////////////////////////////////////////////////////////////////////////////////
//
// Callback dispatcher
//
// For refactoring:
// The control switching the active state of the tool needs always to trigger
// a pipe run (also if it just disabled the tool!) The only exception is the
// external blocked state (blocked or hidden tools!).
//
////////////////////////////////////////////////////////////////////////////////

// For simple switches, just get the value to the settings
void Standard_CB_JustSet (const QString ObjectName, const QVariant Value) {
  QString Temp = ObjectName;
  if (Temp.endsWith("Input") || Temp.endsWith("Check")) Temp.chop(5);
  if (Temp.endsWith("Choice")) Temp.chop(6);
  Settings->SetValue(Temp,Value);
}

// Standard form of call backs, value to settings and pipe run if needed
void Standard_CB_SetAndRun (const QString ObjectName, const QVariant Value) {
  QString Key = ObjectName;
  if (Key.endsWith("Input") || Key.endsWith("Check")) Key.chop(5);
  if (Key.endsWith("Choice")) Key.chop(6);

  QWidget* CurrentControl = Settings->GetGuiWidget(Key);
  if (CurrentControl == NULL) assert(!"Widget not found");
  ptGroupBox* CurrentTool = dynamic_cast<ptGroupBox*>(CurrentControl);

  while (CurrentTool == NULL) {
    CurrentControl = CurrentControl->parentWidget();
    CurrentTool = dynamic_cast<ptGroupBox*>(CurrentControl);
  }
  QString ToolName = CurrentTool->objectName();

  // Save previous state for rerun when disabling a filter
  short PreviousActiveState = Settings->ToolIsActive(ToolName);

  Settings->SetValue(Key,Value);

  short NewActiveState = Settings->ToolIsActive(ToolName);
  CurrentTool->SetActive(NewActiveState);

  if (NewActiveState || PreviousActiveState) {
    Update(ToolName);
  } else {
    MainWindow->UpdateSettings();
  }
}

void CB_InputChanged(const QString ObjectName, const QVariant Value) {

  // No CB processing while in startup phase. Too much
  // noise events otherwise.
  if (InStartup) return;

  if (ObjectName == "ZoomInput") {
    CB_ZoomInput(Value);

  #define M_Dispatch(Name)\
  } else if (ObjectName == #Name) { CB_ ## Name (Value);

  #define M_JustSetDispatch(Name)\
  } else if (ObjectName == #Name) { Standard_CB_JustSet(#Name,Value);

  #define M_SetAndRunDispatch(Name)\
  } else if (ObjectName == #Name) { Standard_CB_SetAndRun(#Name,Value);

  M_Dispatch(CameraColorChoice)
  M_Dispatch(CameraColorProfileIntentChoice)
  M_Dispatch(CameraColorGammaChoice)

  M_Dispatch(WorkColorChoice)
  M_Dispatch(CMQualityChoice)

  M_Dispatch(PreviewColorProfileIntentChoice)

  M_Dispatch(StyleChoice)
  M_Dispatch(StyleHighLightChoice)

  M_Dispatch(SaveConfirmationCheck)
  M_Dispatch(ResetSettingsConfirmationCheck)
  M_Dispatch(FullPipeConfirmationCheck)

  M_Dispatch(WriteBackupSettingsCheck)

  M_Dispatch(MemoryTestInput)

  M_Dispatch(ExportToGimpCheck)

  M_Dispatch(StartupSettingsCheck)
  M_Dispatch(StartupSettingsResetCheck)
  M_Dispatch(StartupUIModeChoice)
  M_Dispatch(StartupPipeSizeChoice)

  M_JustSetDispatch(StartupSwitchARCheck)

  M_Dispatch(CropInitialZoomChoice)

  M_Dispatch(RememberSettingLevelChoice)
  M_Dispatch(InputsAddPowerLawCheck)
  M_Dispatch(ToolBoxModeCheck)
  M_Dispatch(TabStatusIndicatorInput)
  M_Dispatch(PreviewTabModeCheck)
  M_Dispatch(BackgroundColorCheck)
  M_Dispatch(SliderWidthInput)
  M_Dispatch(SaveButtonModeChoice)
  M_Dispatch(ResetButtonModeChoice)
  M_Dispatch(SearchBarEnableCheck)

  M_Dispatch(PipeSizeChoice)
  M_Dispatch(RunModeCheck)
  M_Dispatch(SpecialPreviewChoice)

  M_Dispatch(BadPixelsChoice)
  M_Dispatch(DarkFrameChoice)

  M_Dispatch(WhiteBalanceChoice)
  M_Dispatch(ColorTemperatureInput)
  M_Dispatch(GreenIntensityInput)
  M_Dispatch(MultiplierEnhanceCheck)
  M_Dispatch(RMultiplierInput)
  M_Dispatch(GMultiplierInput)
  M_Dispatch(BMultiplierInput)
  M_Dispatch(ManualBlackPointCheck)
  M_Dispatch(BlackPointInput)
  M_Dispatch(ManualWhitePointCheck)
  M_Dispatch(WhitePointInput)
  M_Dispatch(CaCorrectChoice)
  M_Dispatch(CaRedInput)
  M_Dispatch(CaBlueInput)
  M_Dispatch(GreenEquilInput)
  M_Dispatch(CfaLineDenoiseInput)
  M_Dispatch(AdjustMaximumThresholdInput)
  M_Dispatch(RawDenoiseThresholdInput)
  M_Dispatch(HotpixelReductionInput)
  M_Dispatch(BayerDenoiseChoice)
  M_Dispatch(InterpolationChoice)
  M_Dispatch(InterpolationPassesInput)
  M_Dispatch(MedianPassesInput)
  M_Dispatch(ESMedianPassesInput)
  M_Dispatch(EeciRefineCheck)
  M_Dispatch(ClipModeChoice)
  M_Dispatch(ClipParameterInput)

  M_Dispatch(LfunFocalInput)
  M_Dispatch(LfunApertureInput)
  M_Dispatch(LfunDistanceInput)
  M_Dispatch(LfunScaleInput)
  M_Dispatch(LfunAutoScaleCheck)
  M_Dispatch(LfunCAModelChoice)
  M_Dispatch(LfunCALinearKbInput)
  M_Dispatch(LfunCALinearKrInput)
  M_Dispatch(LfunCAPoly3VrInput)
  M_Dispatch(LfunCAPoly3VbInput)
  M_Dispatch(LfunCAPoly3CrInput)
  M_Dispatch(LfunCAPoly3CbInput)
  M_Dispatch(LfunCAPoly3BrInput)
  M_Dispatch(LfunCAPoly3BbInput)
  M_Dispatch(LfunVignetteModelChoice)
  M_Dispatch(LfunVignettePoly6K1Input)
  M_Dispatch(LfunVignettePoly6K2Input)
  M_Dispatch(LfunVignettePoly6K3Input)
  M_Dispatch(LfunSrcGeoChoice)
  M_Dispatch(LfunTargetGeoChoice)
  M_Dispatch(LfunDistModelChoice)
  M_Dispatch(LfunDistPoly3K1Input)
  M_Dispatch(LfunDistPoly5K1Input)
  M_Dispatch(LfunDistPoly5K2Input)
  M_Dispatch(LfunDistFov1OmegaInput)
  M_Dispatch(LfunDistPTLensAInput)
  M_Dispatch(LfunDistPTLensBInput)
  M_Dispatch(LfunDistPTLensCInput)

  M_Dispatch(DefishCheck)
  M_Dispatch(DefishFocalLengthInput)
  M_Dispatch(DefishScaleInput)
  M_Dispatch(DefishAutoScaleCheck)

  M_Dispatch(RotateInput)
  M_Dispatch(PerspectiveFocalLengthInput)
  M_Dispatch(PerspectiveTiltInput)
  M_Dispatch(PerspectiveTurnInput)
  M_Dispatch(PerspectiveScaleXInput)
  M_Dispatch(PerspectiveScaleYInput)
  M_Dispatch(GridCheck)
  M_Dispatch(GridXInput)
  M_Dispatch(GridYInput)

  M_Dispatch(CropCheck)
  M_Dispatch(CropGuidelinesChoice)
  M_Dispatch(LightsOutChoice)
  M_Dispatch(FixedAspectRatioCheck)
  M_Dispatch(AspectRatioWChoice)
  M_Dispatch(AspectRatioHChoice)
  M_Dispatch(CropExposureInput)

  M_Dispatch(LqrEnergyChoice)
  M_Dispatch(LqrScalingChoice)
  M_Dispatch(LqrHorScaleInput)
  M_Dispatch(LqrVertScaleInput)
  M_Dispatch(LqrWidthInput)
  M_Dispatch(LqrHeightInput)
  M_Dispatch(LqrVertFirstCheck)

  M_Dispatch(ResizeCheck)
  M_Dispatch(ResizeDimensionChoice)
  M_Dispatch(ResizeScaleInput)
  M_Dispatch(ResizeFilterChoice)
  M_Dispatch(AutomaticPipeSizeCheck)

  M_Dispatch(FlipModeChoice)

  M_Dispatch(GeometryBlockCheck)

  M_Dispatch(LevelsBlackPointInput)
  M_Dispatch(LevelsWhitePointInput)

  M_Dispatch(ChannelMixerChoice)
  M_Dispatch(ChannelMixerR2RInput)
  M_Dispatch(ChannelMixerG2RInput)
  M_Dispatch(ChannelMixerB2RInput)
  M_Dispatch(ChannelMixerR2GInput)
  M_Dispatch(ChannelMixerG2GInput)
  M_Dispatch(ChannelMixerB2GInput)
  M_Dispatch(ChannelMixerR2BInput)
  M_Dispatch(ChannelMixerG2BInput)
  M_Dispatch(ChannelMixerB2BInput)

  M_Dispatch(VibranceInput)
  M_Dispatch(IntensityRedInput)
  M_Dispatch(IntensityGreenInput)
  M_Dispatch(IntensityBlueInput)
  M_Dispatch(ColorEnhanceShadowsInput)
  M_Dispatch(ColorEnhanceHighlightsInput)

  M_Dispatch(HighlightsRInput)
  M_Dispatch(HighlightsGInput)
  M_Dispatch(HighlightsBInput)

  M_Dispatch(ExposureGainInput)
  M_Dispatch(CatchWhiteInput)
  M_Dispatch(CatchBlackInput)

  M_Dispatch(AutoExposureChoice)
  M_Dispatch(WhiteFractionInput)
  M_Dispatch(WhiteLevelInput)
  M_Dispatch(ExposureInput)
  M_Dispatch(ExposureClipModeChoice)

  M_Dispatch(Reinhard05Check)
  M_Dispatch(Reinhard05BrightnessInput)
  M_Dispatch(Reinhard05ChromaInput)
  M_Dispatch(Reinhard05LightInput)

  M_Dispatch(RGBGammaAmountInput)
  M_Dispatch(RGBGammaLinearityInput)

  M_Dispatch(NormalizationOpacityInput)
//  M_Dispatch(NormalizationBlackPointInput)
//  M_Dispatch(NormalizationWhitePointInput)

  M_Dispatch(LMHLightRecovery1MaskTypeChoice)
  M_Dispatch(LMHLightRecovery1AmountInput)
  M_Dispatch(LMHLightRecovery1LowerLimitInput)
  M_Dispatch(LMHLightRecovery1UpperLimitInput)
  M_Dispatch(LMHLightRecovery1SoftnessInput)

  M_Dispatch(LMHLightRecovery2MaskTypeChoice)
  M_Dispatch(LMHLightRecovery2AmountInput)
  M_Dispatch(LMHLightRecovery2LowerLimitInput)
  M_Dispatch(LMHLightRecovery2UpperLimitInput)
  M_Dispatch(LMHLightRecovery2SoftnessInput)

  M_Dispatch(RGBContrastAmountInput)
  M_Dispatch(RGBContrastThresholdInput)

  M_Dispatch(RGBTextureContrastThresholdInput)
  M_Dispatch(RGBTextureContrastSoftnessInput)
  M_Dispatch(RGBTextureContrastAmountInput)
  M_Dispatch(RGBTextureContrastOpacityInput)
  M_Dispatch(RGBTextureContrastEdgeControlInput)
  M_Dispatch(RGBTextureContrastMaskingInput)

  M_Dispatch(Microcontrast1MaskTypeChoice)
  M_Dispatch(Microcontrast1RadiusInput)
  M_Dispatch(Microcontrast1AmountInput)
  M_Dispatch(Microcontrast1OpacityInput)
  M_Dispatch(Microcontrast1HaloControlInput)
  M_Dispatch(Microcontrast1LowerLimitInput)
  M_Dispatch(Microcontrast1UpperLimitInput)
  M_Dispatch(Microcontrast1SoftnessInput)

  M_Dispatch(Microcontrast2MaskTypeChoice)
  M_Dispatch(Microcontrast2RadiusInput)
  M_Dispatch(Microcontrast2AmountInput)
  M_Dispatch(Microcontrast2OpacityInput)
  M_Dispatch(Microcontrast2HaloControlInput)
  M_Dispatch(Microcontrast2LowerLimitInput)
  M_Dispatch(Microcontrast2UpperLimitInput)
  M_Dispatch(Microcontrast2SoftnessInput)

  M_Dispatch(CurveRGBChoice)
  M_Dispatch(CurveRChoice)
  M_Dispatch(CurveGChoice)
  M_Dispatch(CurveBChoice)
  M_Dispatch(CurveLChoice)
  M_Dispatch(CurveLaChoice)
  M_Dispatch(CurveLbChoice)
  M_Dispatch(CurveLByHueChoice)
  M_Dispatch(CurveHueChoice)
  M_Dispatch(CurveTextureChoice)
  M_Dispatch(CurveShadowsHighlightsChoice)
  M_Dispatch(CurveDenoiseChoice)
  M_Dispatch(CurveDenoise2Choice)
  M_Dispatch(CurveSaturationChoice)
  M_Dispatch(BaseCurveChoice)
  M_Dispatch(BaseCurve2Choice)
  M_Dispatch(CurveOutlineChoice)

  M_Dispatch(LABTransformChoice)

  M_Dispatch(ShadowsHighlightsFineInput)
  M_Dispatch(ShadowsHighlightsCoarseInput)
  M_Dispatch(ShadowsHighlightsRadiusInput)

  M_Dispatch(DRCBetaInput)
  M_Dispatch(DRCAlphaInput)
  M_Dispatch(DRCColorInput)

  M_Dispatch(LabLMHLightRecovery1MaskTypeChoice)
  M_Dispatch(LabLMHLightRecovery1AmountInput)
  M_Dispatch(LabLMHLightRecovery1LowerLimitInput)
  M_Dispatch(LabLMHLightRecovery1UpperLimitInput)
  M_Dispatch(LabLMHLightRecovery1SoftnessInput)

  M_Dispatch(LabLMHLightRecovery2MaskTypeChoice)
  M_Dispatch(LabLMHLightRecovery2AmountInput)
  M_Dispatch(LabLMHLightRecovery2LowerLimitInput)
  M_Dispatch(LabLMHLightRecovery2UpperLimitInput)
  M_Dispatch(LabLMHLightRecovery2SoftnessInput)

  M_Dispatch(TextureContrast1ThresholdInput)
  M_Dispatch(TextureContrast1SoftnessInput)
  M_Dispatch(TextureContrast1AmountInput)
  M_Dispatch(TextureContrast1OpacityInput)
  M_Dispatch(TextureContrast1EdgeControlInput)
  M_Dispatch(TextureContrast1MaskingInput)
  M_Dispatch(TextureContrast2ThresholdInput)
  M_Dispatch(TextureContrast2SoftnessInput)
  M_Dispatch(TextureContrast2AmountInput)
  M_Dispatch(TextureContrast2OpacityInput)
  M_Dispatch(TextureContrast2EdgeControlInput)
  M_Dispatch(TextureContrast2MaskingInput)

  M_SetAndRunDispatch(LC1RadiusInput)
  M_SetAndRunDispatch(LC1FeatherInput)
  M_SetAndRunDispatch(LC1OpacityInput)
  M_SetAndRunDispatch(LC1mInput)
  M_SetAndRunDispatch(LC2RadiusInput)
  M_SetAndRunDispatch(LC2FeatherInput)
  M_SetAndRunDispatch(LC2OpacityInput)
  M_SetAndRunDispatch(LC2mInput)

  M_SetAndRunDispatch(ContrastAmountInput)
  M_SetAndRunDispatch(ContrastThresholdInput)
  M_SetAndRunDispatch(SaturationAmountInput)
  M_SetAndRunDispatch(ColorBoostValueAInput)
  M_SetAndRunDispatch(ColorBoostValueBInput)

  M_Dispatch(LabMicrocontrast1MaskTypeChoice)
  M_Dispatch(LabMicrocontrast1RadiusInput)
  M_Dispatch(LabMicrocontrast1AmountInput)
  M_Dispatch(LabMicrocontrast1OpacityInput)
  M_Dispatch(LabMicrocontrast1HaloControlInput)
  M_Dispatch(LabMicrocontrast1LowerLimitInput)
  M_Dispatch(LabMicrocontrast1UpperLimitInput)
  M_Dispatch(LabMicrocontrast1SoftnessInput)

  M_Dispatch(LabMicrocontrast2MaskTypeChoice)
  M_Dispatch(LabMicrocontrast2RadiusInput)
  M_Dispatch(LabMicrocontrast2AmountInput)
  M_Dispatch(LabMicrocontrast2OpacityInput)
  M_Dispatch(LabMicrocontrast2HaloControlInput)
  M_Dispatch(LabMicrocontrast2LowerLimitInput)
  M_Dispatch(LabMicrocontrast2UpperLimitInput)
  M_Dispatch(LabMicrocontrast2SoftnessInput)

  M_SetAndRunDispatch(LabLevelsBlackPointInput)
  M_SetAndRunDispatch(LabLevelsWhitePointInput)

  M_SetAndRunDispatch(ImpulseDenoiseThresholdLInput)
  M_SetAndRunDispatch(ImpulseDenoiseThresholdABInput)

  M_Dispatch(EAWMasterInput)
  M_Dispatch(EAWLevel1Input)
  M_Dispatch(EAWLevel2Input)
  M_Dispatch(EAWLevel3Input)
  M_Dispatch(EAWLevel4Input)
  M_Dispatch(EAWLevel5Input)
  M_Dispatch(EAWLevel6Input)

  M_Dispatch(GREYCLabChoice)
  M_Dispatch(GREYCLabMaskTypeChoice)
  M_Dispatch(GREYCLabFastCheck)
  M_Dispatch(GREYCLabInterpolationChoice)
  M_Dispatch(GREYCLabOpacityInput)
  M_Dispatch(GREYCLabAmplitudeInput)
  M_Dispatch(GREYCLabIterationsInput)
  M_Dispatch(GREYCLabSharpnessInput)
  M_Dispatch(GREYCLabAnisotropyInput)
  M_Dispatch(GREYCLabAlphaInput)
  M_Dispatch(GREYCLabSigmaInput)
  M_Dispatch(GREYCLabdlInput)
  M_Dispatch(GREYCLabdaInput)
  M_Dispatch(GREYCLabGaussPrecisionInput)

  M_Dispatch(DefringeRadiusInput)
  M_Dispatch(DefringeThresholdInput)
  M_Dispatch(DefringeShiftInput)
  M_Dispatch(DefringeColor1Check)
  M_Dispatch(DefringeColor2Check)
  M_Dispatch(DefringeColor3Check)
  M_Dispatch(DefringeColor4Check)
  M_Dispatch(DefringeColor5Check)
  M_Dispatch(DefringeColor6Check)

  M_Dispatch(PyrDenoiseLAmountInput)
  M_Dispatch(PyrDenoiseABAmountInput)
  M_Dispatch(PyrDenoiseGammaInput)
  M_Dispatch(PyrDenoiseLevelsInput)

  M_Dispatch(BilateralLOpacityInput)
  M_Dispatch(BilateralLUseMaskInput)
  M_Dispatch(BilateralLSigmaRInput)
  M_Dispatch(BilateralLSigmaSInput)
  M_Dispatch(BilateralASigmaRInput)
  M_Dispatch(BilateralASigmaSInput)
  M_Dispatch(BilateralBSigmaRInput)
  M_Dispatch(BilateralBSigmaSInput)

  M_Dispatch(DenoiseCurveSigmaRInput)
  M_Dispatch(DenoiseCurveSigmaSInput)

  M_Dispatch(WaveletDenoiseLInput)
  M_Dispatch(WaveletDenoiseLSoftnessInput)
  M_Dispatch(WaveletDenoiseLSharpnessInput)
  M_Dispatch(WaveletDenoiseLAnisotropyInput)
  M_Dispatch(WaveletDenoiseLAlphaInput)
  M_Dispatch(WaveletDenoiseLSigmaInput)
  M_Dispatch(WaveletDenoiseAInput)
  M_Dispatch(WaveletDenoiseASoftnessInput)
  M_Dispatch(WaveletDenoiseBInput)
  M_Dispatch(WaveletDenoiseBSoftnessInput)

  M_Dispatch(DetailCurveScalingInput)
  M_Dispatch(DetailCurveWeightInput)
  M_Dispatch(DetailCurveHotpixelInput)

  M_Dispatch(GradientSharpenPassesInput)
  M_Dispatch(GradientSharpenStrengthInput)
  M_Dispatch(MLMicroContrastStrengthInput)
  M_Dispatch(MLMicroContrastScalingInput)
  M_Dispatch(MLMicroContrastWeightInput)
  M_Dispatch(LabHotpixelInput)

  M_Dispatch(WienerFilterCheck)
  M_Dispatch(WienerFilterUseEdgeMaskCheck)
  M_Dispatch(WienerFilterAmountInput)
  M_Dispatch(WienerFilterGaussianInput)
  M_Dispatch(WienerFilterBoxInput)
  M_Dispatch(WienerFilterLensBlurInput)

  M_Dispatch(InverseDiffusionIterationsInput)
  M_Dispatch(InverseDiffusionAmplitudeInput)
  M_Dispatch(InverseDiffusionUseEdgeMaskCheck)
  M_Dispatch(USMChoice)
  M_Dispatch(USMRadiusInput)
  M_Dispatch(USMAmountInput)
  M_Dispatch(USMThresholdInput)

  M_SetAndRunDispatch(HighpassChoice)
  M_SetAndRunDispatch(HighpassRadiusInput)
  M_SetAndRunDispatch(HighpassAmountInput)
  M_SetAndRunDispatch(HighpassDenoiseInput)

  M_Dispatch(Grain1MaskTypeChoice)
  M_Dispatch(Grain1ModeChoice)
  M_Dispatch(Grain1StrengthInput)
  M_Dispatch(Grain1RadiusInput)
  M_Dispatch(Grain1OpacityInput)
  M_Dispatch(Grain1LowerLimitInput)
  M_Dispatch(Grain1UpperLimitInput)

  M_Dispatch(Grain2MaskTypeChoice)
  M_Dispatch(Grain2ModeChoice)
  M_Dispatch(Grain2StrengthInput)
  M_Dispatch(Grain2RadiusInput)
  M_Dispatch(Grain2OpacityInput)
  M_Dispatch(Grain2LowerLimitInput)
  M_Dispatch(Grain2UpperLimitInput)

  M_Dispatch(ViewLABChoice)

  M_SetAndRunDispatch(OutlineModeChoice)
  M_SetAndRunDispatch(OutlineSwitchLayerCheck)
  M_SetAndRunDispatch(OutlineGradientModeChoice)
  M_SetAndRunDispatch(OutlineWeightInput)
  M_SetAndRunDispatch(OutlineBlurRadiusInput)

  M_Dispatch(ColorcontrastRadiusInput)
  M_Dispatch(ColorcontrastAmountInput)
  M_Dispatch(ColorcontrastOpacityInput)
  M_Dispatch(ColorcontrastHaloControlInput)

  M_Dispatch(LABToneAdjust1AmountInput)
  M_Dispatch(LABToneAdjust1HueInput)
  M_Dispatch(LABToneAdjust1SaturationInput)
  M_Dispatch(LABToneAdjust1LowerLimitInput)
  M_Dispatch(LABToneAdjust1UpperLimitInput)
  M_Dispatch(LABToneAdjust1SoftnessInput)
  M_Dispatch(LABToneAdjust1MaskTypeChoice)
  M_Dispatch(LABToneAdjust2AmountInput)
  M_Dispatch(LABToneAdjust2HueInput)
  M_Dispatch(LABToneAdjust2SaturationInput)
  M_Dispatch(LABToneAdjust2MaskTypeChoice)
  M_Dispatch(LABToneAdjust2LowerLimitInput)
  M_Dispatch(LABToneAdjust2UpperLimitInput)
  M_Dispatch(LABToneAdjust2SoftnessInput)

  M_SetAndRunDispatch(LAdjustC1Input)
  M_SetAndRunDispatch(LAdjustC2Input)
  M_SetAndRunDispatch(LAdjustC3Input)
  M_SetAndRunDispatch(LAdjustC4Input)
  M_SetAndRunDispatch(LAdjustC5Input)
  M_SetAndRunDispatch(LAdjustC6Input)
  M_SetAndRunDispatch(LAdjustC7Input)
  M_SetAndRunDispatch(LAdjustC8Input)
  M_SetAndRunDispatch(LAdjustSC1Input)
  M_SetAndRunDispatch(LAdjustSC2Input)
  M_SetAndRunDispatch(LAdjustSC3Input)
  M_SetAndRunDispatch(LAdjustSC4Input)
  M_SetAndRunDispatch(LAdjustSC5Input)
  M_SetAndRunDispatch(LAdjustSC6Input)
  M_SetAndRunDispatch(LAdjustSC7Input)
  M_SetAndRunDispatch(LAdjustSC8Input)

  M_Dispatch(LABToneSaturationInput)
  M_Dispatch(LABToneAmountInput)
  M_Dispatch(LABToneHueInput)
  M_Dispatch(LABToneSSaturationInput)
  M_Dispatch(LABToneSAmountInput)
  M_Dispatch(LABToneSHueInput)
  M_Dispatch(LABToneMSaturationInput)
  M_Dispatch(LABToneMAmountInput)
  M_Dispatch(LABToneMHueInput)
  M_Dispatch(LABToneHSaturationInput)
  M_Dispatch(LABToneHAmountInput)
  M_Dispatch(LABToneHHueInput)

  M_SetAndRunDispatch(LabVignetteModeChoice)
  M_SetAndRunDispatch(LabVignetteInput)
  M_SetAndRunDispatch(LabVignetteAmountInput)
  M_Dispatch(LabVignetteInnerRadiusInput)
  M_Dispatch(LabVignetteOuterRadiusInput)
  M_SetAndRunDispatch(LabVignetteRoundnessInput)
  M_SetAndRunDispatch(LabVignetteCenterXInput)
  M_SetAndRunDispatch(LabVignetteCenterYInput)
  M_SetAndRunDispatch(LabVignetteSoftnessInput)

  M_Dispatch(BWStylerFilmTypeChoice)
  M_SetAndRunDispatch(BWStylerColorFilterTypeChoice)
  M_SetAndRunDispatch(BWStylerMultRInput)
  M_SetAndRunDispatch(BWStylerMultGInput)
  M_SetAndRunDispatch(BWStylerMultBInput)
  M_SetAndRunDispatch(BWStylerOpacityInput)

  M_SetAndRunDispatch(SimpleToneRInput)
  M_SetAndRunDispatch(SimpleToneGInput)
  M_SetAndRunDispatch(SimpleToneBInput)

  M_SetAndRunDispatch(Tone1MaskTypeChoice)
  M_SetAndRunDispatch(Tone1AmountInput)
  M_Dispatch(Tone1LowerLimitInput)
  M_Dispatch(Tone1UpperLimitInput)
  M_SetAndRunDispatch(Tone1SoftnessInput)

  M_SetAndRunDispatch(Tone2MaskTypeChoice)
  M_SetAndRunDispatch(Tone2AmountInput)
  M_Dispatch(Tone2LowerLimitInput)
  M_Dispatch(Tone2UpperLimitInput)
  M_SetAndRunDispatch(Tone2SoftnessInput)

  M_SetAndRunDispatch(CrossprocessingModeChoice)
  M_SetAndRunDispatch(CrossprocessingColor1Input)
  M_SetAndRunDispatch(CrossprocessingColor2Input)

  M_SetAndRunDispatch(RGBContrast2AmountInput)
  M_SetAndRunDispatch(RGBContrast2ThresholdInput)

  M_SetAndRunDispatch(TextureOverlayModeChoice)
  M_Dispatch(TextureOverlayMaskChoice)
  M_SetAndRunDispatch(TextureOverlayOpacityInput)
  M_SetAndRunDispatch(TextureOverlaySaturationInput)
  M_SetAndRunDispatch(TextureOverlayExponentInput)
  M_Dispatch(TextureOverlayInnerRadiusInput)
  M_Dispatch(TextureOverlayOuterRadiusInput)
  M_SetAndRunDispatch(TextureOverlayRoundnessInput)
  M_SetAndRunDispatch(TextureOverlayCenterXInput)
  M_SetAndRunDispatch(TextureOverlayCenterYInput)
  M_SetAndRunDispatch(TextureOverlaySoftnessInput)

  M_SetAndRunDispatch(GradualOverlay1Choice)
  M_SetAndRunDispatch(GradualOverlay1AmountInput)
  M_SetAndRunDispatch(GradualOverlay1AngleInput)
  M_Dispatch(GradualOverlay1LowerLevelInput)
  M_Dispatch(GradualOverlay1UpperLevelInput)
  M_SetAndRunDispatch(GradualOverlay1SoftnessInput)

  M_SetAndRunDispatch(GradualOverlay2Choice)
  M_SetAndRunDispatch(GradualOverlay2AmountInput)
  M_SetAndRunDispatch(GradualOverlay2AngleInput)
  M_Dispatch(GradualOverlay2LowerLevelInput)
  M_Dispatch(GradualOverlay2UpperLevelInput)
  M_SetAndRunDispatch(GradualOverlay2SoftnessInput)

  M_SetAndRunDispatch(VignetteModeChoice)
  M_SetAndRunDispatch(VignetteInput)
  M_SetAndRunDispatch(VignetteAmountInput)
  M_Dispatch(VignetteInnerRadiusInput)
  M_Dispatch(VignetteOuterRadiusInput)
  M_SetAndRunDispatch(VignetteRoundnessInput)
  M_SetAndRunDispatch(VignetteCenterXInput)
  M_SetAndRunDispatch(VignetteCenterYInput)
  M_SetAndRunDispatch(VignetteSoftnessInput)

  M_SetAndRunDispatch(GradBlur1Choice)
  M_SetAndRunDispatch(GradBlur1RadiusInput)
  M_SetAndRunDispatch(GradBlur1LowerLevelInput)
  M_SetAndRunDispatch(GradBlur1UpperLevelInput)
  M_SetAndRunDispatch(GradBlur1SoftnessInput)
  M_SetAndRunDispatch(GradBlur1AngleInput)
  M_SetAndRunDispatch(GradBlur1VignetteInput)
  M_SetAndRunDispatch(GradBlur1RoundnessInput)
  M_SetAndRunDispatch(GradBlur1CenterXInput)
  M_SetAndRunDispatch(GradBlur1CenterYInput)
  M_SetAndRunDispatch(GradBlur2Choice)
  M_SetAndRunDispatch(GradBlur2RadiusInput)
  M_SetAndRunDispatch(GradBlur2LowerLevelInput)
  M_SetAndRunDispatch(GradBlur2UpperLevelInput)
  M_SetAndRunDispatch(GradBlur2SoftnessInput)
  M_SetAndRunDispatch(GradBlur2AngleInput)
  M_SetAndRunDispatch(GradBlur2VignetteInput)
  M_SetAndRunDispatch(GradBlur2RoundnessInput)
  M_SetAndRunDispatch(GradBlur2CenterXInput)
  M_SetAndRunDispatch(GradBlur2CenterYInput)

  M_SetAndRunDispatch(SoftglowModeChoice)
  M_SetAndRunDispatch(SoftglowRadiusInput)
  M_SetAndRunDispatch(SoftglowAmountInput)
  M_SetAndRunDispatch(SoftglowContrastInput)
  M_SetAndRunDispatch(SoftglowSaturationInput)

  M_SetAndRunDispatch(Vibrance2Input)
  M_SetAndRunDispatch(Intensity2RedInput)
  M_SetAndRunDispatch(Intensity2GreenInput)
  M_SetAndRunDispatch(Intensity2BlueInput)

  M_SetAndRunDispatch(OutputGammaCompensationCheck)
  M_SetAndRunDispatch(OutputGammaInput)
  M_SetAndRunDispatch(OutputLinearityInput)

  M_SetAndRunDispatch(RGBContrast3AmountInput)
  M_SetAndRunDispatch(RGBContrast3ThresholdInput)

  M_SetAndRunDispatch(WebResizeChoice)
  M_SetAndRunDispatch(WebResizeDimensionChoice)
  M_SetAndRunDispatch(WebResizeBeforeGammaCheck)
  M_SetAndRunDispatch(WebResizeScaleInput)
  M_SetAndRunDispatch(WebResizeFilterChoice)

  M_SetAndRunDispatch(WienerFilter2Check)
  M_SetAndRunDispatch(WienerFilter2UseEdgeMaskCheck)
  M_SetAndRunDispatch(WienerFilter2AmountInput)
  M_SetAndRunDispatch(WienerFilter2GaussianInput)
  M_SetAndRunDispatch(WienerFilter2BoxInput)
  M_SetAndRunDispatch(WienerFilter2LensBlurInput)

//  M_Dispatch(GREYCCheck)
//  M_Dispatch(GREYCFastCheck)
//  M_Dispatch(GREYCInterpolationChoice)
//  M_Dispatch(GREYCAmplitudeInput)
//  M_Dispatch(GREYCIterationsInput)
//  M_Dispatch(GREYCSharpnessInput)
//  M_Dispatch(GREYCAnisotropyInput)
//  M_Dispatch(GREYCAlphaInput)
//  M_Dispatch(GREYCptInput)
//  M_Dispatch(GREYCdaInput)
//  M_Dispatch(GREYCSigmaInput)
//  M_Dispatch(GREYCGaussPrecisionInput)

  M_Dispatch(OutputColorProfileIntentChoice)

  M_JustSetDispatch(SaveQualityInput)
  M_JustSetDispatch(SaveResolutionInput)
  M_Dispatch(SaveFormatChoice)
  M_JustSetDispatch(SaveSamplingChoice)
  M_JustSetDispatch(IncludeExifCheck)
  M_Dispatch(EraseExifThumbnailCheck)
  M_Dispatch(ImageRatingInput)
  M_JustSetDispatch(OutputModeChoice)

  } else {
    fprintf(stderr,"(%s,%d) Unexpected ObjectName '%s'\n",
            __FILE__,__LINE__,ObjectName.toAscii().data());
    assert(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
