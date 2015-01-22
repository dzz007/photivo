/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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
#include "ptProcessor.h"

#include "ptConstants.h"
#include "ptError.h"
#include "ptSettings.h"
#include "ptImage.h"
//#include "ptLensfun.h"    // TODO BJ: implement lensfun DB
#include "ptCurve.h"
#include "ptCimg.h"
#include "ptDcRaw.h"
#include "ptMessageBox.h"
#include "ptImageHelper.h"
#include <filters/ptFilterDM.h>
#include <filters/ptFilterBase.h>
#include <filters/ptFilterUids.h>

#include <exiv2/error.hpp>

#include <QFileInfo>
#include <QApplication>

//==============================================================================

// Prototype for status report in Viewwindow
void ViewWindowShowStatus(short State);

//==============================================================================

// FindAlpha (helper for ExposureFunction)
// such that f(x)=(1-exp(-Alpha*x)/(1-exp(-Alpha))
// f(x) is then foto curve for which :
// f(0) = 0
// f(1) = 1
// f'(0)= Exposure
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

//==============================================================================

// ExposureFunction
// (used to generate an expsure curve)
static double Alpha;
static double ExposureForWhichAlphaIsCalculated = -1;

double ExposureFunction(double r, double Exposure, double) {
  if (Exposure != ExposureForWhichAlphaIsCalculated) {
    Alpha = FindAlpha(Exposure);
  }
  return (1-exp(-Alpha*r))/(1-exp(-Alpha));
}


//==============================================================================

ptProcessor::ptProcessor(PReportProgressFunc AReportProgress)
{
  // We work with a callback to avoid dependency on ptMainWindow
  m_ReportProgress = AReportProgress;

  // The DcRaw
  m_DcRaw          = NULL;

  // Cached version at different points.
  m_Image_AfterDcRaw       = NULL;
  m_Image_AfterLocalEdit   = NULL;
  m_Image_AfterGeometry    = NULL;
  m_Image_AfterRGB         = NULL;
  m_Image_AfterLabCC       = NULL;
  m_Image_AfterLabSN       = NULL;
  m_Image_AfterLabEyeCandy = NULL;
  m_Image_AfterEyeCandy    = NULL;

  m_Image_DetailPreview    = NULL;

  m_Image_TextureOverlay   = NULL;
  m_Image_TextureOverlay2  = NULL;

  m_AutoExposureValue      = 0.0;

  m_ScaleFactor            = 0;
}

//==============================================================================

ptProcessor::~ptProcessor() {
  // Tricky delete stuff as some pointer might be shared.
  QList <ptImage*> PointerList;
  PointerList << m_Image_AfterDcRaw
              << m_Image_AfterLocalEdit
              << m_Image_AfterGeometry
              << m_Image_AfterRGB
              << m_Image_AfterLabCC
              << m_Image_AfterLabSN
              << m_Image_AfterLabEyeCandy
              << m_Image_AfterEyeCandy
              << m_Image_DetailPreview
              << m_Image_TextureOverlay
              << m_Image_TextureOverlay2;
  while(PointerList.size()) {
    if (PointerList[0] != NULL) {
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
    } else {
      PointerList.removeAt(0);
    }
  }
}

//==============================================================================

void ptProcessor::Run(short Phase,
                      short SubPhase,
                      short WithIdentify,
                      short ProcessorMode)
{
  try {
    // Temp var for new-style filters. Needs to be up here to not interfere with gotos and case labels.
    ptFilterBase *hFilter = nullptr;

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
    FRunTimer.start();

    double  ExposureFactor;
    QString CameraProfileName = Settings->GetString("CameraColorProfile");

    if (!m_DcRaw) goto Exit;

    // Status report
    ::ViewWindowShowStatus(ptStatus_Processing);

    // correction for bitmaps
    if (!Settings->useRAWHandling()) {
      if (Phase == ptProcessorPhase_Raw &&
          SubPhase > ptProcessorPhase_Load) {
        Phase = ptProcessorPhase_LocalEdit;
      }
    }


    switch(Phase) {
      //------------------------------------------------------------------------------
      case ptProcessorPhase_Raw:

        Settings->ToDcRaw(m_DcRaw);

        // The halfsize setting is only used in Gui interactive.
        // (or thumbnail generation)
        // Otherwise we always start from the full image to operate upon.
        // TODO: Improvement possible if final image is less than half
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
        Settings->SetValue("Scaled", m_DcRaw->m_UserSetting_HalfSize);

        if (!Settings->useRAWHandling()) {
          m_ReportProgress(tr("Loading Bitmap"));

          TRACEMAIN("Start opening bitmap at %d ms.",
                      FRunTimer.elapsed());

          if (!m_Image_AfterDcRaw) m_Image_AfterDcRaw = new ptImage();

          int Success = 0;

          if (Settings->GetInt("IsRAW") == 1) { // RAW image, we fetch the thumbnail
            auto ImgData = m_DcRaw->thumbnail();

            m_Image_AfterDcRaw->ptGMCOpenImage(
              (Settings->GetStringList("InputFileNameList"))[0].toLocal8Bit().data(),
              Settings->GetInt("WorkColor"),
              Settings->GetInt("PreviewColorProfileIntent"),
              0,
              true,
              &ImgData,
              Success);
          } else {
            m_Image_AfterDcRaw->ptGMCOpenImage(
              (Settings->GetStringList("InputFileNameList"))[0].toLocal8Bit().data(),
              Settings->GetInt("WorkColor"),
              Settings->GetInt("PreviewColorProfileIntent"),
              0,
              false,
              nullptr,
              Success);
          }

          if (Success == 0) {
            ptMessageBox::critical(0,"File not found","File not found!");
            return;
          }

          Settings->SetValue("ImageW", m_Image_AfterDcRaw->m_Width);
          Settings->SetValue("ImageH", m_Image_AfterDcRaw->m_Height);

          m_ReportProgress(tr("Reading exif info"));

          if (ProcessorMode != ptProcessorMode_Thumb) {
            // Read Exif
            ReadExifBuffer();
          }
          TRACEMAIN("Opened bitmap at %d ms.", FRunTimer.elapsed());

        } else { // we have a RAW image!

          TRACEMAIN("Starting DcRaw at %d ms.",FRunTimer.elapsed());
          switch (SubPhase) {
            case ptProcessorPhase_Load :

              m_ReportProgress(tr("Reading RAW file"));

              if (WithIdentify) m_DcRaw->Identify();
              m_DcRaw->RunDcRaw_Phase1();

              // Do not forget !
              // Not in DcRawToSettings as at this point it is
              // not yet influenced by HalfSize. Later it is and
              // it would be wrongly overwritten then.
              if (Settings->GetInt("DetailViewActive") == 0) {
                Settings->SetValue("ImageW",m_DcRaw->m_ReportedWidth);
                Settings->SetValue("ImageH",m_DcRaw->m_ReportedHeight);

                TRACEKEYVALS("ImageW","%d",Settings->GetInt("ImageW"));
                TRACEKEYVALS("ImageH","%d",Settings->GetInt("ImageH"));
              }

              m_ReportProgress(tr("Reading exif info"));

              if (ProcessorMode != ptProcessorMode_Thumb) {
                // Read Exif
                ReadExifBuffer();
              }
              TRACEMAIN("Opened RAW at %d ms.",
                        FRunTimer.elapsed());

            case ptProcessorPhase_Demosaic :

              m_ReportProgress(tr("Demosaicing"));

              // Settings->GetInt("JobMode") causes NoCache
              m_DcRaw->RunDcRaw_Phase2(Settings->GetInt("JobMode"));

              TRACEMAIN("Done Color Scaling and Interpolation at %d ms.",
                        FRunTimer.elapsed());

            case ptProcessorPhase_Highlights :

              m_ReportProgress(tr("Recovering highlights"));

              // Settings->GetInt("JobMode") causes NoCache
              m_DcRaw->RunDcRaw_Phase3(Settings->GetInt("JobMode"));

              TRACEMAIN("Done Highlights at %d ms.",FRunTimer.elapsed());

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
                               CameraProfileName.toLocal8Bit().data());
                  TRACEMAIN("Found profile at %d ms.",FRunTimer.elapsed());
                } else {
                  ptMessageBox::information(0,
                            tr("Profile not found"),
                            tr("Profile not found. Reverting to Adobe Matrix.\nYou could try an external profile."));
                  TRACEMAIN("Not found profile at %d ms.",FRunTimer.elapsed());
                  Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
                }
              } else if (Settings->GetInt("CameraColor") == ptCameraColor_Profile) {
                  CameraProfileName = Settings->GetString("CameraColorProfile");
                  QFileInfo PathInfo(CameraProfileName);
                  if (!(PathInfo.exists() &&
                        PathInfo.isFile() &&
                        PathInfo.isReadable())) {
                    ptMessageBox::information(0,
                              tr("Profile not found"),
                              tr("Profile not found. Reverting to Adobe Matrix.\nYou could try an external profile."));
                    TRACEMAIN("Not found profile at %d ms.",FRunTimer.elapsed());
                    Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
                  }
              } else if (Settings->GetInt("CameraColor") == ptCameraColor_Flat) {
                QString InputFileName;
                InputFileName = Settings->GetString("UserDirectory");
                // hard coded, other paths may be altered by user
                InputFileName += "/Profiles/Camera/Flat/FlatProfile.icc";
                QFileInfo PathInfo(InputFileName);
                if (PathInfo.exists() &&
                    PathInfo.isFile() &&
                    PathInfo.isReadable()) {
                  CameraProfileName = PathInfo.absoluteFilePath();
                  Settings->SetValue("CameraColorProfile",CameraProfileName);
                  TRACEMAIN("Found profile at %d ms.",FRunTimer.elapsed());
                } else {
                  TRACEMAIN("Not found profile at %d ms.",FRunTimer.elapsed());
                  printf("profile %s\n\n",InputFileName.toLocal8Bit().data());
                  Settings->SetValue("CameraColor",ptCameraColor_Adobe_Matrix);
                }
              }

              // We're at the end of the DcRaw part now and capture
              // the image that DcRaw made for us.
              if (!m_Image_AfterDcRaw) m_Image_AfterDcRaw = new ptImage();

              // Transfer dcraw output to an image, maybe applying a profile
              // and a preprofile.

              m_Image_AfterDcRaw->Set(
                m_DcRaw,
                Settings->GetInt("WorkColor"),
                (Settings->GetInt("CameraColor") == ptCameraColor_Adobe_Matrix) ?
                  NULL : CameraProfileName.toLocal8Bit().data(),
                Settings->GetInt("CameraColorProfileIntent"),
                Settings->GetInt("CameraColorGamma"));

              Settings->FromDcRaw(m_DcRaw);
              if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Ufraw)
                Settings->SetValue("Exposure",Settings->GetDouble("ExposureNormalization"));

              TRACEMAIN("Done m_Image_AfterDcRaw transfer to GUI at %d ms.",
                         FRunTimer.elapsed());

              break;
            default : // Should not happen.
              GInfo->Raise(QString("Processor subphase ") + QString::number(SubPhase) + QString(" does not exist."), AT);
          }
        }

      //------------------------------------------------------------------------------

      case ptProcessorPhase_LocalEdit:
        RunLocalEdit();

      //------------------------------------------------------------------------------

      case ptProcessorPhase_Geometry:
        RunGeometry();

      //------------------------------------------------------------------------------

      case ptProcessorPhase_RGB: // Run everything in RGB.
        // Geometry block
        // This will skip the rest of the pipe, to make geometry adjustments easier.

        if (Settings->ToolIsActive("TabBlock")){ // &&
          if (!m_Image_AfterRGB) m_Image_AfterRGB = new ptImage();
          m_Image_AfterRGB->Set(m_Image_AfterGeometry);
          if (!m_Image_AfterLabCC) m_Image_AfterLabCC = new ptImage();
          m_Image_AfterLabCC->Set(m_Image_AfterGeometry);
          if (!m_Image_AfterLabSN) m_Image_AfterLabSN = new ptImage();
          m_Image_AfterLabSN->Set(m_Image_AfterGeometry);
          if (!m_Image_AfterEyeCandy) m_Image_AfterEyeCandy = new ptImage();
          m_Image_AfterEyeCandy->Set(m_Image_AfterGeometry);
          goto Exit;
        }

        if (Settings->GetInt("JobMode")) {
          m_Image_AfterRGB = m_Image_AfterGeometry; // Job mode -> no cache
        } else {
          if (!m_Image_AfterRGB) m_Image_AfterRGB = new ptImage();
          m_Image_AfterRGB->Set(m_Image_AfterGeometry);
        }

        // Calculate the autoexposure required value at this point.
        if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Auto) {
          m_ReportProgress(tr("Calculate auto exposure"));
          m_AutoExposureValue = CalculateAutoExposure(m_Image_AfterGeometry);
          Settings->SetValue("Exposure",m_AutoExposureValue);
        }
        if (Settings->GetInt("AutoExposure")==ptAutoExposureMode_Ufraw)
            Settings->SetValue("Exposure",Settings->GetDouble("ExposureNormalization"));


        //***************************************************************************
        // Channel mixing.

        hFilter = GFilterDM->GetFilterFromName(Fuid::ChannelMixer_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Highlights

        hFilter = GFilterDM->GetFilterFromName(Fuid::Highlights_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Vibrance

        hFilter = GFilterDM->GetFilterFromName(Fuid::ColorIntensity_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Brightness

        hFilter = GFilterDM->GetFilterFromName(Fuid::Brightness_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Exposure and Exposure gain

        if (Settings->ToolIsActive("TabExposure")) {

          m_ReportProgress(tr("Correcting Exposure"));

          // From EV to factor.
          ExposureFactor = pow(2,Settings->GetDouble("Exposure"));

          TRACEKEYVALS("ExposureFactor","%f",ExposureFactor);

          if (Settings->GetInt("ExposureClipMode") != ptExposureClipMode_Curve ||
              (ExposureFactor <= 1.0) ) {
            m_Image_AfterRGB->Expose(ExposureFactor,
                                     Settings->GetInt("ExposureClipMode"));
          } else {
            auto ExposureCurve = new ptCurve();
            ExposureCurve->setFromFunc(ExposureFunction, ExposureFactor, 0);
            m_Image_AfterRGB->ApplyCurve(ExposureCurve,7);
            delete ExposureCurve;
          }
        }


        //***************************************************************************
        // Reinhard 05

        hFilter = GFilterDM->GetFilterFromName(Fuid::ReinhardBrighten_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Gamma tool

        hFilter = GFilterDM->GetFilterFromName(Fuid::GammaTool_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Normalization

        hFilter = GFilterDM->GetFilterFromName(Fuid::Normalization_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Color Enhancement

        hFilter = GFilterDM->GetFilterFromName(Fuid::ColorEnhancement_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // LMHRecovery

        hFilter = GFilterDM->GetFilterFromName(Fuid::LMHRecovery_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }

        //***************************************************************************
        // RGB Texturecontrast

        hFilter = GFilterDM->GetFilterFromName(Fuid::TextureContrast_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }

        //***************************************************************************
        // Local contrast

        hFilter = GFilterDM->GetFilterFromName(Fuid::LocalContrast1_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::LocalContrast2_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


       //***************************************************************************
       // RGB Contrast.

        hFilter = GFilterDM->GetFilterFromName(Fuid::SigContrastRgb_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // Levels

        hFilter = GFilterDM->GetFilterFromName(Fuid::Levels_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
        }


        //***************************************************************************
        // RGB Curves.

        hFilter = GFilterDM->GetFilterFromName(Fuid::RgbCurve_RGB);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterRGB);
          TRACEMAIN("Done RGB Curve at %d ms.",FRunTimer.elapsed());
        }



      case ptProcessorPhase_LabCC : // Run everything in LAB.

        if (Settings->GetInt("JobMode")) {
          m_Image_AfterLabCC = m_Image_AfterRGB; // Job mode -> no cache
        } else {
          if (!m_Image_AfterLabCC) m_Image_AfterLabCC = new ptImage();
          m_Image_AfterLabCC->Set(m_Image_AfterRGB);
        }


        //***************************************************************************
        // LAB Transform, this has to be the first filter
        // in the LAB series, because it needs RGB input

        hFilter = GFilterDM->GetFilterFromName(Fuid::LabTransform_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Shadows Highlights

        hFilter = GFilterDM->GetFilterFromName(Fuid::ShadowsHighlights_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // LabLMHLightRecovery

        hFilter = GFilterDM->GetFilterFromName(Fuid::LMHRecovery_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Dynamic Range Compression

        hFilter = GFilterDM->GetFilterFromName(Fuid::Drc_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Texture curve

        hFilter = GFilterDM->GetFilterFromName(Fuid::TextureCurve_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
          TRACEMAIN("Done texture curve at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Texturecontrast

        hFilter = GFilterDM->GetFilterFromName(Fuid::TextureContrast1_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::TextureContrast2_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }

        //***************************************************************************
        // Local contrast

        hFilter = GFilterDM->GetFilterFromName(Fuid::LocalContrast1_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::LocalContrast2_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Local Contrast Stretch

        hFilter = GFilterDM->GetFilterFromName(Fuid::LocalContrastStretch1_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::LocalContrastStretch2_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // L Contrast

        hFilter = GFilterDM->GetFilterFromName(Fuid::SigContrastLab_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Saturation

        hFilter = GFilterDM->GetFilterFromName(Fuid::Saturation_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Color Boost

        hFilter = GFilterDM->GetFilterFromName(Fuid::ColorBoost_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Levels

        hFilter = GFilterDM->GetFilterFromName(Fuid::Levels_LabCC);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabCC);
        }



      case ptProcessorPhase_LabSN : // Run everything in LABSN.

        if (Settings->GetInt("JobMode")) {
          m_Image_AfterLabSN = m_Image_AfterLabCC; // Job mode -> no cache
        } else {
          if (!m_Image_AfterLabSN) m_Image_AfterLabSN = new ptImage();
          m_Image_AfterLabSN->Set(m_Image_AfterLabCC);
        }


        //***************************************************************************
        // Impulse denoise

        hFilter = GFilterDM->GetFilterFromName(Fuid::ImpulseNR_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Edge avoiding wavelet filter

        hFilter = GFilterDM->GetFilterFromName(Fuid::EAWavelets_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // GreyCStoration on L

        if (Settings->ToolIsActive("TabLABGreyC") &&
            Settings->GetInt("GREYCLab")<=2) {

          m_ReportProgress(tr("GreyCStoration on L"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

        TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

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

          TRACEMAIN("Done GreyCStoration on L at %d ms.",FRunTimer.elapsed());

        } else if (Settings->ToolIsActive("TabLABGreyC") &&
                   Settings->GetInt("GREYCLab") == ptEnable_ShowMask) {
           m_ReportProgress(tr("GreyCStoration on L"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

          TRACEMAIN("Done conversion to LAB at %d ms.",
          FRunTimer.elapsed());

          }
          ptCimgEdgeTensors(m_Image_AfterLabSN,
                            Settings->GetDouble("GREYCLabSharpness"),
                            Settings->GetDouble("GREYCLabAnisotropy"),
                            Settings->GetDouble("GREYCLabAlpha"),
                            Settings->GetDouble("GREYCLabSigma"),
                            0,
                            Settings->GetInt("GREYCLabMaskType"));
        }


        //***************************************************************************
        // Defringe

        if (Settings->ToolIsActive("TabLABDefringe")) {

          m_ReportProgress(tr("Defringe"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());
          }

          m_Image_AfterLabSN->DeFringe(Settings->GetDouble("DefringeRadius")*m_ScaleFactor,
                                       Settings->GetInt("DefringeThreshold"),
                                       (Settings->GetInt("DefringeColor1")<<0) +
                                       (Settings->GetInt("DefringeColor2")<<1) +
                                       (Settings->GetInt("DefringeColor3")<<2) +
                                       (Settings->GetInt("DefringeColor4")<<3) +
                                       (Settings->GetInt("DefringeColor5")<<4) +
                                       (Settings->GetInt("DefringeColor6")<<5),
                                       Settings->GetDouble("DefringeShift"));
          TRACEMAIN("Done defringe at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Wavelet denoise

        if (Settings->GetDouble("WaveletDenoiseL") &&
            Settings->ToolIsActive("TabWaveletDenoise")) {

          m_ReportProgress(tr("Wavelet L denoising"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());
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
          TRACEMAIN("Done Luminance wavelet denoise at %d ms.",FRunTimer.elapsed());
        }

        if (Settings->GetDouble("WaveletDenoiseA") &&
            Settings->ToolIsActive("TabWaveletDenoise")) {

          m_ReportProgress(tr("Wavelet A denoising"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

          }

          m_Image_AfterLabSN->WaveletDenoise(
                                  2,
                                  Settings->GetDouble("WaveletDenoiseA")/((logf(m_ScaleFactor)/logf(0.5))+1.0),
          Settings->GetDouble("WaveletDenoiseASoftness"));

          TRACEMAIN("Done A wavelet denoise at %d ms.",FRunTimer.elapsed());
        }

        if (Settings->GetDouble("WaveletDenoiseB") &&
            Settings->ToolIsActive("TabWaveletDenoise")) {

          m_ReportProgress(tr("Wavelet B denoising"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

          }

          m_Image_AfterLabSN->WaveletDenoise(
                                  4,
                                  Settings->GetDouble("WaveletDenoiseB")/((logf(m_ScaleFactor)/logf(0.5))+1.0),
              Settings->GetDouble("WaveletDenoiseBSoftness"));

          TRACEMAIN("Done B wavelet denoise at %d ms.",FRunTimer.elapsed());
        }

        //***************************************************************************
        // Bilateral filter on L (luma denoising)

        hFilter = GFilterDM->GetFilterFromName(Fuid::LumaDenoise_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Denoise curve

        hFilter = GFilterDM->GetFilterFromName(Fuid::LumaDenoiseCurve_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Pyramid denoise filter

        hFilter = GFilterDM->GetFilterFromName(Fuid::PyramidDenoise_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Bilateral filter on AB

        hFilter = GFilterDM->GetFilterFromName(Fuid::ColorDenoise_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Detail / denoise curve

        hFilter = GFilterDM->GetFilterFromName(Fuid::DetailCurve_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Detail curve"));
          hFilter->runFilter(m_Image_AfterLabSN);
          TRACEMAIN("Done detail curve at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Gradient Sharpen

        hFilter = GFilterDM->GetFilterFromName(Fuid::GradientSharpen_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Wiener Filter Sharpen

        hFilter = GFilterDM->GetFilterFromName(Fuid::Wiener_LabSN);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabSN);
        }


        //***************************************************************************
        // Inverse Diffusion Sharpen

        if (Settings->ToolIsActive("TabInverseDiffusion")) {

          m_ReportProgress(tr("Inverse Diffusion Sharpen"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());
          }

          ptCimgSharpen(m_Image_AfterLabSN,
                        1,
                        Settings->GetDouble("InverseDiffusionAmplitude"),
                        Settings->GetInt("InverseDiffusionIterations"),
                        Settings->GetInt("InverseDiffusionUseEdgeMask"));

          TRACEMAIN("Done Inverse Diffusion Sharpen at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // USM

        if (Settings->ToolIsActive("TabLABUSM")) {

          m_ReportProgress(tr("USM sharpening"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

          }

  //        m_Image_AfterLabSN->USM(1,
  //                              Settings->GetDouble("USMRadius")*m_ScaleFactor,
  //                              Settings->GetDouble("USMAmount"),
  //                              Settings->GetDouble("USMThreshold"));

          m_Image_AfterLabSN->ptGMUnsharp(Settings->GetDouble("USMRadius")*m_ScaleFactor,
                                          Settings->GetDouble("USMAmount"),
                                          Settings->GetDouble("USMThreshold"));

          TRACEMAIN("Done USM at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Highpass

        if (Settings->ToolIsActive("TabLABHighpass")) {


          m_ReportProgress(tr("Highpass"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

          }

          m_Image_AfterLabSN->Highpass(Settings->GetDouble("HighpassRadius")*m_ScaleFactor,
                                     Settings->GetDouble("HighpassAmount"),
                                     -0.3,
                                     Settings->GetDouble("HighpassDenoise")/((logf(m_ScaleFactor)/logf(0.5))+1.0));

          TRACEMAIN("Done Highpass at %d ms.",FRunTimer.elapsed());
        }



        //***************************************************************************
        // Grain

        if (Settings->ToolIsActive("TabLABFilmGrain") &&
            Settings->GetInt("Grain1MaskType")) {

          m_ReportProgress(tr("Film grain 1"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

          }
          m_Image_AfterLabSN->Grain(Settings->GetDouble("Grain1Strength"),
                                    Settings->GetInt("Grain1Mode"),
                                    Settings->GetDouble("Grain1Radius"),
                                    Settings->GetDouble("Grain1Opacity"),
                                    Settings->GetInt("Grain1MaskType"),
                                    Settings->GetDouble("Grain1LowerLimit"),
                                    Settings->GetDouble("Grain1UpperLimit"),
                                    (int)(logf(m_ScaleFactor)/logf(0.5)));
          TRACEMAIN("Done film grain 1 at %d ms.",FRunTimer.elapsed());
        }

        if (Settings->ToolIsActive("TabLABFilmGrain") &&
            Settings->GetInt("Grain2MaskType")) {

          m_ReportProgress(tr("Film grain 2"));

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());

          }
          m_Image_AfterLabSN->Grain(Settings->GetDouble("Grain2Strength"),
                                    Settings->GetInt("Grain2Mode"),
                                    Settings->GetDouble("Grain2Radius"),
                                    Settings->GetDouble("Grain2Opacity"),
                                    Settings->GetInt("Grain2MaskType"),
                                    Settings->GetDouble("Grain2LowerLimit"),
                                    Settings->GetDouble("Grain2UpperLimit"),
                                    (int)(logf(m_ScaleFactor)/logf(0.5)));
          TRACEMAIN("Done film grain 2 at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // View LAB

        if (Settings->ToolIsActive("TabLABViewLab")) {

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabSN->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabSN->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());
          }
          m_ReportProgress(tr("View LAB"));

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

        if (Settings->GetInt("JobMode")) {
          m_Image_AfterLabEyeCandy = m_Image_AfterLabSN; // Job mode -> no cache
        } else {
          if (!m_Image_AfterLabEyeCandy) m_Image_AfterLabEyeCandy = new ptImage();
          m_Image_AfterLabEyeCandy->Set(m_Image_AfterLabSN);
        }

        //***************************************************************************
        // Outline

        hFilter = GFilterDM->GetFilterFromName(Fuid::Outline_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying Outline"));
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
          TRACEMAIN("Done Outline at %d ms.",FRunTimer.elapsed());
        }

        //***************************************************************************
        // LByHue Curve

        hFilter = GFilterDM->GetFilterFromName(Fuid::LumaByHueCurve_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying L by Hue curve"));
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
          TRACEMAIN("Done L by Hue Curve at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Saturation Curve

        hFilter = GFilterDM->GetFilterFromName(Fuid::SatCurve_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying saturation curve"));
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
          TRACEMAIN("Done saturation Curve at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Hue Curve
        hFilter = GFilterDM->GetFilterFromName(Fuid::HueCurve_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying hue curve"));
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
          TRACEMAIN("Done hue Curve at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // L Curve

        hFilter = GFilterDM->GetFilterFromName(Fuid::LCurve_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying L curve"));
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
          TRACEMAIN("Done L Curve at %d ms.",FRunTimer.elapsed());
        }

        //***************************************************************************
        // a b Curves

        hFilter = GFilterDM->GetFilterFromName(Fuid::ABCurves_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying a* b* curves"));
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
          TRACEMAIN("Done a* b* curves at %d ms.",FRunTimer.elapsed());
        }

        
        //***************************************************************************
        // Colorcontrast

        hFilter = GFilterDM->GetFilterFromName(Fuid::ColorContrast_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
        }


        //***************************************************************************
        // LAB Tone adjustments

        hFilter = GFilterDM->GetFilterFromName(Fuid::ToneAdjust1_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::ToneAdjust2_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
        }


        //***************************************************************************
        // Luminance adjustment

        hFilter = GFilterDM->GetFilterFromName(Fuid::LumaAdjust_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
        }


        //***************************************************************************
        // Saturation adjustment

        hFilter = GFilterDM->GetFilterFromName(Fuid::SatAdjust_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
        }


        //***************************************************************************
        // LAB Tone second stage

        hFilter = GFilterDM->GetFilterFromName(Fuid::Tone_LabEyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterLabEyeCandy);
        }


        //***************************************************************************
        // Vignette

        if (Settings->ToolIsActive("TabLABVignette")) {

          //Postponed RGBToLab for performance.
          if (m_Image_AfterLabEyeCandy->m_ColorSpace != ptSpace_Lab) {
            m_Image_AfterLabEyeCandy->RGBToLab();

            TRACEMAIN("Done conversion to LAB at %d ms.",
                      FRunTimer.elapsed());
          }

          m_ReportProgress(tr("Lab Vignette"));

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

        if (Settings->GetInt("JobMode")) {
          m_Image_AfterEyeCandy = m_Image_AfterLabEyeCandy; // Job mode -> no cache
        } else {
          if (!m_Image_AfterEyeCandy) m_Image_AfterEyeCandy = new ptImage();
          m_Image_AfterEyeCandy->Set(m_Image_AfterLabEyeCandy);
        }

        // Has to be here to allow L histogram in Tabmode
        // Come back from Lab space if we ever were there ...
        if (m_Image_AfterEyeCandy->m_ColorSpace == ptSpace_Lab) {
          m_ReportProgress(tr("Lab to RGB"));
          m_Image_AfterEyeCandy->LabToRGB(Settings->GetInt("WorkColor"));
          TRACEMAIN("Done conversion to RGB at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Black & White Styler

        if (Settings->ToolIsActive("TabBW")) {

          m_ReportProgress(tr("Black and White"));

          m_Image_AfterEyeCandy->BWStyler(Settings->GetInt("BWStylerFilmType"),
            Settings->GetInt("BWStylerColorFilterType"),
            Settings->GetDouble("BWStylerMultR"),
            Settings->GetDouble("BWStylerMultG"),
            Settings->GetDouble("BWStylerMultB"),
            Settings->GetDouble("BWStylerOpacity"));
        }


        //***************************************************************************
        // Simple Tone

        if (Settings->ToolIsActive("TabSimpleTone")) {

          m_ReportProgress(tr("Simple Toning"));

          m_Image_AfterEyeCandy->SimpleTone(Settings->GetDouble("SimpleToneR"),
              Settings->GetDouble("SimpleToneG"),
              Settings->GetDouble("SimpleToneB"));
        }


        //***************************************************************************
        // Tone

        if (Settings->ToolIsActive("TabRGBTone1")) {

          m_ReportProgress(tr("Toning 1"));

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

        if (Settings->ToolIsActive("TabRGBTone2")) {

          m_ReportProgress(tr("Toning 2"));

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


        //***************************************************************************
        // Crossprocessing

        if (Settings->ToolIsActive("TabCrossProcessing")) {

          m_ReportProgress(tr("Crossprocessing"));

          m_Image_AfterEyeCandy->Crossprocess(Settings->GetInt("CrossprocessingMode"),
                                              Settings->GetDouble("CrossprocessingColor1"),
                                              Settings->GetDouble("CrossprocessingColor2"));
        }


        //***************************************************************************
        // RGB Contrast.

        hFilter = GFilterDM->GetFilterFromName(Fuid::SigContrastRgb_EyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying RGB Contrast"));
          hFilter->runFilter(m_Image_AfterEyeCandy);
        }


        //***************************************************************************
        // Texture Overlay

        if (Settings->ToolIsActive("TabTextureOverlay")) {

          m_ReportProgress(tr("Texture Overlay"));

          if (!m_Image_TextureOverlay) m_Image_TextureOverlay = new ptImage();

          if (!m_Image_TextureOverlay->m_Image) {
            // No image in cache!
            int Success = 0;

            m_Image_TextureOverlay->ptGMCOpenImage(
              (Settings->GetString("TextureOverlayFile")).toLocal8Bit().data(),
              Settings->GetInt("WorkColor"),
              Settings->GetInt("PreviewColorProfileIntent"),
              0,
              false,
              nullptr,
              Success);

            if (Success == 0) {
              ptMessageBox::critical(0,"File not found","Please open a valid image for texture overlay.");
              Settings->SetValue("TextureOverlayMode",0);
            }
          }

          // Only proceed if we have an image!
          if (m_Image_TextureOverlay->m_Image != NULL) {
            // work on a temporary copy
            ptImage *TempImage = new ptImage();
            TempImage->Set(m_Image_TextureOverlay);

            // Resize, original sized image in cache
            TempImage->ptGMResizeWH(m_Image_AfterEyeCandy->m_Width,
                                    m_Image_AfterEyeCandy->m_Height,
                                    ptIMFilter_Catrom);

            float Value = ((Settings->GetDouble("TextureOverlaySaturation")-1.0)*100);
            TChannelMatrix VibranceMixer;

            VibranceMixer[0][0] = 1.0+(Value/150.0);
            VibranceMixer[0][1] = -(Value/300.0);
            VibranceMixer[0][2] = VibranceMixer[0][1];
            VibranceMixer[1][0] = VibranceMixer[0][1];
            VibranceMixer[1][1] = VibranceMixer[0][0];
            VibranceMixer[1][2] = VibranceMixer[0][1];
            VibranceMixer[2][0] = VibranceMixer[0][1];
            VibranceMixer[2][1] = VibranceMixer[0][1];
            VibranceMixer[2][2] = VibranceMixer[0][0];

            TempImage->mixChannels(VibranceMixer);



            if (Settings->GetInt("TextureOverlayMask")) {
              float *VignetteMask;
              VignetteMask = TempImage->GetVignetteMask(Settings->GetInt("TextureOverlayMask")-1,
                                                        Settings->GetInt("TextureOverlayExponent"),
                                                        Settings->GetDouble("TextureOverlayInnerRadius"),
                                                        Settings->GetDouble("TextureOverlayOuterRadius"),
                                                        Settings->GetDouble("TextureOverlayRoundness"),
                                                        Settings->GetDouble("TextureOverlayCenterX"),
                                                        Settings->GetDouble("TextureOverlayCenterY"),
                                                        Settings->GetDouble("TextureOverlaySoftness"));

              m_Image_AfterEyeCandy->Overlay(TempImage->m_Image,
                                             Settings->GetDouble("TextureOverlayOpacity"),
                                             VignetteMask,
                                             Settings->GetInt("TextureOverlayMode"));
              FREE(VignetteMask);
            } else {
              m_Image_AfterEyeCandy->Overlay(TempImage->m_Image,
                                             Settings->GetDouble("TextureOverlayOpacity"),
                                             NULL,
                                             Settings->GetInt("TextureOverlayMode"));
            }
            delete TempImage;
          }
        }


        //***************************************************************************
        // Texture Overlay 2

        if (Settings->ToolIsActive("TabTextureOverlay2")) {

          m_ReportProgress(tr("Texture Overlay 2"));

          if (!m_Image_TextureOverlay2) m_Image_TextureOverlay2 = new ptImage();

          if (!m_Image_TextureOverlay2->m_Image) {
            // No image in cache!
            int Success = 0;

            m_Image_TextureOverlay2->ptGMCOpenImage(
              (Settings->GetString("TextureOverlay2File")).toLocal8Bit().data(),
              Settings->GetInt("WorkColor"),
              Settings->GetInt("PreviewColorProfileIntent"),
              0,
              false,
              nullptr,
              Success);

            if (Success == 0) {
              ptMessageBox::critical(0,"File not found","Please open a valid image for texture overlay.");
              Settings->SetValue("TextureOverlay2Mode",0);
            }
          }

          // Only proceed if we have an image!
          if (m_Image_TextureOverlay2->m_Image != NULL) {
            // work on a temporary copy
            ptImage *TempImage = new ptImage();
            TempImage->Set(m_Image_TextureOverlay2);

            // Resize, original sized image in cache
            TempImage->ptGMResizeWH(m_Image_AfterEyeCandy->m_Width,
                                    m_Image_AfterEyeCandy->m_Height,
                                    ptIMFilter_Catrom);

            float Value = ((Settings->GetDouble("TextureOverlay2Saturation")-1.0)*100);
            TChannelMatrix VibranceMixer;

            VibranceMixer[0][0] = 1.0f+(Value/150.0f);
            VibranceMixer[0][1] = -(Value/300.0f);
            VibranceMixer[0][2] = VibranceMixer[0][1];
            VibranceMixer[1][0] = VibranceMixer[0][1];
            VibranceMixer[1][1] = VibranceMixer[0][0];
            VibranceMixer[1][2] = VibranceMixer[0][1];
            VibranceMixer[2][0] = VibranceMixer[0][1];
            VibranceMixer[2][1] = VibranceMixer[0][1];
            VibranceMixer[2][2] = VibranceMixer[0][0];

            TempImage->mixChannels(VibranceMixer);



            if (Settings->GetInt("TextureOverlay2Mask")) {
              float *VignetteMask;
              VignetteMask = TempImage->GetVignetteMask(Settings->GetInt("TextureOverlay2Mask")-1,
                                                        Settings->GetInt("TextureOverlay2Exponent"),
                                                        Settings->GetDouble("TextureOverlay2InnerRadius"),
                                                        Settings->GetDouble("TextureOverlay2OuterRadius"),
                                                        Settings->GetDouble("TextureOverlay2Roundness"),
                                                        Settings->GetDouble("TextureOverlay2CenterX"),
                                                        Settings->GetDouble("TextureOverlay2CenterY"),
                                                        Settings->GetDouble("TextureOverlay2Softness"));

              m_Image_AfterEyeCandy->Overlay(TempImage->m_Image,
                                             Settings->GetDouble("TextureOverlay2Opacity"),
                                             VignetteMask,
                                             Settings->GetInt("TextureOverlay2Mode"));
              FREE(VignetteMask);
            } else {
              m_Image_AfterEyeCandy->Overlay(TempImage->m_Image,
                                             Settings->GetDouble("TextureOverlay2Opacity"),
                                             NULL,
                                             Settings->GetInt("TextureOverlay2Mode"));
            }
            delete TempImage;
          }
        }


        //***************************************************************************
        // Gradual Overlay

        if (Settings->ToolIsActive("TabGradualOverlay1")) {

          m_ReportProgress(tr("Gradual Overlay 1"));

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

          m_ReportProgress(tr("Gradual Overlay 2"));

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


        //***************************************************************************
        // Vignette

        if (Settings->ToolIsActive("TabRGBVignette")) {

          m_ReportProgress(tr("Vignette"));

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

        //***************************************************************************
        // Gradual Blur 1
        if (Settings->ToolIsActive("TabGradualBlur1")) {
          m_ReportProgress(tr("Gradual Blur 1"));

          m_Image_AfterEyeCandy->GradualBlur(Settings->GetInt("GradBlur1"),
                                             Settings->GetDouble("GradBlur1Radius")*m_ScaleFactor,
                                             Settings->GetDouble("GradBlur1LowerLevel"),
                                             Settings->GetDouble("GradBlur1UpperLevel"),
                                             Settings->GetDouble("GradBlur1Softness"),
                                             Settings->GetDouble("GradBlur1Angle"),
                                             Settings->GetInt("GradBlur1Vignette"),
                                             Settings->GetDouble("GradBlur1Roundness"),
                                             Settings->GetDouble("GradBlur1CenterX"),
                                             Settings->GetDouble("GradBlur1CenterY"));

          TRACEMAIN("Done gradual blur 1 at %d ms.",FRunTimer.elapsed());
        }

        //***************************************************************************
        // Gradual Blur 2
        if (Settings->ToolIsActive("TabGradualBlur2")) {
          m_ReportProgress(tr("Gradual Blur 2"));

          m_Image_AfterEyeCandy->GradualBlur(Settings->GetInt("GradBlur2"),
                                             Settings->GetDouble("GradBlur2Radius")*m_ScaleFactor,
                                             Settings->GetDouble("GradBlur2LowerLevel"),
                                             Settings->GetDouble("GradBlur2UpperLevel"),
                                             Settings->GetDouble("GradBlur2Softness"),
                                             Settings->GetDouble("GradBlur2Angle"),
                                             Settings->GetInt("GradBlur2Vignette"),
                                             Settings->GetDouble("GradBlur2Roundness"),
                                             Settings->GetDouble("GradBlur2CenterX"),
                                             Settings->GetDouble("GradBlur2CenterY"));

          TRACEMAIN("Done gradual blur 2 at %d ms.",FRunTimer.elapsed());
        }


        //***************************************************************************
        // Softglow

        if (Settings->ToolIsActive("TabSoftglow")) {

          m_ReportProgress(tr("Softglow"));

          m_Image_AfterEyeCandy->Softglow(Settings->GetInt("SoftglowMode"),
            Settings->GetDouble("SoftglowRadius")*m_ScaleFactor,
            Settings->GetDouble("SoftglowAmount"),
            7,
            Settings->GetDouble("SoftglowContrast"),
            Settings->GetInt("SoftglowSaturation"));

        }

        //***************************************************************************
        // Vibrance

        hFilter = GFilterDM->GetFilterFromName(Fuid::ColorIntensity_EyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(hFilter->caption());
          hFilter->runFilter(m_Image_AfterEyeCandy);
        }


        //***************************************************************************
        // Tone curves

        hFilter = GFilterDM->GetFilterFromName(Fuid::RTone_EyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying R curve"));
          hFilter->runFilter(m_Image_AfterEyeCandy);
          TRACEMAIN("Done R Curve at %d ms.",FRunTimer.elapsed());
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::GTone_EyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying G curve"));
          hFilter->runFilter(m_Image_AfterEyeCandy);
          TRACEMAIN("Done G Curve at %d ms.",FRunTimer.elapsed());
        }

        hFilter = GFilterDM->GetFilterFromName(Fuid::BTone_EyeCandy);
        if (hFilter->isActive()) {
          m_ReportProgress(tr("Applying B curve"));
          hFilter->runFilter(m_Image_AfterEyeCandy);
          TRACEMAIN("Done B Curve at %d ms.",FRunTimer.elapsed());
        }


  //***************************************************************************
  Exit:

        Settings->SetValue("PipeImageW",m_Image_AfterEyeCandy->m_Width);
        Settings->SetValue("PipeImageH",m_Image_AfterEyeCandy->m_Height);

      case ptProcessorPhase_Output : // Run Output.

        TRACEMAIN("Done pipe processing at %d ms.",FRunTimer.elapsed());

        break;

      default : // Should not happen.
        assert(!"Invalid processor phase in ptProcessor::Run()");
    }

    m_ReportProgress(tr("Ready"));
  } catch (std::bad_alloc) {
    printf("\n*************************\n\nMemory error in processor\n\n*************************\n\n");
    fflush(stdout);
    throw std::bad_alloc();
  }
}

//==============================================================================

void ptProcessor::RunLocalEdit(ptProcessorStopBefore StopBefore) {
  ptFilterBase *hFilter = nullptr;

  // We fetch the image from the input processing
  if (!Settings->useRAWHandling()) {
    // image is a bitmap
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Transfer Bitmap"));
    }

    // This will be equivalent to m_PipeSize EXCEPT if overwritten
    // by the FinalRun setting that will be always in full size.
    // 0 for full, 1 for half, 2 for quarter (useable in >> operators)
    Settings->SetValue("Scaled", Settings->GetInt("PipeSize"));

    if (Settings->GetInt("JobMode") == 1) // FinalRun!
      Settings->SetValue("Scaled", 0);

    if (!m_Image_AfterLocalEdit)
      m_Image_AfterLocalEdit = new ptImage();

    m_Image_AfterLocalEdit->SetScaled(m_Image_AfterDcRaw,
                                    Settings->GetInt("Scaled"));

    if (Settings->GetInt("DetailViewActive") == 1) {
      m_Image_AfterLocalEdit->Crop(Settings->GetInt("DetailViewCropX") >> Settings->GetInt("Scaled"),
                                   Settings->GetInt("DetailViewCropY") >> Settings->GetInt("Scaled"),
                                   Settings->GetInt("DetailViewCropW") >> Settings->GetInt("Scaled"),
                                   Settings->GetInt("DetailViewCropH") >> Settings->GetInt("Scaled"));
    }

    // The full image width and height is already set.
    // This is the current size:
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEKEYVALS("ImageW","%d",m_Image_AfterLocalEdit->m_Width);
      TRACEKEYVALS("ImageH","%d",m_Image_AfterLocalEdit->m_Height);
      TRACEMAIN("Done bitmap transfer at %d ms.", FRunTimer.elapsed());
    }

  } else {
    // image is raw
    if (Settings->GetInt("JobMode")) {
      m_Image_AfterLocalEdit = m_Image_AfterDcRaw; // Job mode -> no cache
    } else {
      if (!m_Image_AfterLocalEdit) m_Image_AfterLocalEdit = new ptImage();
      m_Image_AfterLocalEdit->Set(m_Image_AfterDcRaw);
    }
  }

  // We have the image for the pipe in linear RGB.

  if (StopBefore == ptProcessorStopBefore::SpotTuning) return;

  // execute local adjust spots
  hFilter = GFilterDM->GetFilterFromName(Fuid::SpotTuning_Local);
  if (hFilter->isActive()) {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Spot tuning"));
    }

    hFilter->runFilter(m_Image_AfterLocalEdit);
  }

  // Output of “Local” tab is Lch, but we need RGB for the following “Geometry” tab.
  m_Image_AfterLocalEdit->LchToRGB(Settings->GetInt("WorkColor"));
}

//==============================================================================

void ptProcessor::RunGeometry(ptProcessorStopBefore StopBefore) {
  if (!m_Image_AfterGeometry) m_Image_AfterGeometry = new ptImage();
  m_Image_AfterGeometry->Set(m_Image_AfterLocalEdit);

  // Often used.
  int TmpScaled = Settings->GetInt("Scaled");

  //***************************************************************************
  // Lensfun
  if (Settings->ToolIsActive("TabLensfunCA") || Settings->ToolIsActive("TabLensfunVignette") ||
      Settings->ToolIsActive("TabLensfunDistortion") || Settings->ToolIsActive("TabLensfunGeometry") )
  {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Lensfun corrections"));
    }
    int modflags = 0;

    lfLensCalibTCA TCAData;
    TCAData.Model = (lfTCAModel)(Settings->GetInt("LfunCAModel"));
    TCAData.Focal = Settings->GetDouble("LfunFocal");
    switch (TCAData.Model) {
      case LF_TCA_MODEL_NONE:
        TCAData.Terms[0] = 0.0;
        TCAData.Terms[1] = 0.0;
        TCAData.Terms[2] = 0.0;
        TCAData.Terms[3] = 0.0;
        TCAData.Terms[4] = 0.0;
        TCAData.Terms[5] = 0.0;
        break;
      case LF_TCA_MODEL_LINEAR:
        modflags |= LF_MODIFY_TCA;
        TCAData.Terms[0] = Settings->GetDouble("LfunCALinearKr");
        TCAData.Terms[1] = Settings->GetDouble("LfunCALinearKb");
        TCAData.Terms[2] = 0.0;
        TCAData.Terms[3] = 0.0;
        TCAData.Terms[4] = 0.0;
        TCAData.Terms[5] = 0.0;
        break;
      case LF_TCA_MODEL_POLY3:
        modflags |= LF_MODIFY_TCA;
        TCAData.Terms[0] = Settings->GetDouble("LfunCAPoly3Vr");
        TCAData.Terms[1] = Settings->GetDouble("LfunCAPoly3Vb");
        TCAData.Terms[2] = Settings->GetDouble("LfunCAPoly3Cr");
        TCAData.Terms[3] = Settings->GetDouble("LfunCAPoly3Cb");
        TCAData.Terms[4] = Settings->GetDouble("LfunCAPoly3Br");
        TCAData.Terms[5] = Settings->GetDouble("LfunCAPoly3Bb");
        break;
      default:
        assert(0);
    }
    lfLensCalibVignetting VignetteData;
    VignetteData.Model = (lfVignettingModel)(Settings->GetInt("LfunVignetteModel"));
    VignetteData.Focal = Settings->GetDouble("LfunFocal");
    VignetteData.Aperture = Settings->GetDouble("LfunAperture");
    VignetteData.Distance = Settings->GetDouble("LfunDistance");
    switch (VignetteData.Model) {
      case LF_VIGNETTING_MODEL_NONE:
        VignetteData.Terms[0] = 0.0;
        VignetteData.Terms[1] = 0.0;
        VignetteData.Terms[2] = 0.0;
        break;
      case LF_VIGNETTING_MODEL_PA:
        modflags |= LF_MODIFY_VIGNETTING;
        VignetteData.Terms[0] = Settings->GetDouble("LfunVignettePoly6K1");
        VignetteData.Terms[1] = Settings->GetDouble("LfunVignettePoly6K2");
        VignetteData.Terms[2] = Settings->GetDouble("LfunVignettePoly6K3");
        break;
      default:
        assert(0);
    }

    lfLensCalibDistortion DistortionData;
    DistortionData.Model = (lfDistortionModel)(Settings->GetInt("LfunDistModel"));
    DistortionData.Focal = Settings->GetDouble("LfunFocal");
    switch (DistortionData.Model) {
      case LF_DIST_MODEL_NONE:
        DistortionData.Terms[0] = 0.0;
        DistortionData.Terms[1] = 0.0;
        DistortionData.Terms[2] = 0.0;
        break;
      case LF_DIST_MODEL_POLY3:
        modflags |= LF_MODIFY_DISTORTION;
        DistortionData.Terms[0] = Settings->GetDouble("LfunDistPoly3K1");
        DistortionData.Terms[1] = 0.0;
        DistortionData.Terms[2] = 0.0;
        break;
      case LF_DIST_MODEL_POLY5:
        modflags |= LF_MODIFY_DISTORTION;
        DistortionData.Terms[0] = Settings->GetDouble("LfunDistPoly5K1");
        DistortionData.Terms[1] = Settings->GetDouble("LfunDistPoly5K2");
        DistortionData.Terms[2] = 0.0;
        break;
      case LF_DIST_MODEL_FOV1:
        modflags |= LF_MODIFY_DISTORTION;
        DistortionData.Terms[0] = Settings->GetDouble("LfunDistFov1Omega");
        DistortionData.Terms[1] = 0.0;
        DistortionData.Terms[2] = 0.0;
        break;
      case LF_DIST_MODEL_PTLENS:
        modflags |= LF_MODIFY_DISTORTION;
        DistortionData.Terms[0] = Settings->GetDouble("LfunDistPTLensA");
        DistortionData.Terms[1] = Settings->GetDouble("LfunDistPTLensB");
        DistortionData.Terms[2] = Settings->GetDouble("LfunDistPTLensC");
        break;
      default:
        assert(0);
    }

    lfLens LensData = lfLens();
    LensData.Type = (lfLensType)(Settings->GetInt("LfunSrcGeo"));
    LensData.SetMaker("Photivo Custom");
    LensData.SetModel("Photivo Custom");
    LensData.AddMount("Photivo Custom");
    LensData.AddCalibTCA(&TCAData);
    LensData.AddCalibVignetting(&VignetteData);
    LensData.AddCalibDistortion(&DistortionData);
    assert(LensData.Check());
    lfModifier* LfunData = lfModifier::Create(&LensData,
                                              1.0,  // focal length always normalised to 35mm equiv.
                                              m_Image_AfterGeometry->m_Width,
                                              m_Image_AfterGeometry->m_Height);

    // complete list of desired modify actions
    lfLensType TargetGeo = (lfLensType)(Settings->GetInt("LfunTargetGeo"));
    if (LensData.Type != TargetGeo) {
      modflags |= LF_MODIFY_GEOMETRY;
    }

    double LfunScaleFactor = 0.0;   // 0.0 is lensfun's magic value for auto scaling
    if (Settings->GetInt("LfunAutoScale") == 0) {
      LfunScaleFactor = Settings->GetDouble("LfunScale");
    }
    if (qAbs(LfunScaleFactor - 1.0) > 0.001) {
      modflags |= LF_MODIFY_SCALE;
    }

    // Init modifier and get list of lensfun actions that actually get performed
    modflags = LfunData->Initialize(&LensData,
                                    LF_PF_U16,  //image is uint16 data
                                    Settings->GetDouble("LfunFocal"),
                                    Settings->GetDouble("LfunAperture"),
                                    Settings->GetDouble("LfunDistance"),
                                    LfunScaleFactor,
                                    TargetGeo,
                                    modflags,
                                    false);  //distortion correction, not dist. simulation

    // Execute lensfun corrections. For vignetting the image is changed in place.
    // For everything else new pixel coordinates are returned in TransformedCoords.
    m_Image_AfterGeometry->Lensfun(modflags, LfunData);
    LfunData->Destroy();

    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEMAIN("Done Lensfun corrections at %d ms.",FRunTimer.elapsed());
    }
  }


  //***************************************************************************
  // Defish (dedicated lensfun)
  if (Settings->ToolIsActive("TabDefish"))
  {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Defish correction"));
    }

    lfLens LensData = lfLens();
    LensData.Type = (lfLensType)LF_FISHEYE;
    LensData.SetMaker("Photivo Custom");
    LensData.SetModel("Photivo Custom");
    LensData.AddMount("Photivo Custom");
    assert(LensData.Check());
    lfModifier* LfunData = lfModifier::Create(&LensData,
                                              1.0,  // focal length always normalised to 35mm equiv.
                                              m_Image_AfterGeometry->m_Width,
                                              m_Image_AfterGeometry->m_Height);

    // complete list of desired modify actions
    lfLensType TargetGeo = (lfLensType)LF_RECTILINEAR;
    int modflags = LF_MODIFY_GEOMETRY;

    double DefishScaleFactor = 0.0;   // 0.0 is lensfun's magic value for auto scaling
    if (Settings->GetInt("DefishAutoScale") == 0) {
      DefishScaleFactor = Settings->GetDouble("DefishScale");
    }
    if (qAbs(DefishScaleFactor - 1.0) > 0.001) {
      modflags |= LF_MODIFY_SCALE;
    }

    // Init modifier and get list of lensfun actions that actually get performed
    modflags = LfunData->Initialize(&LensData,
                                    LF_PF_U16,  //image is uint16 data
                                    Settings->GetDouble("DefishFocalLength"),
                                    1.0, // doesn't matter for defish
                                    1.0, // doesn't matter for defish
                                    DefishScaleFactor,
                                    TargetGeo,
                                    modflags,
                                    false);  //distortion correction, not dist. simulation

    // Execute lensfun corrections. For vignetting the image is changed in place.
    // For everything else new pixel coordinates are returned in TransformedCoords.
    m_Image_AfterGeometry->Lensfun(modflags, LfunData);
    LfunData->Destroy();

    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEMAIN("Done defish correction at %d ms.",FRunTimer.elapsed());
    }
  }


  //***************************************************************************
  // Stop here when activating rotate tool (the draw-line-to-get-angle mode)
  if (StopBefore == ptProcessorStopBefore::Rotate) {
    return;
  }


  //***************************************************************************
  // Rotation
  if (Settings->ToolIsActive("TabRotation")) {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Perspective transform"));
    }

    m_Image_AfterGeometry->ptCIPerspective(Settings->GetDouble("Rotate"),
                                           Settings->GetDouble("PerspectiveFocalLength"),
                                           Settings->GetDouble("PerspectiveTilt"),
                                           Settings->GetDouble("PerspectiveTurn"),
                                           Settings->GetDouble("PerspectiveScaleX"),
                                           Settings->GetDouble("PerspectiveScaleY"));

    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEMAIN("Done perspective at %d ms.",FRunTimer.elapsed());
    }
  }

  // Remember sizes after Rotation, also valid if there
  // was no rotation. Expressed in terms of the original
  // non scaled image.
  Settings->SetValue("RotateW",m_Image_AfterGeometry->m_Width << TmpScaled);
  Settings->SetValue("RotateH",m_Image_AfterGeometry->m_Height<< TmpScaled);

  if (StopBefore == ptProcessorStopBefore::NoStop) {
    TRACEKEYVALS("RotateW","%d",Settings->GetInt("RotateW"));
    TRACEKEYVALS("RotateH","%d",Settings->GetInt("RotateH"));
  }


  //***************************************************************************
  // Stop here when activating crop tool
  if (StopBefore == ptProcessorStopBefore::Crop) {
    return;
  }


  //***************************************************************************
  // Crop
  if (Settings->ToolIsActive("TabCrop")) {

    if (((Settings->GetInt("CropX") >> TmpScaled) + (Settings->GetInt("CropW") >> TmpScaled))
          > m_Image_AfterGeometry->m_Width ||
        ((Settings->GetInt("CropY") >> TmpScaled) + (Settings->GetInt("CropH") >> TmpScaled))
          > m_Image_AfterGeometry->m_Height) {
      ptMessageBox::information(0,
                               tr("Crop outside the image"),
                               tr("Crop rectangle too large.\nNo crop, try again."));
      Settings->SetValue("CropX",0);
      Settings->SetValue("CropY",0);
      Settings->SetValue("CropW",0);
      Settings->SetValue("CropH",0);
      Settings->SetValue("Crop",0);
    } else {
      if (StopBefore == ptProcessorStopBefore::NoStop) {
        TRACEKEYVALS("CropW","%d",Settings->GetInt("CropW"));
        TRACEKEYVALS("CropH","%d",Settings->GetInt("CropH"));
        m_ReportProgress(tr("Cropping"));
      }

      m_Image_AfterGeometry->Crop(Settings->GetInt("CropX") >> TmpScaled,
                                 Settings->GetInt("CropY") >> TmpScaled,
                                 Settings->GetInt("CropW") >> TmpScaled,
                                 Settings->GetInt("CropH") >> TmpScaled);

      if (StopBefore == ptProcessorStopBefore::NoStop) {
        TRACEMAIN("Done cropping at %d ms.",FRunTimer.elapsed());
      }
    }
  }

  if (StopBefore == ptProcessorStopBefore::NoStop) {
    TRACEKEYVALS("CropW","%d",m_Image_AfterGeometry->m_Width << TmpScaled);
    TRACEKEYVALS("CropH","%d",m_Image_AfterGeometry->m_Height<< TmpScaled);
  }

  // set scale factor for size dependend filters
  m_ScaleFactor = 1/powf(2.0, Settings->GetInt("Scaled"));


  //***************************************************************************
  // Liquid rescale
  if (Settings->ToolIsActive("TabLiquidRescale")) {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Seam carving"));
    }

    if (Settings->GetInt("LqrScaling") == ptLqr_ScaleRelative) {
      m_Image_AfterGeometry->LiquidRescaleRelative(Settings->GetDouble("LqrHorScale"),
                                                   Settings->GetDouble("LqrVertScale"),
                                                   Settings->GetInt("LqrEnergy"),
                                                   Settings->GetInt("LqrVertFirst"));
    } else if (Settings->GetInt("LqrScaling") == ptLqr_ScaleAbsolute) {
      m_Image_AfterGeometry->LiquidRescale(Settings->GetInt("LqrWidth"),
                                           Settings->GetInt("LqrHeight"),
                                           Settings->GetInt("LqrEnergy"),
                                           Settings->GetInt("LqrVertFirst"));
    }
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEMAIN("Done seam carving at %d ms.",FRunTimer.elapsed());
    }
  }

  //***************************************************************************
  // Resize
  if (Settings->ToolIsActive("TabResize")) {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Resize image"));
    }

    float WidthIn = m_Image_AfterGeometry->m_Width;

    m_Image_AfterGeometry->ptGMResize(Settings->GetInt("ResizeScale"),
                                      Settings->GetInt("ResizeHeight"),
                                      Settings->GetInt("ResizeFilter"),
                                      Settings->GetInt("ResizeDimension"));

    m_ScaleFactor = (float) m_Image_AfterGeometry->m_Width/WidthIn/powf(2.0, Settings->GetInt("Scaled"));

    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEMAIN("Done resize at %d ms.",FRunTimer.elapsed());
    }
  }

  //***************************************************************************
  // Flip
  if (Settings->ToolIsActive("TabFlip")) {
    if (StopBefore == ptProcessorStopBefore::NoStop) {
      m_ReportProgress(tr("Flip image"));
    }

    m_Image_AfterGeometry->Flip(Settings->GetInt("FlipMode"));

    if (StopBefore == ptProcessorStopBefore::NoStop) {
      TRACEMAIN("Done flip at %d ms.",FRunTimer.elapsed());
    }
  }

}

//==============================================================================

void ptProcessor::ReadExifBuffer() {
  if (Settings->GetStringList("InputFileNameList").size() == 0) return;

  if (!ptImageHelper::ReadExif(Settings->GetStringList("InputFileNameList")[0],
                               m_ExifData,
                               m_ExifBuffer)) return;

  try {
    // for use by lensfun : Make, Model, Focal Length, FNumber
    Exiv2::ExifData::iterator Pos;

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Image.Make"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
//      Settings->SetValue("LensfunCameraMake",
//        QString(str.str().c_str()).toUpper().trimmed());
    }

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Image.Model"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
//      Settings->SetValue("LensfunCameraModel",
//        QString(str.str().c_str()).toUpper().trimmed());
    }

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      float FNumber;
      sscanf(str.str().c_str(),"%*c%f",&FNumber);
//      Settings->SetValue("LensfunF",FNumber);
    }

    Pos = m_ExifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength"));
    if (Pos != m_ExifData.end() ) {
      std::stringstream str;
      str << *Pos;
      int FocalLength;
      sscanf(str.str().c_str(),"%d",&FocalLength);
//      Settings->SetValue("LensfunFocalLength",FocalLength);
    }

  } catch (Exiv2::Error& Error) {
    // Exiv2 errors are in this context hopefully harmless
    // (unsupported tags etc.)

    ptLogWarning(ptWarning_Exiv2,"Exiv2 : %s\n",Error.what());
  }
}

//==============================================================================

// There should be Settings->m_WhiteFraction % pixels above 90%
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

//==============================================================================

