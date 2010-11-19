////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>)
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

#include <QFileInfo>
#include <QApplication>

#include "ptConstants.h"
#include "ptError.h"
#include "ptSettings.h"
#include "ptLensfun.h"
#include "ptCurve.h"
#include "ptChannelMixer.h"
#include "ptCimg.h"
//~ #include "ptImageMagick.h"
//~ #include "ptImageMagickC.h"
#include "ptFastBilateral.h"
#include "ptWiener.h"

#include "ptProcessor.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
////////////////////////////////////////////////////////////////////////////////

ptProcessor::ptProcessor(void (*ReportProgress)(const QString Message), void (*UpdateGUI)()) {

  // We work with a callback to avoid dependency on ptMainWindow
  m_ReportProgress = ReportProgress;
  m_UpdateGUI = UpdateGUI;

  // The DcRaw
  m_DcRaw          = NULL;

  // Cached version at different points.
  m_Image_AfterDcRaw       = NULL;
  m_Image_AfterLensfun     = NULL;
  m_Image_AfterRGB         = NULL;
  m_Image_AfterLabCC       = NULL;
  m_Image_AfterLabSN       = NULL;
  m_Image_AfterLabEyeCandy = NULL;
  m_Image_AfterEyeCandy    = NULL;

  //
  m_AutoExposureValue = 0.0;

  // Exif Data and Buffer
  m_ExifBuffer       = NULL;
  m_ExifBufferLength = 0;

  m_ScaleFactor = 0;
}

// Prototype for status report in Viewwindow
void ViewWindowStatusReport(short State);

////////////////////////////////////////////////////////////////////////////////
//
// FindAlpha (helper for ExposureFunction)
// such that f(x)=(1-exp(-Alpha*x)/(1-exp(-Alpha))
// f(x) is then foto curve for which :
// f(0) = 0
// f(1) = 1
// f'(0)= Exposure
//
////////////////////////////////////////////////////////////////////////////////

double FindAlpha(double Exposure) {

  assert(Exposure > 1.0);

  double Alpha;
  double fDerivedAt0;
  if (Exposure<2) {
    Alpha=(Exposure-1)/2;
  } else {
    Alpha=Exposure;
  }
  fDerivedAt0 = Alpha/(1-exp(-Alpha));
  while (fabs(fDerivedAt0-Exposure)>0.001) {
    Alpha = Alpha + (Exposure-fDerivedAt0);
    fDerivedAt0 = Alpha/(1-exp(-Alpha));
  }
  return Alpha;
}

////////////////////////////////////////////////////////////////////////////////
//
// ExposureFunction
// (used to generate an expsure curve)
//
////////////////////////////////////////////////////////////////////////////////

static double Alpha;
static double ExposureForWhichAlphaIsCalculated = -1;

double ExposureFunction(double r, double Exposure, double) {
  if (Exposure != ExposureForWhichAlphaIsCalculated) {
    Alpha = FindAlpha(Exposure);
  }
  return (1-exp(-Alpha*r))/(1-exp(-Alpha));
}

////////////////////////////////////////////////////////////////////////////////
//
// Main Graphical Pipe.
// Look here for all operations and all possible future extensions.
// As well Gui mode as JobMode are sharing this part of the code.
// The idea is to have an image object and operating on it.
// Run the graphical pipe from a certain point on.
//
////////////////////////////////////////////////////////////////////////////////

void ptProcessor::Run(short Phase,
                      short SubPhase,
                      short WithIdentify,
                      short ProcessorMode) {

  printf("(%s,%d) %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);

  static short PreviousProcessorMode = ptProcessorMode_Preview;
  if (PreviousProcessorMode != ptProcessorMode_Preview) {
    // If the previous was a final run we now
    // have to recalculate again everything.
    // (Only important when further processing on same image,
    // but doesn't hurt with a new image as that starts anyway here.
    Phase    = ptProcessorPhase_Raw;
    SubPhase = ptProcessorPhase_Load;
    WithIdentify = 1;
  };
  PreviousProcessorMode = ProcessorMode;

  // Purposes of timing the lenghty operations.
  QTime Timer;
  Timer.start();

  int TmpScaled; // Later used need it out the switch ..

  double  ExposureFactor;
  QString CameraProfileName = Settings->GetString("CameraColorProfile");

  if (!m_DcRaw) goto Exit;

  // Status report
  ::ViewWindowStatusReport(2);

  switch(Phase) {
    case ptProcessorPhase_Raw :

      Settings->ToDcRaw(m_DcRaw);

      // The halfsize setting is only used in Gui interactive.
      // (or thumbnail generation)
      // Otherwise we always start from the full image to operate upon.
      // TODO Improvement possible if final image is less than half
      // the size of the original. With Crop/Rotate/Resize maybe non-trivial
      if (ProcessorMode == ptProcessorMode_Full) {
        m_DcRaw->m_UserSetting_HalfSize = 0;
      }

      // Thumbs even smaller !
      if (ProcessorMode == ptProcessorMode_Thumb) {
        m_DcRaw->m_UserSetting_HalfSize = 5;
      }

      // This will be equivalent to m_PipeSize EXCEPT if overwritten
      // by the FinalRun setting that will be always in full size.
      // 0 for full, 1 for half, 2 for quarter (useable in >> operators)
      Settings->SetValue("Scaled",m_DcRaw->m_UserSetting_HalfSize);

      TRACEMAIN("Starting DcRaw at %d ms.",Timer.elapsed());

      switch (SubPhase) {
        case ptProcessorPhase_Load :

          m_ReportProgress(QObject::tr("Reading RAW file"));

          if (WithIdentify) m_DcRaw->Identify();
          m_DcRaw->RunDcRaw_Phase1();

          // Do not forget !
          // Not in DcRawToSettings as at this point it is
          // not yet influenced by HalfSize. Later it is and
          // it would be wrongly overwritten then.
          Settings->SetValue("ImageW",m_DcRaw->m_ReportedWidth);
          Settings->SetValue("ImageH",m_DcRaw->m_ReportedHeight);

          TRACEKEYVALS("ImageW","%d",Settings->GetInt("ImageW"));
          TRACEKEYVALS("ImageH","%d",Settings->GetInt("ImageH"));

          m_ReportProgress(QObject::tr("Reading exif info"));

          if (ProcessorMode != ptProcessorMode_Thumb) {
            // Read Exif
            ReadExifBuffer();
          }
          TRACEMAIN("Opened RAW at %d ms.",
                    Timer.elapsed());

        case ptProcessorPhase_Demosaic :

          m_ReportProgress(QObject::tr("Demosaicing"));

          // Settings->GetInt("JobMode") causes NoCache
          m_DcRaw->RunDcRaw_Phase2(Settings->GetInt("JobMode"));

          TRACEMAIN("Done Color Scaling and Interpolation at %d ms.",
                    Timer.elapsed());

        case ptProcessorPhase_Highlights :

          m_ReportProgress(QObject::tr("Recovering highlights"));

          // Settings->GetInt("JobMode") causes NoCache
          m_DcRaw->RunDcRaw_Phase3(Settings->GetInt("JobMode"));

          TRACEMAIN("Done Highlights at %d ms.",Timer.elapsed());

          // Already here we want the output profile.
          // It will be applied, not in the pipe but on a
          // tee for preview reasons.
          // Find the output profile if we are in that mode.
          if (Settings->GetInt("CameraColor") == ptCameraColor_Adobe_Profile) {
            QString Identification = m_DcRaw->m_CameraAdobeIdentification;
            for (int i=0;i<Identification.size();i++) {
              if (Identification[i].isSpace()) Identification[i]='_';
              // Some contain '*' which is problem under Windows.
              if (Identification[i] == '*') Identification[i]='_';
            }
            QString InputFileName;
            InputFileName =
              Settings->GetString("StandardAdobeProfilesDirectory");
            InputFileName += "/";
            InputFileName += Identification;
            InputFileName += ".icc";
            QFileInfo PathInfo(InputFileName);
            if (PathInfo.exists() &&
                PathInfo.isFile() &&
                PathInfo.isReadable()) {
              CameraProfileName = PathInfo.absoluteFilePath();
              Settings->SetValue("CameraColorProfile",CameraProfileName);
              TRACEKEYVALS("Found adobe profile","%s",
                           CameraProfileName.toAscii().data());
              TRACEMAIN("Found profile at %d ms.",Timer.elapsed());
            } else {
              QMessageBox::information(0,
                        QObject::tr("Profile not found"),
                        QObject::tr("Profile not found. Reverting to Adobe Matrix.\nYou could try an external profile."));
              TRACEMAIN("Not found profile at %d ms.",Timer.elapsed());
              Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
            }
          } else if (Settings->GetInt("CameraColor") == ptCameraColor_Profile) {
              CameraProfileName = Settings->GetString("CameraColorProfile");
              QFileInfo PathInfo(CameraProfileName);
              if (!(PathInfo.exists() &&
                    PathInfo.isFile() &&
                    PathInfo.isReadable())) {
                QMessageBox::information(0,
                          QObject::tr("Profile not found"),
                          QObject::tr("Profile not found. Reverting to Adobe Matrix.\nYou could try an external profile."));
                TRACEMAIN("Not found profile at %d ms.",Timer.elapsed());
                Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
              }
          } else if (Settings->GetInt("CameraColor") == ptCameraColor_Flat) {
            QString InputFileName;
            InputFileName =
              Settings->GetString("CameraColorProfilesDirectory");
            InputFileName += "/Flat/FlatProfile.icc";
            QFileInfo PathInfo(InputFileName);
            if (PathInfo.exists() &&
                PathInfo.isFile() &&
                PathInfo.isReadable()) {
              CameraProfileName = PathInfo.absoluteFilePath();
              Settings->SetValue("CameraColorProfile",CameraProfileName);
              TRACEMAIN("Found profile at %d ms.",Timer.elapsed());
            } else {
              TRACEMAIN("Not found profile at %d ms.",Timer.elapsed());
              Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
            }
          }

          // We're at the end of the DcRaw part now and capture
          // the image that DcRaw made for us.
          if (!m_Image_AfterDcRaw) m_Image_AfterDcRaw = new ptImage(m_UpdateGUI);

          // Transfer dcraw output to an image, maybe applying a profile
          // and a preprofile.

          m_Image_AfterDcRaw->Set(
            m_DcRaw,
            Settings->GetInt("WorkColor"),
            (Settings->GetInt("CameraColor") == ptCameraColor_Adobe_Matrix) ?
              NULL : CameraProfileName.toAscii().data(),
            Settings->GetInt("CameraColorProfileIntent"),
            Settings->GetInt("CameraColorGamma"));

          Settings->FromDcRaw(m_DcRaw);
          if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Ufraw)
            Settings->SetValue("Exposure",Settings->GetDouble("ExposureNormalization"));

          TRACEMAIN("Done m_Image_AfterDcRaw transfer to GUI at %d ms.",
                     Timer.elapsed());

        case ptProcessorPhase_Lensfun :

          m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags = 0;
          if (Settings->GetInt("EnableLensfun") &&
              (Settings->GetInt("LensfunLensIndex") != -1)) {
            m_ReportProgress(QObject::tr("Applying lens corrections"));
            m_DcRaw->m_UserSetting_photivo_LensfunModifier = lfModifier::Create(
              LensfunData->m_Lenses[Settings->GetInt("LensfunLensIndex")].Lens,
              LensfunData->m_Cameras[Settings->GetInt("LensfunCameraIndex")].
                Camera->CropFactor,
              m_DcRaw->m_OutWidth,
              m_DcRaw->m_OutHeight);
            if (Settings->GetInt("LensfunGeometryEnable")) {
              m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags |=
                LF_MODIFY_GEOMETRY;
              m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags |=
                LF_MODIFY_SCALE;
            }
            if (Settings->GetInt("LensfunTCAEnable")) {
              m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags |=
                LF_MODIFY_TCA;
            }
            if (Settings->GetInt("LensfunVignettingEnable")) {
              m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags |=
                LF_MODIFY_VIGNETTING;
            }
            if (Settings->GetInt("LensfunDistortionEnable")) {
              m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags |=
                LF_MODIFY_DISTORTION;
            }
            // TODO Aperture value or f number ? => review for lensfun.
            m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags =
              m_DcRaw->m_UserSetting_photivo_LensfunModifier->Initialize(
               LensfunData->m_Lenses[Settings->GetInt("LensfunLensIndex")].Lens,
               LF_PF_U16,
               Settings->GetInt("LensfunFocalLength"),
               Settings->GetDouble("LensfunF"),
               Settings->GetDouble("LensfunDistance"),
               Settings->GetDouble("LensfunScale"),
               (lfLensType) Settings->GetInt("LensfunGeometry"),
               m_DcRaw->m_UserSetting_photivo_LensfunModifierFlags,
               0);
          }

          // Settings->GetInt("JobMode") causes NoCache
          m_DcRaw->RunDcRaw_Phase4(Settings->GetInt("JobMode"));

          TRACEMAIN("Done lensfun corrections at %d ms.",Timer.elapsed());

          m_ReportProgress(QObject::tr("RGB to RGB"));

         break;

        default : // Should not happen.
          assert(0);
      }

    case ptProcessorPhase_AfterRAW : // Mostly to deal with non RAW bitmaps

      if (Settings->GetInt("IsRAW")==0) {
        m_ReportProgress(QObject::tr("Loading Bitmap"));

        TRACEMAIN("Start opening bitmap at %d ms.",
                    Timer.elapsed());

        // This will be equivalent to m_PipeSize EXCEPT if overwritten
        // by the FinalRun setting that will be always in full size.
        // 0 for full, 1 for half, 2 for quarter (useable in >> operators)
        Settings->SetValue("Scaled",Settings->GetInt("PipeSize"));

        if (Settings->GetInt("JobMode")==1) // FinalRun!
          Settings->SetValue("Scaled",0);

        if (!m_Image_AfterLensfun) m_Image_AfterLensfun = new ptImage(m_UpdateGUI);

        int Success = 0;

        m_Image_AfterLensfun->ptGMOpenImage(
          (Settings->GetStringList("InputFileNameList"))[0].toAscii().data(),
          Settings->GetInt("WorkColor"),
          Settings->GetInt("PreviewColorProfileIntent"),
          Settings->GetInt("Scaled"),
          Success);

        if (Success == 0) {
          QMessageBox::critical(0,"File not found","File not found!");
          return;
        }

        Settings->SetValue("ImageW",m_Image_AfterLensfun->m_Width);
        Settings->SetValue("ImageH",m_Image_AfterLensfun->m_Height);

        TRACEKEYVALS("ImageW","%d",Settings->GetInt("ImageW"));
        TRACEKEYVALS("ImageH","%d",Settings->GetInt("ImageH"));

        m_ReportProgress(QObject::tr("Reading exif info"));

        if (ProcessorMode != ptProcessorMode_Thumb) {
          // Read Exif
          ReadExifBuffer();
        }
        TRACEMAIN("Opened bitmap at %d ms.", Timer.elapsed());

      } else {
        // We're at the end of the DcRaw part now and capture
        // the image that DcRaw made for us.
        if (!m_Image_AfterLensfun) m_Image_AfterLensfun = new ptImage(m_UpdateGUI);

        // Transfer dcraw output to an image, maybe applying a profile
        // and a preprofile.

        m_Image_AfterLensfun->Set(
          m_DcRaw,
          Settings->GetInt("WorkColor"),
          (Settings->GetInt("CameraColor") == ptCameraColor_Adobe_Matrix) ?
            NULL : CameraProfileName.toAscii().data(),
          Settings->GetInt("CameraColorProfileIntent"),
          Settings->GetInt("CameraColorGamma"));

        TRACEMAIN("Done m_Image_Afterlensfun transfer to GUI at %d ms.",
            Timer.elapsed());
      }

      // Often used.
      TmpScaled = Settings->GetInt("Scaled");

      // Rotation

      if (Settings->ToolIsActive("TabRotation")) {
        m_ReportProgress(QObject::tr("Rotating"));

        m_Image_AfterLensfun->Rotate(Settings->GetDouble("Rotate"));

        TRACEMAIN("Done rotation at %d ms.",Timer.elapsed());
      }

      // Remember sizes after Rotation, also valid if there
      // was no rotation. Expressed in terms of the original
      // non scaled image.
      Settings->SetValue("RotateW",m_Image_AfterLensfun->m_Width << TmpScaled);
      Settings->SetValue("RotateH",m_Image_AfterLensfun->m_Height<< TmpScaled);

      TRACEKEYVALS("RotateW","%d",Settings->GetInt("RotateW"));
      TRACEKEYVALS("RotateH","%d",Settings->GetInt("RotateH"));

      // Crop

      if (Settings->ToolIsActive("TabCrop")) {

        if (((Settings->GetInt("CropX") >> TmpScaled) + (Settings->GetInt("CropW") >> TmpScaled))
              > m_Image_AfterLensfun->m_Width ||
            ((Settings->GetInt("CropY") >> TmpScaled) + (Settings->GetInt("CropH") >> TmpScaled))
              > m_Image_AfterLensfun->m_Height) {
          QMessageBox::information(0,
                                   QObject::tr("Crop outside the image"),
                                   QObject::tr("Crop rectangle too large.\nNo crop, try again."));
          Settings->SetValue("CropX",0);
          Settings->SetValue("CropY",0);
          Settings->SetValue("CropW",0);
          Settings->SetValue("CropH",0);
          Settings->SetValue("Crop",0);
        } else {
          TRACEKEYVALS("CropW","%d",Settings->GetInt("CropW"));
          TRACEKEYVALS("CropH","%d",Settings->GetInt("CropH"));

          m_ReportProgress(QObject::tr("Cropping"));

          m_Image_AfterLensfun->Crop(Settings->GetInt("CropX") >> TmpScaled,
                                     Settings->GetInt("CropY") >> TmpScaled,
                                     Settings->GetInt("CropW") >> TmpScaled,
                                     Settings->GetInt("CropH") >> TmpScaled);

          TRACEMAIN("Done cropping at %d ms.",Timer.elapsed());
        }
      }

      // Remember sizes after Crop, also valid if there
      // was no Crop. Expressed in terms of the original
      // non scaled image.

      Settings->SetValue("CropW",m_Image_AfterLensfun->m_Width << TmpScaled);
      Settings->SetValue("CropH",m_Image_AfterLensfun->m_Height<< TmpScaled);

      TRACEKEYVALS("CropW","%d",Settings->GetInt("CropW"));
      TRACEKEYVALS("CropH","%d",Settings->GetInt("CropH"));

      // set scale factor for size dependend filters
      m_ScaleFactor = 1/powf(2.0, Settings->GetInt("Scaled"));

      // Resize
      if (Settings->ToolIsActive("TabResize")) {
        m_ReportProgress(QObject::tr("Resize image"));

        float WidthIn = m_Image_AfterLensfun->m_Width;

        m_Image_AfterLensfun->ptGMResize(Settings->GetInt("ResizeScale"),
                                         Settings->GetInt("ResizeFilter"));

        m_ScaleFactor = (float) m_Image_AfterLensfun->m_Width/WidthIn/powf(2.0, Settings->GetInt("Scaled"));

        TRACEMAIN("Done resize at %d ms.",Timer.elapsed());
      }

      // Flip

      if (Settings->ToolIsActive("TabFlip")) {
        m_ReportProgress(QObject::tr("Flip image"));

        m_Image_AfterLensfun->Flip(Settings->GetInt("FlipMode"));

        TRACEMAIN("Done flip at %d ms.",Timer.elapsed());
      }

      // Geometry block
      // This will skip the rest of the pipe, to make geometry adjustments easier.

      if (Settings->ToolIsActive("TabBlock")){ // &&
        //~ MainWindow->GetCurrentTab() == ptGeometryTab) {
        if (!m_Image_AfterRGB) m_Image_AfterRGB = new ptImage(m_UpdateGUI);
        m_Image_AfterRGB->Set(m_Image_AfterLensfun);
        if (!m_Image_AfterLabCC) m_Image_AfterLabCC = new ptImage(m_UpdateGUI);
        m_Image_AfterLabCC->Set(m_Image_AfterLensfun);
        if (!m_Image_AfterLabSN) m_Image_AfterLabSN = new ptImage(m_UpdateGUI);
        m_Image_AfterLabSN->Set(m_Image_AfterLensfun);
        if (!m_Image_AfterEyeCandy) m_Image_AfterEyeCandy = new ptImage(m_UpdateGUI);
        m_Image_AfterEyeCandy->Set(m_Image_AfterLensfun);
        goto Exit;
      }

      // Calculate the autoexposure required value at this point.
      if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Auto) {
        m_ReportProgress(QObject::tr("Calculate auto exposure"));
        m_AutoExposureValue = CalculateAutoExposure(m_Image_AfterLensfun);
        Settings->SetValue("Exposure",m_AutoExposureValue);
      }
      if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Ufraw)
          Settings->SetValue("Exposure",Settings->GetDouble("ExposureNormalization"));

      m_ReportProgress(QObject::tr("Next"));

    case ptProcessorPhase_RGB : // Run everything in RGB.

      if (Settings->GetInt("JobMode")) {
        m_Image_AfterRGB = m_Image_AfterLensfun; // Job mode -> no cache
      } else {
        if (!m_Image_AfterRGB) m_Image_AfterRGB = new ptImage(m_UpdateGUI);
        m_Image_AfterRGB->Set(m_Image_AfterLensfun);
      }

      // Channel mixing.

      if (Settings->ToolIsActive("TabChannelMixer")) {

        m_ReportProgress(QObject::tr("Channel Mixing"));

        m_Image_AfterRGB->MixChannels(ChannelMixer->m_Mixer);
      }

      // Highlights

      if (Settings->ToolIsActive("TabHighlights")) {

        ptCurve* HighlightsCurve = new ptCurve();
        HighlightsCurve->m_Type = ptCurveType_Anchor;
        HighlightsCurve->m_IntendedChannel = ptCurveChannel_RGB;
        HighlightsCurve->m_NrAnchors=12;
        for (int i=0; i<11; i++) {
          HighlightsCurve->m_XAnchor[i]=(double) i/33.0;
          HighlightsCurve->m_YAnchor[i]=(double) i/33.0;
        }
        if (Settings->GetDouble("HighlightsR") < 0) {
          HighlightsCurve->m_XAnchor[11]=1.0;
          HighlightsCurve->m_YAnchor[11]=1.0+0.5*Settings->GetDouble("HighlightsR");
          HighlightsCurve->SetCurveFromAnchors();
          m_Image_AfterRGB->ApplyCurve(HighlightsCurve,1);
        } else if (Settings->GetDouble("HighlightsR") > 0) {
          HighlightsCurve->m_XAnchor[11]=1.0-0.5*Settings->GetDouble("HighlightsR");
          HighlightsCurve->m_YAnchor[11]=1.0;
          HighlightsCurve->SetCurveFromAnchors();
          m_Image_AfterRGB->ApplyCurve(HighlightsCurve,1);
        }
        if (Settings->GetDouble("HighlightsG") < 0) {
          HighlightsCurve->m_XAnchor[11]=1.0;
          HighlightsCurve->m_YAnchor[11]=1.0+0.5*Settings->GetDouble("HighlightsG");
          HighlightsCurve->SetCurveFromAnchors();
          m_Image_AfterRGB->ApplyCurve(HighlightsCurve,2);
        } else if (Settings->GetDouble("HighlightsG") > 0) {
          HighlightsCurve->m_XAnchor[11]=1.0-0.5*Settings->GetDouble("HighlightsG");
          HighlightsCurve->m_YAnchor[11]=1.0;
          HighlightsCurve->SetCurveFromAnchors();
          m_Image_AfterRGB->ApplyCurve(HighlightsCurve,2);
        }
        if (Settings->GetDouble("HighlightsB") < 0) {
          HighlightsCurve->m_XAnchor[11]=1.0;
          HighlightsCurve->m_YAnchor[11]=1.0+0.5*Settings->GetDouble("HighlightsB");
          HighlightsCurve->SetCurveFromAnchors();
          m_Image_AfterRGB->ApplyCurve(HighlightsCurve,4);
        } else if (Settings->GetDouble("HighlightsB") > 0) {
          HighlightsCurve->m_XAnchor[11]=1.0-0.5*Settings->GetDouble("HighlightsB");
          HighlightsCurve->m_YAnchor[11]=1.0;
          HighlightsCurve->SetCurveFromAnchors();
          m_Image_AfterRGB->ApplyCurve(HighlightsCurve,4);
        }
        delete HighlightsCurve;
      }

      // Vibrance

      if (Settings->GetInt("Vibrance") &&
          Settings->ToolIsActive("TabColorIntensity")) {

        m_ReportProgress(QObject::tr("Vibrance"));
        int Value = (Settings->GetInt("Vibrance"));
        double VibranceMixer[3][3];

        VibranceMixer[0][0] = 1.0+(Value/150.0);
        VibranceMixer[0][1] = -(Value/300.0);
        VibranceMixer[0][2] = VibranceMixer[0][1];
        VibranceMixer[1][0] = VibranceMixer[0][1];
        VibranceMixer[1][1] = VibranceMixer[0][0];
        VibranceMixer[1][2] = VibranceMixer[0][1];
        VibranceMixer[2][0] = VibranceMixer[0][1];
        VibranceMixer[2][1] = VibranceMixer[0][1];
        VibranceMixer[2][2] = VibranceMixer[0][0];

        m_Image_AfterRGB->MixChannels(VibranceMixer);
      }

      if ((Settings->GetInt("IntensityRed")   ||
          Settings->GetInt("IntensityGreen") ||
          Settings->GetInt("IntensityBlue")) &&
          Settings->ToolIsActive("TabColorIntensity")) {

        m_ReportProgress(QObject::tr("IntensityRGB"));
        int ValueR=(Settings->GetInt("IntensityRed"));
        int ValueG=(Settings->GetInt("IntensityGreen"));
        int ValueB=(Settings->GetInt("IntensityBlue"));
        double IntensityMixer[3][3];
        IntensityMixer[0][0] = 1.0+(ValueR/150.0);
        IntensityMixer[0][1] = -(ValueR/300.0);
        IntensityMixer[0][2] = IntensityMixer[0][1];
        IntensityMixer[1][0] = -(ValueG/300.0);
        IntensityMixer[1][1] = 1.0+(ValueG/150.0);;
        IntensityMixer[1][2] = IntensityMixer[1][0];
        IntensityMixer[2][0] = -(ValueB/300.0);
        IntensityMixer[2][1] = IntensityMixer[2][0];
        IntensityMixer[2][2] = 1.0+(ValueB/150.0);;

        m_Image_AfterRGB->MixChannels(IntensityMixer);
      }

      // Brightness

      if (Settings->ToolIsActive("TabBrightness")) {
        m_ReportProgress(QObject::tr("Brightness"));
        ptCurve* CatchWhiteCurve = new ptCurve();
        CatchWhiteCurve->m_Type = ptCurveType_Anchor;
        CatchWhiteCurve->m_IntendedChannel = ptCurveChannel_RGB;
        if (Settings->GetDouble("CatchBlack") > 0) {
          CatchWhiteCurve->m_XAnchor[0]=0.0;
          CatchWhiteCurve->m_YAnchor[0]=0.02*Settings->GetDouble("CatchBlack");
        } else {
          CatchWhiteCurve->m_XAnchor[0]=-0.02*Settings->GetDouble("CatchBlack");
          CatchWhiteCurve->m_YAnchor[0]=0.0;
        }
        for (int i=3; i<12; i++) {
          CatchWhiteCurve->m_XAnchor[i-2]=(double) i/20.0;
          CatchWhiteCurve->m_YAnchor[i-2]=(double) i/20.0;
        }
        if (Settings->GetDouble("CatchWhite") > 0) {
          CatchWhiteCurve->m_XAnchor[10]=1.0;
          CatchWhiteCurve->m_YAnchor[10]=1.0-0.40*Settings->GetDouble("CatchWhite");
        } else {
          CatchWhiteCurve->m_XAnchor[10]=1.0+0.40*Settings->GetDouble("CatchWhite");
          CatchWhiteCurve->m_YAnchor[10]=1.0;
        }
        CatchWhiteCurve->m_NrAnchors=11;
        CatchWhiteCurve->SetCurveFromAnchors();
        m_Image_AfterRGB->ApplyCurve(CatchWhiteCurve,7);

        ptCurve* ExposureGainCurve = new ptCurve();
        ExposureGainCurve->m_Type = ptCurveType_Anchor;
        ExposureGainCurve->m_IntendedChannel = ptCurveChannel_RGB;
        ExposureGainCurve->m_XAnchor[0]=0.0;
        ExposureGainCurve->m_YAnchor[0]=0.0;
        ExposureGainCurve->m_XAnchor[1]=0.5-0.2*Settings->GetDouble("ExposureGain");
        ExposureGainCurve->m_YAnchor[1]=0.5+0.2*Settings->GetDouble("ExposureGain");
        ExposureGainCurve->m_XAnchor[2]=1.0;
        ExposureGainCurve->m_YAnchor[2]=1.0;
        ExposureGainCurve->m_NrAnchors=3;
        ExposureGainCurve->SetCurveFromAnchors();
        // Merging these two curves results in bad results, no clue
        // ExposureGainCurve->ApplyCurve(CatchWhiteCurve,0);

        m_Image_AfterRGB->ApplyCurve(ExposureGainCurve,7);
        delete ExposureGainCurve;
        delete CatchWhiteCurve;
      }

      // Exposure and Exposure gain

      if (Settings->ToolIsActive("TabExposure")) {

        m_ReportProgress(QObject::tr("Correcting Exposure"));

        // From EV to factor.
        ExposureFactor = pow(2,Settings->GetDouble("Exposure"));

        TRACEKEYVALS("ExposureFactor","%f",ExposureFactor);

        if (Settings->GetInt("ExposureClipMode") != ptExposureClipMode_Curve ||
            (ExposureFactor <= 1.0) ) {
          m_Image_AfterRGB->Expose(ExposureFactor,
                                   Settings->GetInt("ExposureClipMode"));
        } else {
          if (!ExposureCurve) ExposureCurve = new ptCurve();
          ExposureCurve->SetCurveFromFunction(ExposureFunction,
                                              ExposureFactor,
                                              0);
          m_Image_AfterRGB->ApplyCurve(ExposureCurve,7);
        }
      }

      // Reinhard 05

      if (Settings->ToolIsActive("TabReinhard05")) {

        m_ReportProgress(QObject::tr("Brighten"));

        m_Image_AfterRGB->Reinhard05(Settings->GetDouble("Reinhard05Brightness"),
                                     Settings->GetDouble("Reinhard05Chroma"),
                                     Settings->GetDouble("Reinhard05Light"));
      }

      // Gamma tool

      if (Settings->ToolIsActive("TabGammaTool")) {

        m_ReportProgress(QObject::tr("Applying RGB Gamma"));

        if (!RGBGammaCurve) RGBGammaCurve = new ptCurve();
        RGBGammaCurve->SetCurveFromFunction(
                                   GammaTool,
                                   Settings->GetDouble("RGBGammaAmount"),
                                   Settings->GetDouble("RGBGammaLinearity"));
        m_Image_AfterRGB->ApplyCurve(RGBGammaCurve,7);
      }

      // Normalization

      if (Settings->ToolIsActive("TabNormalization")) {

        m_ReportProgress(QObject::tr("Normalization"));

        m_Image_AfterRGB->ptGMNormalize(Settings->GetDouble("NormalizationOpacity"));
//        ptIMContrastStretch(m_Image_AfterRGB,
//                           Settings->GetDouble("NormalizationBlackPoint"),
//                           Settings->GetDouble("NormalizationWhitePoint"),
//                           Settings->GetDouble("NormalizationOpacity"));

      }

      // Color Enhancement

      if (Settings->ToolIsActive("TabColorEnhance")) {
        m_ReportProgress(QObject::tr("Color enhance"));

        m_Image_AfterRGB->ColorEnhance(Settings->GetDouble("ColorEnhanceShadows"),
                                       Settings->GetDouble("ColorEnhanceHighlights"));
      }

      // LMHLightRecovery

      if (Settings->GetInt("LMHLightRecovery1MaskType") &&
          Settings->ToolIsActive("TabRGBRecovery")) {

        m_ReportProgress(QObject::tr("Local Exposure"));

        m_Image_AfterRGB->LMHLightRecovery(
          Settings->GetInt("LMHLightRecovery1MaskType"),
          Settings->GetDouble("LMHLightRecovery1Amount"),
          pow(Settings->GetDouble("LMHLightRecovery1LowerLimit"),
              Settings->GetDouble("InputPowerFactor")),
          pow(Settings->GetDouble("LMHLightRecovery1UpperLimit"),
              Settings->GetDouble("InputPowerFactor")),
          Settings->GetDouble("LMHLightRecovery1Softness"));
      }

      if (Settings->GetInt("LMHLightRecovery2MaskType") &&
          Settings->ToolIsActive("TabRGBRecovery")) {

        m_ReportProgress(QObject::tr("Local Exposure"));

        m_Image_AfterRGB->LMHLightRecovery(
          Settings->GetInt("LMHLightRecovery2MaskType"),
          Settings->GetDouble("LMHLightRecovery2Amount"),
          pow(Settings->GetDouble("LMHLightRecovery2LowerLimit"),
              Settings->GetDouble("InputPowerFactor")),
          pow(Settings->GetDouble("LMHLightRecovery2UpperLimit"),
              Settings->GetDouble("InputPowerFactor")),
          Settings->GetDouble("LMHLightRecovery2Softness"));
      }

      // RGB Texturecontrast

      if (Settings->ToolIsActive("TabRGBTextureContrast")) {

        m_ReportProgress(QObject::tr("RGB Texture contrast"));

        m_Image_AfterRGB->TextureContrast(Settings->GetDouble("RGBTextureContrastThreshold")*m_ScaleFactor,
          Settings->GetDouble("RGBTextureContrastSoftness"),
          Settings->GetDouble("RGBTextureContrastAmount"),
          Settings->GetDouble("RGBTextureContrastOpacity"),
          Settings->GetDouble("RGBTextureContrastEdgeControl"),
          Settings->GetDouble("RGBTextureContrastMasking")*m_ScaleFactor);
        TRACEMAIN("Done RGB texture contrast at %d ms.",Timer.elapsed());
      }

      // Microcontrast

     if (Settings->ToolIsActive("TabRGBLocalContrast1")) {

        m_ReportProgress(QObject::tr("Microcontrast 1"));

        m_Image_AfterRGB->Microcontrast(
          Settings->GetInt("Microcontrast1Radius")*m_ScaleFactor,
          Settings->GetDouble("Microcontrast1Amount"),
          Settings->GetDouble("Microcontrast1Opacity"),
          Settings->GetDouble("Microcontrast1HaloControl"),
          Settings->GetInt("Microcontrast1MaskType"),
          pow(Settings->GetDouble("Microcontrast1LowerLimit"),Settings->GetDouble("InputPowerFactor")),
          pow(Settings->GetDouble("Microcontrast1UpperLimit"),Settings->GetDouble("InputPowerFactor")),
          Settings->GetDouble("Microcontrast1Softness"));
      }

     if (Settings->ToolIsActive("TabRGBLocalContrast2")) {

        m_ReportProgress(QObject::tr("Microcontrast 2"));

        m_Image_AfterRGB->Microcontrast(
          Settings->GetInt("Microcontrast2Radius")*m_ScaleFactor,
          Settings->GetDouble("Microcontrast2Amount"),
          Settings->GetDouble("Microcontrast2Opacity"),
          Settings->GetDouble("Microcontrast2HaloControl"),
          Settings->GetInt("Microcontrast2MaskType"),
          pow(Settings->GetDouble("Microcontrast2LowerLimit"),Settings->GetDouble("InputPowerFactor")),
          pow(Settings->GetDouble("Microcontrast2UpperLimit"),Settings->GetDouble("InputPowerFactor")),
          Settings->GetDouble("Microcontrast2Softness"));
      }

      // RGB Contrast.

      if (Settings->ToolIsActive("TabRGBContrast")) {

        m_ReportProgress(QObject::tr("Applying RGB Contrast"));

        m_Image_AfterRGB->SigmoidalContrast(Settings->GetDouble("RGBContrastAmount"),
                                            Settings->GetDouble("RGBContrastThreshold"));
      }

      // Levels
      if (Settings->ToolIsActive("TabRGBLevels")) {

        m_ReportProgress(QObject::tr("Levels"));

        double BP = Settings->GetDouble("LevelsBlackPoint");
        double WP = Settings->GetDouble("LevelsWhitePoint");
        BP = (BP>0)? pow(BP,Settings->GetDouble("InputPowerFactor")):
                    -pow(-BP,Settings->GetDouble("InputPowerFactor"));
        WP = pow(WP,Settings->GetDouble("InputPowerFactor"));
        m_Image_AfterRGB->Levels(BP,WP);
      }

      // RGB Curves.

      if (Settings->ToolIsActive("TabRGBCurve")) {

        m_ReportProgress(QObject::tr("Applying RGB curve"));

        m_Image_AfterRGB->ApplyCurve(Curve[ptCurveChannel_RGB],7);

        TRACEMAIN("Done RGB Curve at %d ms.",Timer.elapsed());

      }

     case ptProcessorPhase_LabCC : // Run everything in LAB.

      m_ReportProgress(QObject::tr("Phase Lab"));

      if (Settings->GetInt("JobMode")) {
        m_Image_AfterLabCC = m_Image_AfterRGB; // Job mode -> no cache
      } else {
        if (!m_Image_AfterLabCC) m_Image_AfterLabCC = new ptImage(m_UpdateGUI);
        m_Image_AfterLabCC->Set(m_Image_AfterRGB);
      }

      // LAB Transform, this has to be the first filter
      // in the LAB series, because it needs RGB input

      if (Settings->ToolIsActive("TabLABTransform")) {

        m_ReportProgress(QObject::tr("Lab transform"));

        m_Image_AfterLabCC->LABTransform(Settings->GetInt("LABTransform"));
      }

      // Shadows Highlights

      if (Settings->ToolIsActive("TabLABShadowsHighlights")) {

        m_ReportProgress(QObject::tr("Shadows and Highlights"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->ShadowsHighlights(Curve[ptCurveChannel_ShadowsHighlights],
                                              Settings->GetDouble("ShadowsHighlightsRadius")*m_ScaleFactor,
                                              Settings->GetDouble("ShadowsHighlightsCoarse"),
                                              Settings->GetDouble("ShadowsHighlightsFine"));
      }

      // LabLMHLightRecovery

      if (Settings->GetInt("LabLMHLightRecovery1MaskType") &&
          Settings->ToolIsActive("TabLABRecovery")) {

        m_ReportProgress(QObject::tr("LabLocal Exposure"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->LMHLightRecovery(
          Settings->GetInt("LabLMHLightRecovery1MaskType"),
          Settings->GetDouble("LabLMHLightRecovery1Amount"),
          Settings->GetDouble("LabLMHLightRecovery1LowerLimit"),
          Settings->GetDouble("LabLMHLightRecovery1UpperLimit"),
          Settings->GetDouble("LabLMHLightRecovery1Softness"));
      }

      if (Settings->GetInt("LabLMHLightRecovery2MaskType") &&
          Settings->ToolIsActive("TabLABRecovery")) {

        m_ReportProgress(QObject::tr("LabLocal Exposure"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->LMHLightRecovery(
          Settings->GetInt("LabLMHLightRecovery2MaskType"),
          Settings->GetDouble("LabLMHLightRecovery2Amount"),
          Settings->GetDouble("LabLMHLightRecovery2LowerLimit"),
          Settings->GetDouble("LabLMHLightRecovery2UpperLimit"),
          Settings->GetDouble("LabLMHLightRecovery2Softness"));
      }

      // Dynamic Range Compression

      if (Settings->ToolIsActive("TabLABDRC")) {

        m_ReportProgress(QObject::tr("Dynamic Range Compression"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->DRC(Settings->GetDouble("DRCAlpha"),
                                Settings->GetDouble("DRCBeta"),
                                Settings->GetDouble("DRCColor"));
      }

      // Texture curve

      if (Settings->ToolIsActive("TabLABTextureCurve")) {

        m_ReportProgress(QObject::tr("Texture curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }
        m_Image_AfterLabCC->ApplyTextureCurve(Curve[ptCurveChannel_Texture],
                                              Settings->GetInt("TextureCurveType"),
                                              (int) (logf(m_ScaleFactor)/logf(0.5)));
        TRACEMAIN("Done texture curve at %d ms.",Timer.elapsed());
      }

      // Texturecontrast

      if (Settings->ToolIsActive("TabLABTexture1")) {

        m_ReportProgress(QObject::tr("Texture contrast 1"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }
        m_Image_AfterLabCC->TextureContrast(Settings->GetDouble("TextureContrast1Threshold")*m_ScaleFactor,
          Settings->GetDouble("TextureContrast1Softness"),
          Settings->GetDouble("TextureContrast1Amount"),
          Settings->GetDouble("TextureContrast1Opacity"),
          Settings->GetDouble("TextureContrast1EdgeControl"),
          Settings->GetDouble("TextureContrast1Masking")*m_ScaleFactor);
        TRACEMAIN("Done texture contrast 1 at %d ms.",Timer.elapsed());
      }

      if (Settings->ToolIsActive("TabLABTexture2")) {

        m_ReportProgress(QObject::tr("Texture contrast 2"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }
        m_Image_AfterLabCC->TextureContrast(Settings->GetDouble("TextureContrast2Threshold")*m_ScaleFactor,
          Settings->GetDouble("TextureContrast2Softness"),
          Settings->GetDouble("TextureContrast2Amount"),
          Settings->GetDouble("TextureContrast2Opacity"),
          Settings->GetDouble("TextureContrast2EdgeControl"),
          Settings->GetDouble("TextureContrast2Masking")*m_ScaleFactor);
        TRACEMAIN("Done texture contrast 2 at %d ms.",Timer.elapsed());
      }

      // Microcontrast

      if (Settings->ToolIsActive("TabLABLocalContrast1")) {

        m_ReportProgress(QObject::tr("LabMicrocontrast 1"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->Microcontrast(
          Settings->GetInt("LabMicrocontrast1Radius")*m_ScaleFactor,
          Settings->GetDouble("LabMicrocontrast1Amount"),
          Settings->GetDouble("LabMicrocontrast1Opacity"),
          Settings->GetDouble("LabMicrocontrast1HaloControl"),
          Settings->GetInt("LabMicrocontrast1MaskType"),
          Settings->GetDouble("LabMicrocontrast1LowerLimit"),
          Settings->GetDouble("LabMicrocontrast1UpperLimit"),
          Settings->GetDouble("LabMicrocontrast1Softness"));
      }

      if (Settings->ToolIsActive("TabLABLocalContrast2")) {

        m_ReportProgress(QObject::tr("LabMicrocontrast 2"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->Microcontrast(
          Settings->GetInt("LabMicrocontrast2Radius")*m_ScaleFactor,
          Settings->GetDouble("LabMicrocontrast2Amount"),
          Settings->GetDouble("LabMicrocontrast2Opacity"),
          Settings->GetDouble("LabMicrocontrast2HaloControl"),
          Settings->GetInt("LabMicrocontrast2MaskType"),
          Settings->GetDouble("LabMicrocontrast2LowerLimit"),
          Settings->GetDouble("LabMicrocontrast2UpperLimit"),
          Settings->GetDouble("LabMicrocontrast2Softness"));
      }


      // Local Contrast Stretch

      if (Settings->ToolIsActive("TabLABLCStretch1")) {

        m_ReportProgress(QObject::tr("Local Contrast 1"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->Localcontrast(Settings->GetInt("LC1Radius")*m_ScaleFactor,
                    Settings->GetDouble("LC1Opacity"),
                    Settings->GetDouble("LC1m"),
                    Settings->GetDouble("LC1Feather"),1);
      }

      if (Settings->ToolIsActive("TabLABLCStretch2")) {

        m_ReportProgress(QObject::tr("Local Contrast 2"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->Localcontrast(Settings->GetInt("LC2Radius")*m_ScaleFactor,
                    Settings->GetDouble("LC2Opacity"),
                    Settings->GetDouble("LC2m"),
                    Settings->GetDouble("LC2Feather"),1);
      }


      // L Contrast

      if (Settings->ToolIsActive("TabLABContrast")) {

        m_ReportProgress(QObject::tr("Applying Lab contrast"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

         m_Image_AfterLabCC->SigmoidalContrast(Settings->GetDouble("ContrastAmount"),
                                               Settings->GetDouble("ContrastThreshold"),
                                               1);
      }

      // Saturation

      if (Settings->ToolIsActive("TabLABSaturation")) {

        m_ReportProgress(QObject::tr("Applying Lab saturation"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->SigmoidalContrast(Settings->GetDouble("SaturationAmount"),0.5,6);
      }

      // Color Boost

      if (Settings->ToolIsActive("TabLABColorBoost")) {

        m_ReportProgress(QObject::tr("Applying Color Boost"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabCC->ColorBoost(Settings->GetDouble("ColorBoostValueA"),Settings->GetDouble("ColorBoostValueB"));
      }


      // Levels
      if (Settings->ToolIsActive("TabLABLevels")) {

        m_ReportProgress(QObject::tr("LabLevels"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabCC->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabCC->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        //~ double BP = Settings->GetDouble("LabLevelsBlackPoint");
        //~ double WP = Settings->GetDouble("LabLevelsWhitePoint");
        //~ BP = (BP>0)? pow(BP,Settings->GetDouble("InputPowerFactor")):
                    //~ -pow(-BP,Settings->GetDouble("InputPowerFactor"));
        //~ WP = pow(WP,Settings->GetDouble("InputPowerFactor"));
        m_Image_AfterLabCC->Levels(Settings->GetDouble("LabLevelsBlackPoint"),Settings->GetDouble("LabLevelsWhitePoint"));
        //~ m_Image_AfterLab->Levels(BP,WP);
      }


     case ptProcessorPhase_LabSN : // Run everything in LABSN.

      m_ReportProgress(QObject::tr("Phase Lab2"));

      if (Settings->GetInt("JobMode")) {
        m_Image_AfterLabSN = m_Image_AfterLabCC; // Job mode -> no cache
      } else {
        if (!m_Image_AfterLabSN) m_Image_AfterLabSN = new ptImage(m_UpdateGUI);
        m_Image_AfterLabSN->Set(m_Image_AfterLabCC);
      }

      // Edge avoiding wavelet filter
      if (Settings->ToolIsActive("TabLABEAW")) {

        m_ReportProgress(QObject::tr("Edge avoiding wavelets"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabSN->EAWChannel((int)(logf(m_ScaleFactor)/logf(0.5)),
                                       Settings->GetDouble("EAWLevel1"),
                                       Settings->GetDouble("EAWLevel2"),
                                       Settings->GetDouble("EAWLevel3"),
                                       Settings->GetDouble("EAWLevel4"),
                                       Settings->GetDouble("EAWLevel5"),
                                       Settings->GetDouble("EAWLevel6"));

        TRACEMAIN("Done EAW at %d ms.",Timer.elapsed());
      }

      // GreyCStoration on L

      if (Settings->ToolIsActive("TabLABGreyC") &&
          Settings->GetInt("GREYCLab")<=2) {

        m_ReportProgress(QObject::tr("GreyCStoration on L"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

      TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        ptGreycStorationLab(m_Image_AfterLabSN,
                         m_ReportProgress,
                         Settings->GetInt("GREYCLabIterations"),
                         Settings->GetDouble("GREYCLabAmplitude"),
                         Settings->GetDouble("GREYCLabSharpness"),
                         Settings->GetDouble("GREYCLabAnisotropy"),
                         Settings->GetDouble("GREYCLabAlpha"),
                         Settings->GetDouble("GREYCLabSigma"),
                         Settings->GetDouble("GREYCLabdl"),
                         Settings->GetInt("GREYCLabda"),
                         Settings->GetDouble("GREYCLabGaussPrecision"),
                         Settings->GetInt("GREYCLabInterpolation"),
                         Settings->GetInt("GREYCLabFast"),
             Settings->GetInt("GREYCLabMaskType"),
             Settings->GetDouble("GREYCLabOpacity"));

        TRACEMAIN("Done GreyCStoration on L at %d ms.",Timer.elapsed());

      } else if (Settings->ToolIsActive("TabLABGreyC") &&
                 Settings->GetInt("GREYCLab") == ptEnable_ShowMask) {
         m_ReportProgress(QObject::tr("GreyCStoration on L"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

        TRACEMAIN("Done conversion to LAB at %d ms.",
        Timer.elapsed());

        }
        ptCimgEdgeTensors(m_Image_AfterLabSN,
                          Settings->GetDouble("GREYCLabSharpness"),
                          Settings->GetDouble("GREYCLabAnisotropy"),
                          Settings->GetDouble("GREYCLabAlpha"),
                          Settings->GetDouble("GREYCLabSigma"),
                          0,
                          Settings->GetInt("GREYCLabMaskType"));
      }

      // Wavelet denoise

      if (Settings->GetDouble("WaveletDenoiseL") &&
          Settings->ToolIsActive("TabWaveletDenoise")) {

        m_ReportProgress(QObject::tr("Wavelet L denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabSN->WaveletDenoise(
                                1,
                                Settings->GetDouble("WaveletDenoiseL")/((logf(m_ScaleFactor)/logf(0.5))+1.0),
            Settings->GetDouble("WaveletDenoiseLSoftness"),
            1,
            Settings->GetDouble("WaveletDenoiseLSharpness"),
            Settings->GetDouble("WaveletDenoiseLAnisotropy"),
            Settings->GetDouble("WaveletDenoiseLAlpha"),
            Settings->GetDouble("WaveletDenoiseLSigma"));
        // The scaling of the inputvalue is artificial, to get roughly the same image in each pipesize with the same value...
        TRACEMAIN("Done Luminance wavelet denoise at %d ms.",Timer.elapsed());
      }

      if (Settings->GetDouble("WaveletDenoiseA") &&
          Settings->ToolIsActive("TabWaveletDenoise")) {

        m_ReportProgress(QObject::tr("Wavelet A denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        m_Image_AfterLabSN->WaveletDenoise(
                                2,
                                Settings->GetDouble("WaveletDenoiseA")/((logf(m_ScaleFactor)/logf(0.5))+1.0),
        Settings->GetDouble("WaveletDenoiseASoftness"));

        TRACEMAIN("Done A wavelet denoise at %d ms.",Timer.elapsed());
      }

      if (Settings->GetDouble("WaveletDenoiseB") &&
          Settings->ToolIsActive("TabWaveletDenoise")) {

        m_ReportProgress(QObject::tr("Wavelet B denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        m_Image_AfterLabSN->WaveletDenoise(
                                4,
                                Settings->GetDouble("WaveletDenoiseB")/((logf(m_ScaleFactor)/logf(0.5))+1.0),
            Settings->GetDouble("WaveletDenoiseBSoftness"));

        TRACEMAIN("Done B wavelet denoise at %d ms.",Timer.elapsed());
      }

      // Bilateral filter on L
      if (Settings->ToolIsActive("TabLuminanceDenoise")) {

        m_ReportProgress(QObject::tr("Luminance denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabSN->BilateralDenoise(Settings->GetDouble("BilateralLSigmaS")*m_ScaleFactor,
               Settings->GetDouble("BilateralLSigmaR")/10.0,
               Settings->GetDouble("BilateralLOpacity"),
               Settings->GetDouble("BilateralLUseMask")*m_ScaleFactor);

        TRACEMAIN("Done Luminance denoise at %d ms.",Timer.elapsed());
      }

      // Pyramid denoise filter
      if (Settings->ToolIsActive("TabPyramidDenoise")) {

        m_ReportProgress(QObject::tr("Pyramid denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabSN->dirpyrLab_denoise(
            (int)(Settings->GetInt("PyrDenoiseLAmount")/powf(3.0,(logf(m_ScaleFactor)/logf(0.5)))),
            Settings->GetInt("PyrDenoiseABAmount"),
            Settings->GetDouble("PyrDenoiseGamma")/3.0,
            Settings->GetInt("PyrDenoiseLevels"),
            (int)(logf(m_ScaleFactor)/logf(0.5)));

        TRACEMAIN("Done Pyramid denoise at %d ms.",Timer.elapsed());
      }

      // Bilateral filter on AB
      if (Settings->ToolIsActive("TabColorDenoise") &&
          Settings->GetDouble("BilateralASigmaR")) {

        m_ReportProgress(QObject::tr("Color A denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        ptFastBilateralChannel(m_Image_AfterLabSN,
             Settings->GetDouble("BilateralASigmaS")*m_ScaleFactor,
             Settings->GetDouble("BilateralASigmaR")/10.0,
             2,
             2);

        TRACEMAIN("Done A denoise at %d ms.",Timer.elapsed());
      }

      if (Settings->ToolIsActive("TabColorDenoise") &&
          Settings->GetDouble("BilateralBSigmaR")) {

        m_ReportProgress(QObject::tr("Color B denoising"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        ptFastBilateralChannel(m_Image_AfterLabSN,
             Settings->GetDouble("BilateralBSigmaS")*m_ScaleFactor,
             Settings->GetDouble("BilateralBSigmaR")/10.0,
             2,
             4);

        TRACEMAIN("Done B denoise at %d ms.",Timer.elapsed());
      }

      // Detail / denoise curve

      if (Settings->ToolIsActive("TabDetailCurve")) {

        m_ReportProgress(QObject::tr("Detail curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }
        m_Image_AfterLabSN->MLMicroContrast(0.15,
                                            Settings->GetDouble("DetailCurveScaling"),
                                            Settings->GetDouble("DetailCurveWeight"),
                                            Curve[ptCurveChannel_Denoise],
                                            Settings->GetInt("DenoiseCurveType"));

        m_Image_AfterLabSN->HotpixelReduction(Settings->GetDouble("DetailCurveHotpixel"));

        TRACEMAIN("Done detail curve at %d ms.",Timer.elapsed());
      }


      // Gradient Sharpen

      if (Settings->ToolIsActive("TabLABGradientSharpen")) {

        m_ReportProgress(QObject::tr("Gradient Sharpen"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabSN->GradientSharpen(Settings->GetInt("GradientSharpenPasses"),
                                            Settings->GetDouble("GradientSharpenStrength"));

        m_Image_AfterLabSN->MLMicroContrast(Settings->GetDouble("MLMicroContrastStrength"),
                                            Settings->GetDouble("MLMicroContrastScaling"),
                                            Settings->GetDouble("MLMicroContrastWeight"));

        m_Image_AfterLabSN->HotpixelReduction(Settings->GetDouble("LabHotpixel"));

        TRACEMAIN("Done Gradient Sharpen at %d ms.",Timer.elapsed());
      }

      // Wiener Filter Sharpen

      if (Settings->ToolIsActive("TabLABWiener")) {

        m_ReportProgress(QObject::tr("Wiener Filter"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        ptWienerFilterChannel(m_Image_AfterLabSN,
                              Settings->GetDouble("WienerFilterGaussian"),
                              Settings->GetDouble("WienerFilterBox"),
                              Settings->GetDouble("WienerFilterLensBlur"),
                              Settings->GetDouble("WienerFilterAmount"),
                              Settings->GetInt("WienerFilterUseEdgeMask"));

        TRACEMAIN("Done Wiener Filter at %d ms.",Timer.elapsed());
      }

      // Inverse Diffusion Sharpen

      if (Settings->ToolIsActive("TabInverseDiffusion")) {

        m_ReportProgress(QObject::tr("Inverse Diffusion Sharpen"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        ptCimgSharpen(m_Image_AfterLabSN,
                      1,
                      Settings->GetDouble("InverseDiffusionAmplitude"),
                      Settings->GetInt("InverseDiffusionIterations"),
                      Settings->GetInt("InverseDiffusionUseEdgeMask"));

        TRACEMAIN("Done Inverse Diffusion Sharpen at %d ms.",Timer.elapsed());
      }

      // USM

      if (Settings->ToolIsActive("TabLABUSM")) {

        m_ReportProgress(QObject::tr("USM sharpening"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

//        m_Image_AfterLabSN->USM(1,
//                              Settings->GetDouble("USMRadius")*m_ScaleFactor,
//                              Settings->GetDouble("USMAmount"),
//                              Settings->GetDouble("USMThreshold"));

        m_Image_AfterLabSN->ptGMUnsharp(Settings->GetDouble("USMRadius")*m_ScaleFactor,
                                        Settings->GetDouble("USMAmount"),
                                        Settings->GetDouble("USMThreshold"));

        TRACEMAIN("Done USM at %d ms.",Timer.elapsed());
      }

      // Highpass

      if (Settings->ToolIsActive("TabLABHighpass")) {


        m_ReportProgress(QObject::tr("Highpass"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        m_Image_AfterLabSN->Highpass(Settings->GetDouble("HighpassRadius")*m_ScaleFactor,
                                   Settings->GetDouble("HighpassAmount"),
                                   -0.3,
                                   Settings->GetDouble("HighpassDenoise")/((logf(m_ScaleFactor)/logf(0.5))+1.0));

        TRACEMAIN("Done Highpass at %d ms.",Timer.elapsed());
      }


      // Grain

      if (Settings->ToolIsActive("TabLABFilmGrain") &&
          Settings->GetInt("Grain1MaskType")) {

        m_ReportProgress(QObject::tr("Film grain 1"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }
        m_Image_AfterLabSN->Grain(Settings->GetDouble("Grain1Strength"),
                                  Settings->GetInt("Grain1Mode"),
                                  Settings->GetDouble("Grain1Radius"),
                                  Settings->GetDouble("Grain1Opacity"),
                                  Settings->GetInt("Grain1MaskType"),
                                  Settings->GetDouble("Grain1LowerLimit"),
                                  Settings->GetDouble("Grain1UpperLimit"),
                                  (int)(logf(m_ScaleFactor)/logf(0.5)));
        TRACEMAIN("Done film grain 1 at %d ms.",Timer.elapsed());
      }

      if (Settings->ToolIsActive("TabLABFilmGrain") &&
          Settings->GetInt("Grain2MaskType")) {

        m_ReportProgress(QObject::tr("Film grain 2"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }
        m_Image_AfterLabSN->Grain(Settings->GetDouble("Grain2Strength"),
                                  Settings->GetInt("Grain2Mode"),
                                  Settings->GetDouble("Grain2Radius"),
                                  Settings->GetDouble("Grain2Opacity"),
                                  Settings->GetInt("Grain2MaskType"),
                                  Settings->GetDouble("Grain2LowerLimit"),
                                  Settings->GetDouble("Grain2UpperLimit"),
                                  (int)(logf(m_ScaleFactor)/logf(0.5)));
        TRACEMAIN("Done film grain 2 at %d ms.",Timer.elapsed());
      }

      // View LAB

      if (Settings->ToolIsActive("TabLABViewLab")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }
        m_ReportProgress(QObject::tr("View LAB"));

        m_Image_AfterLabSN->ViewLAB(Settings->GetInt("ViewLAB"));
      }

    // case ptProcessorPhase_Greyc : // Run GREYC.

      // if (Settings->GetInt("JobMode")) {
        // m_Image_AfterGREYC = m_Image_AfterLabInRGB; // Job mode -> no cache
      // } else {
        // if (!m_Image_AfterGREYC) m_Image_AfterGREYC = new(ptImage);
        // m_Image_AfterGREYC->Set(m_Image_AfterLabInRGB);
      // }

      ////GREYC Restoration (denoise)

      // if (Settings->GetInt("GREYC")) {

        // ptGreycStoration(m_Image_AfterGREYC,
                         // m_ReportProgress,
                         // Settings->GetInt("GREYCIterations"),
                         // Settings->GetDouble("GREYCAmplitude"),
                         // Settings->GetDouble("GREYCSharpness"),
                         // Settings->GetDouble("GREYCAnisotropy"),
                         // Settings->GetDouble("GREYCAlpha"),
                         // Settings->GetDouble("GREYCSigma"),
                         // Settings->GetDouble("GREYCpt"),
                         // Settings->GetInt("GREYCda"),
                         // Settings->GetDouble("GREYCGaussPrecision"),
                         // Settings->GetInt("GREYCInterpolation"),
                         // Settings->GetInt("GREYCFast"),

        // TRACEMAIN("Done GREYC Restoration at %d ms.",Timer.elapsed());

      // }

    case ptProcessorPhase_LabEyeCandy : // Run everything in LABSN.

      m_ReportProgress(QObject::tr("Phase Lab3"));

      if (Settings->GetInt("JobMode")) {
        m_Image_AfterLabEyeCandy = m_Image_AfterLabSN; // Job mode -> no cache
      } else {
        if (!m_Image_AfterLabEyeCandy) m_Image_AfterLabEyeCandy = new ptImage(m_UpdateGUI);
        m_Image_AfterLabEyeCandy->Set(m_Image_AfterLabSN);
      }

      // LByHue Curve

      if (Settings->ToolIsActive("TabLbyHue")) {
        m_ReportProgress(QObject::tr("Applying L by Hue curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabEyeCandy->ApplyLByHueCurve(Curve[ptCurveChannel_LByHue]);

        TRACEMAIN("Done L by Hue Curve at %d ms.",Timer.elapsed());
      }

      // Saturation Curve

      if (Settings->ToolIsActive("TabSaturationCurve")) {
        m_ReportProgress(QObject::tr("Applying saturation curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabEyeCandy->ApplySaturationCurve(Curve[ptCurveChannel_Saturation],
                                                       Settings->GetInt("SatCurveMode"),
                                                       Settings->GetInt("SatCurveType"));

        TRACEMAIN("Done saturation Curve at %d ms.",Timer.elapsed());
      }

      // L Curve

      if (Settings->ToolIsActive("TabLCurve")) {

        m_ReportProgress(QObject::tr("Applying L curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        m_Image_AfterLabEyeCandy->ApplyCurve(Curve[ptCurveChannel_L],1);

        TRACEMAIN("Done L Curve at %d ms.",Timer.elapsed());

      }

      // a Curve

      if (Settings->GetInt("CurveLa") &&
          Settings->ToolIsActive("TabABCurves")) {

        m_ReportProgress(QObject::tr("Applying a curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        m_Image_AfterLabEyeCandy->ApplyCurve(Curve[ptCurveChannel_a],2);

        TRACEMAIN("Done a Curve at %d ms.",Timer.elapsed());

      }

      // b Curve

      if (Settings->GetInt("CurveLb") &&
          Settings->ToolIsActive("TabABCurves")) {

        m_ReportProgress(QObject::tr("Applying b curve"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());

        }

        m_Image_AfterLabEyeCandy->ApplyCurve(Curve[ptCurveChannel_b],4);

        TRACEMAIN("Done b Curve at %d ms.",Timer.elapsed());

      }


      // Colorcontrast

      if (Settings->ToolIsActive("TabColorContrast")) {

        m_ReportProgress(QObject::tr("Colorcontrast"));

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_Image_AfterLabEyeCandy->Colorcontrast(
          Settings->GetInt("ColorcontrastRadius")*m_ScaleFactor,
          Settings->GetDouble("ColorcontrastAmount"),
          Settings->GetDouble("ColorcontrastOpacity"),
          Settings->GetDouble("ColorcontrastHaloControl"));
      }

      // LAB Tone adjustments

      if (Settings->ToolIsActive("TabLABToneAdjust1")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("LAB tone adjustments 1"));

        m_Image_AfterLabEyeCandy->LABTone(Settings->GetDouble("LABToneAdjust1Amount"),
            Settings->GetDouble("LABToneAdjust1Hue"),
            Settings->GetDouble("LABToneAdjust1Saturation"),
            Settings->GetInt("LABToneAdjust1MaskType"),
            1,
            Settings->GetDouble("LABToneAdjust1LowerLimit"),
            Settings->GetDouble("LABToneAdjust1UpperLimit"),
            Settings->GetDouble("LABToneAdjust1Softness"));
      }

      if (Settings->ToolIsActive("TabLABToneAdjust2")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("LAB tone adjustments 2"));

        m_Image_AfterLabEyeCandy->LABTone(Settings->GetDouble("LABToneAdjust2Amount"),
            Settings->GetDouble("LABToneAdjust2Hue"),
            Settings->GetDouble("LABToneAdjust2Saturation"),
            Settings->GetInt("LABToneAdjust2MaskType"),
            1,
            Settings->GetDouble("LABToneAdjust2LowerLimit"),
            Settings->GetDouble("LABToneAdjust2UpperLimit"),
            Settings->GetDouble("LABToneAdjust2Softness"));
      }

      // Luminance adjustment

      if (Settings->ToolIsActive("TabSaturationAdjustment") ||
          Settings->ToolIsActive("TabLAdjustment")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }
        m_ReportProgress(QObject::tr("Luminance and saturation adjustment"));

        m_Image_AfterLabEyeCandy->LAdjust(Settings->GetDouble("LAdjustC1"),
            Settings->GetDouble("LAdjustC2"),
            Settings->GetDouble("LAdjustC3"),
            Settings->GetDouble("LAdjustC4"),
            Settings->GetDouble("LAdjustC5"),
            Settings->GetDouble("LAdjustC6"),
            Settings->GetDouble("LAdjustC7"),
            Settings->GetDouble("LAdjustC8"),
            Settings->GetDouble("LAdjustSC1"),
            Settings->GetDouble("LAdjustSC2"),
            Settings->GetDouble("LAdjustSC3"),
            Settings->GetDouble("LAdjustSC4"),
            Settings->GetDouble("LAdjustSC5"),
            Settings->GetDouble("LAdjustSC6"),
            Settings->GetDouble("LAdjustSC7"),
            Settings->GetDouble("LAdjustSC8"));
      }

      // LAB Tone second stage

      if ((Settings->GetDouble("LABToneAmount") != 0.0 ||
          Settings->GetDouble("LABToneSaturation") != 1.0) &&
          Settings->ToolIsActive("TabLABTone")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("LAB toning"));

        m_Image_AfterLabEyeCandy->LABTone(Settings->GetDouble("LABToneAmount"),
            Settings->GetDouble("LABToneHue"),
            Settings->GetDouble("LABToneSaturation"));
      }

      if ((Settings->GetDouble("LABToneSAmount") != 0.0 ||
          Settings->GetDouble("LABToneSSaturation") != 1.0) &&
          Settings->ToolIsActive("TabLABTone")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("LAB shadows toning"));

        m_Image_AfterLabEyeCandy->LABTone(Settings->GetDouble("LABToneSAmount"),
            Settings->GetDouble("LABToneSHue"),
            Settings->GetDouble("LABToneSSaturation"),
            ptMaskType_Shadows);
      }

      if ((Settings->GetDouble("LABToneMAmount") != 0.0 ||
          Settings->GetDouble("LABToneMSaturation") != 1.0) &&
          Settings->ToolIsActive("TabLABTone")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("LAB midtones toning"));

        m_Image_AfterLabEyeCandy->LABTone(Settings->GetDouble("LABToneMAmount"),
            Settings->GetDouble("LABToneMHue"),
            Settings->GetDouble("LABToneMSaturation"),
            ptMaskType_Midtones);
      }

      if ((Settings->GetDouble("LABToneHAmount") != 0.0 ||
          Settings->GetDouble("LABToneHSaturation") != 1.0) &&
          Settings->ToolIsActive("TabLABTone")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("LAB highlights toning"));

        m_Image_AfterLabEyeCandy->LABTone(Settings->GetDouble("LABToneHAmount"),
            Settings->GetDouble("LABToneHHue"),
            Settings->GetDouble("LABToneHSaturation"),
            ptMaskType_Highlights);
      }

      // Vignette

      if (Settings->ToolIsActive("TabLABVignette")) {

        //Postponed RGBToLab for performance.
        if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
          m_Image_AfterLabEyeCandy->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
                    Timer.elapsed());
        }

        m_ReportProgress(QObject::tr("Lab Vignette"));

        m_Image_AfterLabEyeCandy->Vignette(Settings->GetInt("LabVignetteMode"),
          Settings->GetInt("LabVignette"),
          Settings->GetDouble("LabVignetteAmount"),
          Settings->GetDouble("LabVignetteInnerRadius"),
          Settings->GetDouble("LabVignetteOuterRadius"),
          Settings->GetDouble("LabVignetteRoundness")/2,
          Settings->GetDouble("LabVignetteCenterX"),
          Settings->GetDouble("LabVignetteCenterY"),
          Settings->GetDouble("LabVignetteSoftness"));

      }

    case ptProcessorPhase_EyeCandy : // Run EyeCandy.

      m_ReportProgress(QObject::tr("Phase EyeCandy"));

      if (Settings->GetInt("JobMode")) {
        m_Image_AfterEyeCandy = m_Image_AfterLabEyeCandy; // Job mode -> no cache
      } else {
        if (!m_Image_AfterEyeCandy) m_Image_AfterEyeCandy = new ptImage(m_UpdateGUI);
        m_Image_AfterEyeCandy->Set(m_Image_AfterLabEyeCandy);
      }

      // Has to be here to allow L histogram in Tabmode
      // Come back from Lab space if we ever were there ...
      if (m_Image_AfterEyeCandy->m_ColorSpace == ptSpace_Lab) {
        m_ReportProgress(QObject::tr("Lab to RGB"));
        m_Image_AfterEyeCandy->LabToRGB(Settings->GetInt("WorkColor"));
        TRACEMAIN("Done conversion to RGB at %d ms.",Timer.elapsed());
      }

      // Black & White Styler

      if (Settings->ToolIsActive("TabBW")) {

        m_ReportProgress(QObject::tr("Black and White"));

        m_Image_AfterEyeCandy->BWStyler(Settings->GetInt("BWStylerFilmType"),
          Settings->GetInt("BWStylerColorFilterType"),
          Settings->GetDouble("BWStylerMultR"),
          Settings->GetDouble("BWStylerMultG"),
          Settings->GetDouble("BWStylerMultB"),
          Settings->GetDouble("BWStylerOpacity"));
      }

      // Simple Tone

      if (Settings->ToolIsActive("TabSimpleTone")) {

        m_ReportProgress(QObject::tr("Simple Toning"));

        m_Image_AfterEyeCandy->SimpleTone(Settings->GetDouble("SimpleToneR"),
            Settings->GetDouble("SimpleToneG"),
            Settings->GetDouble("SimpleToneB"));
      }

      // Tone

      if (Settings->GetInt("Tone1MaskType") &&
          Settings->ToolIsActive("TabRGBTone")) {

        m_ReportProgress(QObject::tr("Toning"));

        m_Image_AfterEyeCandy->Tone(
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("Tone1ColorRed")  /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("Tone1ColorGreen")/(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("Tone1ColorBlue") /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          Settings->GetDouble("Tone1Amount"),
          Settings->GetInt("Tone1MaskType"),
          pow(Settings->GetDouble("Tone1LowerLimit"),
              Settings->GetDouble("InputPowerFactor")),
          pow(Settings->GetDouble("Tone1UpperLimit"),
              Settings->GetDouble("InputPowerFactor")),
          Settings->GetDouble("Tone1Softness"));
      }

      if (Settings->GetInt("Tone2MaskType") &&
          Settings->ToolIsActive("TabRGBTone")) {

        m_ReportProgress(QObject::tr("Toning"));

        m_Image_AfterEyeCandy->Tone(
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("Tone2ColorRed")  /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("Tone2ColorGreen")/(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("Tone2ColorBlue") /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          Settings->GetDouble("Tone2Amount"),
          Settings->GetInt("Tone2MaskType"),
          pow(Settings->GetDouble("Tone2LowerLimit"),Settings->GetDouble("InputPowerFactor")),
          pow(Settings->GetDouble("Tone2UpperLimit"),Settings->GetDouble("InputPowerFactor")),
          Settings->GetDouble("Tone2Softness"));

      }

      // Crossprocessing

      if (Settings->ToolIsActive("TabCrossProcessing")) {

        m_ReportProgress(QObject::tr("Crossprocessing"));

        m_Image_AfterEyeCandy->Crossprocess(Settings->GetInt("CrossprocessingMode"),
                                            Settings->GetDouble("CrossprocessingColor1"),
                                            Settings->GetDouble("CrossprocessingColor2"));
      }

      // RGB Contrast.

      if (Settings->ToolIsActive("TabECContrast")) {

        m_ReportProgress(QObject::tr("Applying RGB Contrast"));

        m_Image_AfterEyeCandy->SigmoidalContrast(Settings->GetDouble("RGBContrast2Amount"),
                                                 Settings->GetDouble("RGBContrast2Threshold"));
      }

      // Gradual Overlay

      if (Settings->ToolIsActive("TabGradualOverlay1")) {

        m_ReportProgress(QObject::tr("Gradual Overlay 1"));

        m_Image_AfterEyeCandy->GradualOverlay(
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("GradualOverlay1ColorRed")  /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("GradualOverlay1ColorGreen")/(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("GradualOverlay1ColorBlue") /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
            Settings->GetInt("GradualOverlay1"),
            Settings->GetDouble("GradualOverlay1Amount"),
            Settings->GetDouble("GradualOverlay1Angle"),
            Settings->GetDouble("GradualOverlay1LowerLevel"),
            Settings->GetDouble("GradualOverlay1UpperLevel"),
            Settings->GetDouble("GradualOverlay1Softness"));

      }

      if (Settings->ToolIsActive("TabGradualOverlay2")) {

        m_ReportProgress(QObject::tr("Gradual Overlay 2"));

        m_Image_AfterEyeCandy->GradualOverlay(
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("GradualOverlay2ColorRed")  /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("GradualOverlay2ColorGreen")/(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
          (uint16_t)(0xffff*
                     pow(Settings->GetInt("GradualOverlay2ColorBlue") /(double)0xff,
                         Settings->GetDouble("InputPowerFactor"))),
            Settings->GetInt("GradualOverlay2"),
            Settings->GetDouble("GradualOverlay2Amount"),
            Settings->GetDouble("GradualOverlay2Angle"),
            Settings->GetDouble("GradualOverlay2LowerLevel"),
            Settings->GetDouble("GradualOverlay2UpperLevel"),
            Settings->GetDouble("GradualOverlay2Softness"));

      }

      // Vignette

      if (Settings->ToolIsActive("TabRGBVignette")) {

        m_ReportProgress(QObject::tr("Vignette"));

        m_Image_AfterEyeCandy->Vignette(Settings->GetInt("VignetteMode"),
          Settings->GetInt("Vignette"),
          Settings->GetDouble("VignetteAmount"),
          Settings->GetDouble("VignetteInnerRadius"),
          Settings->GetDouble("VignetteOuterRadius"),
          Settings->GetDouble("VignetteRoundness")/2,
          Settings->GetDouble("VignetteCenterX"),
          Settings->GetDouble("VignetteCenterY"),
          Settings->GetDouble("VignetteSoftness"));

      }

      // Softglow

      if (Settings->ToolIsActive("TabSoftglow")) {

        m_ReportProgress(QObject::tr("Softglow"));

        m_Image_AfterEyeCandy->Softglow(Settings->GetInt("SoftglowMode"),
          Settings->GetDouble("SoftglowRadius")*m_ScaleFactor,
          Settings->GetDouble("SoftglowAmount"),
          7,
          Settings->GetDouble("SoftglowContrast"),
          Settings->GetInt("SoftglowSaturation"));
      }

      // Vibrance

      if (Settings->ToolIsActive("TabECColorIntensity") &&
          Settings->GetInt("Vibrance2")) {

        m_ReportProgress(QObject::tr("Vibrance 2"));
        int Value = (Settings->GetInt("Vibrance2"));
        double VibranceMixer[3][3];

        VibranceMixer[0][0] = 1.0+(Value/150.0);
        VibranceMixer[0][1] = -(Value/300.0);
        VibranceMixer[0][2] = VibranceMixer[0][1];
        VibranceMixer[1][0] = VibranceMixer[0][1];
        VibranceMixer[1][1] = VibranceMixer[0][0];
        VibranceMixer[1][2] = VibranceMixer[0][1];
        VibranceMixer[2][0] = VibranceMixer[0][1];
        VibranceMixer[2][1] = VibranceMixer[0][1];
        VibranceMixer[2][2] = VibranceMixer[0][0];

        m_Image_AfterEyeCandy->MixChannels(VibranceMixer);
      }

      if (Settings->ToolIsActive("TabECColorIntensity") &&
          (Settings->GetInt("Intensity2Red")   ||
           Settings->GetInt("Intensity2Green") ||
           Settings->GetInt("Intensity2Blue"))) {

        m_ReportProgress(QObject::tr("Intensity RGB 2"));
        int ValueR=(Settings->GetInt("Intensity2Red"));
        int ValueG=(Settings->GetInt("Intensity2Green"));
        int ValueB=(Settings->GetInt("Intensity2Blue"));
        double IntensityMixer[3][3];
        IntensityMixer[0][0] = 1.0+(ValueR/150.0);
        IntensityMixer[0][1] = -(ValueR/300.0);
        IntensityMixer[0][2] = IntensityMixer[0][1];
        IntensityMixer[1][0] = -(ValueG/300.0);
        IntensityMixer[1][1] = 1.0+(ValueG/150.0);;
        IntensityMixer[1][2] = IntensityMixer[1][0];
        IntensityMixer[2][0] = -(ValueB/300.0);
        IntensityMixer[2][1] = IntensityMixer[2][0];
        IntensityMixer[2][2] = 1.0+(ValueB/150.0);;

        m_Image_AfterEyeCandy->MixChannels(IntensityMixer);
      }

      // Tone curves
      if (Settings->ToolIsActive("TabRToneCurve")) {

        m_ReportProgress(QObject::tr("Applying R curve"));

        m_Image_AfterEyeCandy->ApplyCurve(Curve[ptCurveChannel_R],1);

        TRACEMAIN("Done R Curve at %d ms.",Timer.elapsed());

      }

      if (Settings->ToolIsActive("TabGToneCurve")) {

        m_ReportProgress(QObject::tr("Applying G curve"));

        m_Image_AfterEyeCandy->ApplyCurve(Curve[ptCurveChannel_G],2);

        TRACEMAIN("Done G Curve at %d ms.",Timer.elapsed());

      }

      if (Settings->ToolIsActive("TabBToneCurve")) {

        m_ReportProgress(QObject::tr("Applying B curve"));

        m_Image_AfterEyeCandy->ApplyCurve(Curve[ptCurveChannel_B],4);

        TRACEMAIN("Done B Curve at %d ms.",Timer.elapsed());

      }

      Settings->SetValue("PipeImageW",m_Image_AfterEyeCandy->m_Width);
      Settings->SetValue("PipeImageH",m_Image_AfterEyeCandy->m_Height);

    case ptProcessorPhase_Output : // Run Output.

Exit:

      TRACEMAIN("Done pipe processing at %d ms.",Timer.elapsed());

      break;

    default : // Should not happen.
      assert(0);
  }

  m_ReportProgress(QObject::tr("Ready"));
}

////////////////////////////////////////////////////////////////////////////////
//
// ReadExifBuffer
//
////////////////////////////////////////////////////////////////////////////////

void ptProcessor::ReadExifBuffer() {

  // Safe defaults.
  FREE(m_ExifBuffer);
  m_ExifBufferLength = 0;
  Settings->SetValue("LensfunCameraMake","");
  Settings->SetValue("LensfunCameraModel","");
  Settings->SetValue("LensfunCameraUpdatedByProcessor",1);

  try {
    Exiv2::Image::AutoPtr Image;
    if (Exiv2::ImageFactory::getType(
      (Settings->GetStringList("InputFileNameList"))[0].toAscii().data()) != Exiv2::ImageType::none)
      Image =
        Exiv2::ImageFactory::open((Settings->GetStringList("InputFileNameList"))[0].toAscii().data());
    else return;

    assert(Image.get() != 0);
    // TODO mike: The next line gives a seg fault when exiv2 doesn't know the camera.
    Image->readMetadata();

    m_ExifData = Image->exifData();
    if (m_ExifData.empty()) {
      ptLogWarning(ptWarning_Argument,
                "No Exif data found in %s",
                (Settings->GetStringList("InputFileNameList"))[0].toAscii().data());
      return;
    }

//~ #if EXIV2_TEST_VERSION(0,14,0)
    // Processing software: photivo
    // less intrusive than having photivo in the camera model like gimp
    //~ m_ExifData["Exif.Image.ProcessingSoftware"] = ProgramName;
    //~ m_ExifData["Exif.Image.Software"] = ProgramName;
    // Since gimp kills the software tags, append it to model
    //~ Exiv2::Exifdatum& tag = m_ExifData["Exif.Image.Model"];
    //~ std::string model = tag.toString();
    //~ std::string space = " ";
    //~ int Counter = model.find_last_not_of(space);
    //~ model.erase(Counter+1, model.length()-1-Counter);
    //~ std::string photivoInfo = " (photivo)";
    //~ model.append(photivoInfo);
    //~ tag.setValue(model);
//~ #endif

    if (Settings->GetInt("ImageRating"))
      m_ExifData["Exif.Image.Rating"] = Settings->GetInt("ImageRating");

    Exiv2::ExifData::iterator Pos;
    long Size;

#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
    Exiv2::Blob Blob;
    Exiv2::ExifParser::encode(Blob,Exiv2::bigEndian,m_ExifData);
    Size = Blob.size();
#else
    Exiv2::DataBuf Buf(m_ExifData.copy());
    Size = Buf.size_;
#endif
    const unsigned char ExifHeader[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};
    const unsigned short MaxHeaderLength = 65533;
    /* If buffer too big for JPEG, try deleting some stuff. */
    if ( Size + sizeof(ExifHeader) > MaxHeaderLength ) {
      if ( (Pos=m_ExifData.findKey(Exiv2::ExifKey("Exif.Photo.MakerNote")))
            != m_ExifData.end() ) {
        m_ExifData.erase(Pos);
        ptLogWarning(ptWarning_Argument,
         "Exif buffer too big, erasing Exif.Photo.MakerNote");
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
        Exiv2::ExifParser::encode(Blob,Exiv2::bigEndian,m_ExifData);
        Size = Blob.size();
#else
        Buf = m_ExifData.copy();
        Size = Buf.size_;
#endif
      }
    }
    if (Settings->GetInt("EraseExifThumbnail") || Size + sizeof(ExifHeader) > MaxHeaderLength ) {
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
      Exiv2::ExifThumb Thumb(m_ExifData);
      Thumb.erase();
#else
      m_ExifData.eraseThumbnail();
#endif

      if (!Settings->GetInt("EraseExifThumbnail"))
        ptLogWarning(ptWarning_Argument,
         "Exif buffer too big, erasing Thumbnail");
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
      Exiv2::ExifParser::encode(Blob,Exiv2::bigEndian,m_ExifData);
      Size = Blob.size();
#else
      Buf = m_ExifData.copy();
      Size = Buf.size_;
#endif
    }

    m_ExifBufferLength = Size + sizeof(ExifHeader);
    FREE(m_ExifBuffer);
    m_ExifBuffer = (unsigned char*) MALLOC(m_ExifBufferLength);
    ptMemoryError(m_ExifBuffer,__FILE__,__LINE__);

    memcpy(m_ExifBuffer, ExifHeader, sizeof(ExifHeader));
#if EXIV2_TEST_VERSION(0,17,91)   /* Exiv2 0.18-pre1 */
    memcpy(m_ExifBuffer+sizeof(ExifHeader), &Blob[0], Blob.size());
#else
    memcpy(m_ExifBuffer+sizeof(ExifHeader), Buf.pData_, Buf.size_);
#endif

    // for use by lensfun : Make, Model, Focal Length, FNumber

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Image.Make"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      Settings->SetValue("LensfunCameraMake",
        QString(str.str().c_str()).toUpper().trimmed());
    }

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Image.Model"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      Settings->SetValue("LensfunCameraModel",
        QString(str.str().c_str()).toUpper().trimmed());
    }

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      float FNumber;
      sscanf(str.str().c_str(),"%*c%f",&FNumber);
      Settings->SetValue("LensfunF",FNumber);
    }

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      int FocalLength;
      sscanf(str.str().c_str(),"%d",&FocalLength);
      Settings->SetValue("LensfunFocalLength",FocalLength);
    }

  }
  catch(Exiv2::Error& Error) {
    // Exiv2 errors are in this context hopefully harmless
    // (unsupported tags etc.)

    ptLogWarning(ptWarning_Exiv2,"Exiv2 : %s\n",Error.what());
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// CalculateAutoExposure
// There should be Settings->m_WhiteFraction % pixels above 90%
//
////////////////////////////////////////////////////////////////////////////////

double ptProcessor::CalculateAutoExposure(ptImage *Image) {

  uint16_t White = Image->CalculateFractionLevel(Settings->GetInt("WhiteFraction")/100.0,7);
  TRACEKEYVALS("White","%d",White);
  // Account also for the ExposureNormalization that will be added.
  // And express in EV.
  // TODO Is Settings->ExposureNormalization valid at al times at this
  // point ? Might be a bug.
  double AutoExposure = log(Settings->GetInt("WhiteLevel")/100.0*0xFFFF / White)/log(2);
  TRACEKEYVALS("AutoExpos(EV)","%f",AutoExposure);
  //TRACEKEYVALS("ExposNorm(EV)","%f",Settings->GetDouble("ExposureNormalization"));

  return AutoExposure; //-Settings->GetDouble("ExposureNormalization");
  // Since now the ExposureNormalization is not used all the time.
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
////////////////////////////////////////////////////////////////////////////////

ptProcessor::~ptProcessor() {
  // Tricky delete stuff as some pointer might be shared.
  QList <ptImage*> PointerList;
  PointerList << m_Image_AfterDcRaw
              << m_Image_AfterLensfun
              << m_Image_AfterRGB
              << m_Image_AfterLabCC
              << m_Image_AfterLabSN
        << m_Image_AfterLabEyeCandy
              << m_Image_AfterEyeCandy;
  while(PointerList.size()) {
    ptImage* CurrentPointer = PointerList[0];
    delete CurrentPointer;
    // Remove all elements equal to CurrentPointer.
    short Index=0;
    while (Index<PointerList.size()) {
      if (CurrentPointer == PointerList[Index]) {
        PointerList.removeAt(Index);
      } else {
        Index++;
      }
    }
  }
  if (m_ExifBuffer) FREE(m_ExifBuffer);
}

///////////////////////////////////////////////////////////////////////////////
