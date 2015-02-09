/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
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

#include "ptFilterDM.h"
#include "ptFilterFactory.h"
#include "ptFilterUids.h"
#include "../ptDefines.h"
#include "../ptInfo.h"
#include "../ptTempFile.h"
#include "../batch/ptBatchWindow.h"

// deprecated: To be removed when transition to new settings system is complete.
#include "../ptSettings.h"
#include "../ptCurve.h"
#include "../ptMainWindow.h"
#include "../ptError.h"
#include "../ptMessageBox.h"
// end of deprecated

#include <QFileDialog>
#include <QSettings>
#include <cassert>

//==============================================================================

ptFilterDM* ptFilterDM::FInstance = nullptr;

//==============================================================================

/*! Global variable for fast access.*/
ptFilterDM* GFilterDM = nullptr;

//==============================================================================

void ptFilterDM::CreateInstance()
{
  // We complain if it is already created...
  if (FInstance != nullptr)
    GInfo->Raise("FilterDM already created", AT);

  FInstance = new ptFilterDM();
  GFilterDM = FInstance;
}

//==============================================================================

void ptFilterDM::ToNewPreset(QSettings *APreset) {
  TranslatePreset(APreset, true);
}

//==============================================================================

void ptFilterDM::ToOldPreset(QSettings *APreset) {
  TranslatePreset(APreset, false);
}

//==============================================================================

// TODO: Function taken from ptMain with very little changes. Needs cleanup!
// Original comment:
// Setting files contain information about everything needed to process
// the image, they may need string corrections.
// Job files contain additional information about the files to process.
// Preset files contain just information about filters, no string
// correction will be needed.
extern ptMainWindow* MainWindow;
void   PreCalcTransforms();

bool ptFilterDM::ReadPresetFile(const QString &AFileName, short &ANextPhase) {
  QFileInfo Info(AFileName);
  if (!Info.exists()) return ptError_FileOpen;

  // Temporary copy to leave the files untouched
  ptTempFile hTempFile(AFileName);
  QSettings hPreset(hTempFile.fileName(), QSettings::IniFormat);
  if (!(hPreset.value("Magic") == "photivoJobFile" ||
        hPreset.value("Magic") == "photivoSettingsFile" ||
        hPreset.value("Magic") == "dlRawJobFile" ||
        hPreset.value("Magic") == "dlRawSettingsFile" ||
        hPreset.value("Magic") == "photivoPresetFile"))
  {
    ptLogError(ptError_FileFormat,
               "'%s' has wrong format\n",
               AFileName.toLocal8Bit().data());
    return ptError_FileFormat;
  }

  // Color space transformations precalc
  short NeedRecalcTransforms = 0;
  if (hPreset.contains("CMQuality")) {
    if (hPreset.value("CMQuality").toInt() != Settings->GetInt("CMQuality")) {
      NeedRecalcTransforms = 1;
    }
  } else if (hPreset.contains("WorkColor")) {
    if (hPreset.value("WorkColor").toInt() != Settings->GetInt("WorkColor")) {
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
      << "TextureOverlayFile"
      << "TextureOverlay2File";

  QStringList Locations;
  Locations  << "ChannelMixerFileNames"
             << "CurveFileNamesRGB"
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

  QString OldString = QString();
  QString NewString = Settings->GetString("UserDirectory");
  bool CorrectionNeeded = false;

  if (hPreset.contains("UserDirectory") && hPreset.value("Magic") != "photivoPresetFile")   {
    // simplest case: file from a different user directory
    OldString = hPreset.value("UserDirectory").toString();
    if (OldString != NewString) CorrectionNeeded = true;

  } else if (hPreset.contains("ShareDirectory") && hPreset.value("Magic") != "photivoPresetFile")   {
    // new style settings files
    // intermediate settings, just for compatibility
    OldString = hPreset.value("ShareDirectory").toString();
    if (OldString != NewString) CorrectionNeeded = true;

  } else if (hPreset.value("Magic") != "photivoPresetFile") { // old style settings files
    // Adopt the old settings files, just for compatibility.
    // Asumes the LensfunsDatabase dir was the most stable directory
    if (hPreset.contains("LensfunDatabaseDirectory")) {
      OldString = hPreset.value("LensfunDatabaseDirectory").toString();
      OldString.chop(QString("LensfunDatabase").length());
      if (OldString != NewString) CorrectionNeeded = true;
    }
  }

  if (CorrectionNeeded) {
    int LeftPart = OldString.length();
    // Strings
    for (int i = 0; i < Directories.size(); i++) {
      if (hPreset.value(Directories.at(i)).toString().left(LeftPart)==OldString) {
        QString TmpStr = hPreset.value(Directories.at(i)).toString();
        TmpStr.remove(OldString);
        TmpStr.prepend(NewString);
        hPreset.setValue(Directories.at(i),TmpStr);
      }
    }
    // Stringlists
    for (int i = 0; i < Locations.size(); i++) {
      QStringList TempList = hPreset.value(Locations.at(i)).toStringList();
      for (int j = 0; j < TempList.size(); j++) {
        if (TempList.at(j).left(LeftPart)==OldString) {
          QString TmpStr = TempList.at(j);
          TmpStr.remove(OldString);
          TmpStr.prepend(NewString);
          TempList.replace(j,TmpStr);
        }
      }
      hPreset.setValue(Locations.at(i),TempList);
    }
  }

  // All Settings were already read (with 0 remembering)
  // so we can safely use that to have them now overwritten
  // with the jobfile settings.
  QStringList Keys = Settings->GetKeys();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    if (!Settings->GetInJobFile(Key)) continue;
    if (!hPreset.contains(Key)) continue;
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
    QVariant Tmp = hPreset.value(Key);
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


  // Make sure new-style filters get new-style settings.
  this->ToNewPreset(&hPreset);

  // load new-style settings
  for (auto hItem = FFilters.constBegin(); hItem != FFilters.constEnd(); hItem++) {
    hItem.value()->importPreset(&hPreset);
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
  QStringList ProposedHiddenTools = hPreset.value("HiddenTools").toStringList();
  CurrentHiddenTools.removeDuplicates();
  ProposedHiddenTools.removeDuplicates();
  Settings->SetValue("HiddenTools",ProposedHiddenTools);
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
  if (hPreset.contains("NextPhase"))
    ANextPhase = hPreset.value("NextPhase").toInt();

  // Color space transformations precalc
  if (NeedRecalcTransforms == 1) PreCalcTransforms();

  if (MainWindow)
    MainWindow->Settings_2_Form();

  hPreset.sync();
  if (hPreset.status() == QSettings::NoError) {
    return 0;
  }
  assert(hPreset.status() == QSettings::NoError); // TODO
  return ptError_FileFormat;
}

//==============================================================================

extern QString SettingsFilePattern;
bool ptFilterDM::WritePresetFile(const QString       &AFileName,
                                 const bool           AAppend,
                                 const bool           AIncludeFlags,
                                 const ptFilterBase  *AFilter)
{
  QString hFileName = "";

  if (AFileName.isEmpty() || AFileName.endsWith(".")) {
    QString hSuggestion = "";
    if (AFileName.isEmpty()) {
      hSuggestion = Settings->GetString("PresetDirectory") + "/preset.pts";
    } else {
      hSuggestion = AFileName + "pts";
    }

    QString hAppendCaption = QObject::tr("Append settings file");
    QString hSaveCaption   = QObject::tr("Save settings file");
    hFileName = QFileDialog::getSaveFileName(
                          nullptr,
                          AAppend ? hAppendCaption : hSaveCaption,
                          hSuggestion,
                          SettingsFilePattern,
                          nullptr,
                          AAppend ? QFileDialog::DontConfirmOverwrite : (QFileDialog::Option)0
                        );

    // Empty file name means user aborted
    if (hFileName.isEmpty()) {
      return false;
    }
  } else {
    hFileName = AFileName;
  }

  return PerformWritePreset(hFileName, AAppend, AIncludeFlags, false, AFilter);
}

//==============================================================================

extern QString RawPattern;
extern QString JobFilePattern;
bool ptFilterDM::WriteJobFile() {
  // First a dialog to obtain the input for the job.
  QStringList InputFileNames = QFileDialog::getOpenFileNames(
                              NULL,
                              QObject::tr("Select input file(s)"),
                              Settings->GetString("RawsDirectory"),
                              RawPattern);

  // Operation cancelled.
  if (InputFileNames.isEmpty()) return false;

  Settings->SetValue("InputFileNameList",InputFileNames);

  // Then a dialog to obtain the output directory
  QString Directory =
    QFileDialog::getExistingDirectory(NULL,
                                      QObject::tr("Select output directory"),
                                      Settings->GetString("OutputDirectory"));

  // Operation cancelled.
  if (Directory.size() == 0) return false;

  Settings->SetValue("OutputDirectory",Directory);

  // And finally a dialog to obtain the output job file.
  QFileInfo PathInfo(InputFileNames[0]);
  QString SuggestedJobFileName = PathInfo.dir().path() + "/" + PathInfo.completeBaseName() +
                                 Settings->GetString("OutputFileNameSuffix") + ".ptj";

  QString JobFileName =
    QFileDialog::getSaveFileName(NULL,
                                 QObject::tr("Select job file"),
                                 SuggestedJobFileName,
                                 JobFilePattern);


  // Operation cancelled.
  if (JobFileName.size() == 0) return false;

  return PerformWritePreset(JobFileName, false, true, true, nullptr);
}

//==============================================================================

extern ptBatchWindow *BatchWindow;
bool ptFilterDM::SendToBatch(const QString &AFileName) {
  QString hSuggestion = AFileName + "pts";
  QString hFileName = hSuggestion;

  if (Settings->GetInt("SaveConfirmation") != 0) {
    QString hSaveCaption = QObject::tr("Save settings file");
    hFileName = QFileDialog::getSaveFileName(
                        nullptr,
                        hSaveCaption,
                        hSuggestion,
                        SettingsFilePattern,
                        nullptr);

    // Empty file name means user aborted
    if (hFileName.isEmpty())
      return false;
  }

  bool result = PerformWritePreset(hFileName, false, true, false, nullptr);
  BatchWindow->AddJobToList(hFileName, Settings->GetStringList("InputFileNameList").first());
  return result;
}

//==============================================================================

bool ptFilterDM::PerformWritePreset(const QString &AFileName,
                                    const bool AAppend,
                                    const bool AIncludeFlags,
                                    const bool AIsJobFile,
                                    const ptFilterBase *AFilter)
{
  if (!AAppend) {
    if (QFile::exists(AFileName))
      QFile::remove(AFileName);
  }

  QSettings hSettings(AFileName, QSettings::IniFormat);

  // Magic value for identifying a valid settings file
  hSettings.setValue("Magic", "photivoSettingsFile");

  // Fill with filter config data from new-style filters
  if (AFilter) { // only one given filter
    AFilter->exportPreset(&hSettings, AIncludeFlags);
  } else {  // all filters
    for (auto hItem = FFilters.constBegin(); hItem != FFilters.constEnd(); hItem++) {
      hItem.value()->exportPreset(&hSettings, AIncludeFlags);
    }
    // Get old-style settings from ptSettings
    QStringList hOldStyleKeys = Settings->GetKeys();
    hOldStyleKeys.sort();
    for (QString hKey: hOldStyleKeys) {
      if (!Settings->GetInJobFile(hKey))
        continue;
      if (hKey == "InputFileNameList" && !AIsJobFile)
        continue;
      if (hKey == "OutputDirectory" && !AIsJobFile)
        continue;

      hSettings.setValue(hKey, Settings->GetValue(hKey));
    }
  }

  // Until all filters are ported old-style settings are written to file.
  ToOldPreset(&hSettings);

  hSettings.sync();
  return hSettings.status() == QSettings::NoError;
}

//==============================================================================

int ptFilterDM::FindBeginTab(const TCacheGroupList &AList){
  int hTab = -1;
  while (hTab < AList.size()-1) {
    if (AList.at(++hTab).size() > 0)
      break;
  }
  return hTab;
}

//==============================================================================

int ptFilterDM::FindEndTab(const TCacheGroupList &AList){
  int hTab = AList.size();
  while (hTab > 0) {
    if (AList.at(--hTab).size() > 0)
      break;
  }
  return hTab;
}

//==============================================================================

ptFilterIterator ptFilterDM::activeBegin() {
  return ptFilterIterator(&FActiveFilters);
}

//==============================================================================

ptFilterIterator ptFilterDM::begin() {
  return ptFilterIterator(&FOrderedFilters);
}

//==============================================================================

ptFilterIterator ptFilterDM::activeEnd() {
  return ptFilterIterator(&FActiveFilters, true);
}

//==============================================================================

ptFilterIterator ptFilterDM::end() {
  return ptFilterIterator(&FOrderedFilters, true);
}

//==============================================================================

ptFilterBase *ptFilterDM::NewFilter(const QString &AFilterId,
                                    const QString &AUniqueName,
                                    const QString &AGuiNamePostfix /*= ""*/)
{
  if (FFilters.contains(AUniqueName))
    GInfo->Raise("Filter name not unique: " + AUniqueName, AT);

  if (!FFactory->AvailableFilters().contains(AFilterId))
    GInfo->Raise("Filter for AFilterId not available: " + AFilterId, AT);

  ptFilterBase *hFilter = FFactory->CreateFilterFromName(AFilterId);
  hFilter->init(AUniqueName, AGuiNamePostfix);

  FFilters.insert(AUniqueName, hFilter);
  return hFilter;
}

//==============================================================================

void ptFilterDM::UpdatePositions(ptFilterBase *AFilter) {
  // Remove filter at old positions
  for (TCacheGroup hCacheGroup: FOrderedFilters) {
    if (hCacheGroup.removeOne(AFilter))
      break;
  }
  for (TCacheGroup hCacheGroup: FActiveFilters) {
    if (hCacheGroup.removeOne(AFilter))
      break;
  }

  // insert at new position
  InsertToList(FullList, AFilter);
  UpdateActivesList(AFilter);
}

//==============================================================================

ptFilterBase *ptFilterDM::GetFilterFromName(const QString AUniqueName,
                                            const bool AThrowOnNotFound /*= true*/) const {
  ptFilterBase *hFilter = FFilters.value(AUniqueName, nullptr);

  if (!hFilter && AThrowOnNotFound)
    GInfo->Raise("Filter does not exist: " + AUniqueName, AT);

  return hFilter;
}

//==============================================================================

bool ptFilterDM::isActiveCacheGroup(const int AGroupIdx) {
  if (AGroupIdx < 0 || AGroupIdx > FActiveFilters.size()-1)
    return false;   // catch invalid indexes

  return FActiveFilters.at(AGroupIdx).size() > 0;
}

//==============================================================================

void ptFilterDM::UpdateActivesList(ptFilterBase *AFilter) {
  if (AFilter->isActive()) {
    InsertToList(ActiveList, AFilter);
  } else {
    FActiveFilters[AFilter->parentTabIdx()].removeAll(AFilter);
  }
}

//==============================================================================

ptFilterDM::ptFilterDM() {
  // init ordered filter lists. We know we have exactly 9 cache groups == gui tabs
  for (int i = 0; i < 9; ++i) {
    FOrderedFilters.append(TCacheGroup());
    FActiveFilters.append(TCacheGroup());
  }

  FFactory = ptFilterFactory::GetInstance();
  FillNameMap();
}

//==============================================================================

ptFilterDM::~ptFilterDM() {
  FFactory = nullptr;
  for (auto hIter = FFilters.constBegin(); hIter != FFilters.constEnd(); ++hIter) {
    delete hIter.value();
  }
}

//==============================================================================

extern short JobMode;
void ptFilterDM::InsertToList(const TListType AListType, ptFilterBase *AFilter) {
  if (JobMode) return;
  TCacheGroup *hCacheGroup = nullptr;

  switch (AListType) {
    case FullList:   hCacheGroup = &FOrderedFilters[AFilter->parentTabIdx()]; break;
    case ActiveList: hCacheGroup = &FActiveFilters[AFilter->parentTabIdx()]; break;
    default:         GInfo->Raise("Unknown list type.", AT);
  }

  // Filter already in list. Nothing to do.
  if (hCacheGroup->contains(AFilter))
    return;

  // Find insertion point
  int hIdx = 0;
  for (; hIdx < hCacheGroup->size(); hIdx++) {
    if (hCacheGroup->at(hIdx)->idxInParentTab() > AFilter->idxInParentTab())
      break;
  }

  hCacheGroup->insert(hIdx, AFilter);
}

//==============================================================================


void ptFilterDM::TranslatePreset(QSettings *APreset, const bool AOldToNew) {
  auto hKeys = APreset->allKeys();
  if (AOldToNew) {
    TranslateCurvesToNew(APreset, &hKeys);
    TranslateSpecialToNew(APreset, &hKeys);
    TranslateNormalToNew(APreset, &hKeys);
   } else {
    TranslateCurvesToOld(APreset, &hKeys);
    TranslateSpecialToOld(APreset, &hKeys);
    TranslateNormalToOld(APreset, &hKeys);
  }
}

//==============================================================================

void ptFilterDM::TranslateNormalToNew(QSettings *APreset, QStringList *AKeys) {
  for (const QString& hFromKey: *AKeys) {
    QString hToKey = FNameMap.value(hFromKey, "<invalid>");

    if (hToKey != "<invalid>") {
      APreset->setValue(hToKey, APreset->value(hFromKey));
      APreset->remove(hFromKey);
    }
  }
}

//==============================================================================

void ptFilterDM::TranslateNormalToOld(QSettings *APreset, QStringList *AKeys) {
  QStringList hBlockedTools;

  for (QString hFromKey: *AKeys) {
    if (hFromKey.endsWith("isBlocked")) {
      if (APreset->value(hFromKey).toInt() != 0)
        hBlockedTools.append(hFromKey);
      APreset->remove(hFromKey);

    } else if (hFromKey.endsWith("CustomStores")) {
      APreset->remove(hFromKey);

    } else {
      QString hToKey = FNameMap.key(hFromKey, "<invalid>");
      if (hToKey != "<invalid>") {
        APreset->setValue(hToKey, APreset->value(hFromKey));
        APreset->remove(hFromKey);
      }
    }
  }

  if (APreset->contains("BlockedTools")) {
    hBlockedTools.append(Settings->GetStringList("BlockedTools"));
    APreset->setValue("BlockedTools", hBlockedTools);
  }
}

//==============================================================================

/*
  About the old pts format:
  - CurveType: The old combo box index is saved. Index 0 = curve disabled, 1 = manual curve,
    3 and above = curves loaded from files.
  - FileName: Actually a QStringList corresponding to the >=3 CurveType indexes.
  - Mask: Formerly called “type”. Not to confuse with the interpolation type, which also used
    to be called “type”, though with a different prefix. Only exists for curves that support
    several mask types.
*/
void ptFilterDM::TranslateCurvesToNew(QSettings *APreset, QStringList *AKeys) {
  auto hOldCurves = FCurveNameMap.keys();
  for (QString hOldCurveId: hOldCurves) {
    if (!APreset->contains(hOldCurveId))
      continue;

    auto hNewCurveId = FCurveNameMap.value(hOldCurveId);
    int  hLastSep    = hNewCurveId.lastIndexOf("/");
    APreset->setValue(hNewCurveId.left(hLastSep)+"/CustomStores",
                      APreset->value(hNewCurveId.left(hLastSep)+"/CustomStores").toStringList() <<
                      hNewCurveId.mid(hLastSep+1));

    // channel mask (only exists for some)
    auto hOldMask = FCurveMap.key(hNewCurveId+"/Mask"); // get old key
    if (!hOldMask.isEmpty()) {
      APreset->setValue(hNewCurveId+"/Mask", ((APreset->value(hOldMask) == 0) ? 4 : 2));
      APreset->remove(hOldMask);
      AKeys->removeOne(hOldMask);
    }

    // curve type
    int hOldCurveType = APreset->value(hOldCurveId).toInt();
    APreset->setValue(hNewCurveId+"/CurveType", (hOldCurveType <= 1) ? 1 : 0);
    APreset->remove(hOldCurveId);
    AKeys->removeOne(hOldCurveId);

    if (hOldCurveType == 0) {   // curve is not active
      APreset->setValue(hNewCurveId+"/Anchor/size", 0);

    } else if (hOldCurveType >= 2) {   // curve from an old ptc curve file
      QString hFileName = "";
      QString hOldFNameKey = FCurveMap.key(hNewCurveId+"/FileName");
      auto hFList = APreset->value(hOldFNameKey).toStringList();
      if (hFList.size() > hOldCurveType-2)
        hFileName = hFList[hOldCurveType-2];
      APreset->setValue(hNewCurveId+"/FileName", hFileName);
      APreset->remove(hOldFNameKey);
      AKeys->removeOne(hOldFNameKey);

    } else if (hOldCurveType == 1) {    // manual curve
      // anchor count
      auto hAnchorCount = APreset->value(hOldCurveId+"Counter", 0).toInt();
      APreset->setValue(hNewCurveId+"/Anchor/size", hAnchorCount);
      APreset->remove(hOldCurveId+"Counter");
      AKeys->removeOne(hOldCurveId+"Counter");

      // translate anchor list
      for (int i = 0; i < hAnchorCount; ++i) {
        auto hOldXKey = QString("%1X%2").arg(hOldCurveId).arg(i);
        auto hOldYKey = QString("%1Y%2").arg(hOldCurveId).arg(i);
        APreset->setValue(QString("%1/Anchor/%2/X").arg(hNewCurveId).arg(i), APreset->value(hOldXKey));
        APreset->setValue(QString("%1/Anchor/%2/Y").arg(hNewCurveId).arg(i), APreset->value(hOldYKey));
        APreset->remove(hOldXKey);
        APreset->remove(hOldYKey);
        AKeys->removeOne(hOldXKey);
        AKeys->removeOne(hOldYKey);
      }
    }
  }
}

//==============================================================================

void ptFilterDM::TranslateCurvesToOld(QSettings *APreset, QStringList *AKeys) {
  auto hNewCurves = FCurveNameMap.values();
  for (QString hNewCurveId: hNewCurves) {
    if (!APreset->contains(hNewCurveId+"/CurveType"))
      continue;

    auto hOldCurveId = FCurveNameMap.key(hNewCurveId);

    // channel mask (only exists for some)
    auto hOldMask = FCurveMap.key(hNewCurveId+"/Mask"); // get old key
    if (!hOldMask.isEmpty())
      APreset->setValue(hOldMask, ((APreset->value(hNewCurveId+"/Mask").toInt() == 4) ? 0 : 1));

    int  hNewCurveType = APreset->value(hNewCurveId+"/CurveType").toInt();
    int  hAnchorCount  = APreset->value(hNewCurveId+"/Anchor/size").toInt();

    if (hNewCurveType == 0) {   // full precalc
      APreset->setValue(hOldCurveId, 2);
      APreset->setValue(FCurveMap.key(hNewCurveId+"/FileName"),
                        APreset->value(hNewCurveId+"/FileName").convert(QVariant::StringList));

    } else if (hNewCurveType == 1 && hAnchorCount == 0) {  // disabled
      APreset->setValue(hOldCurveId, 0);

    } else if (hNewCurveType == 1 && hAnchorCount > 0) {  // manual
      APreset->setValue(hOldCurveId, 1);
      APreset->setValue(hOldCurveId+"Counter", hAnchorCount);

      // translate anchor list
      for (int i = 0; i < hAnchorCount; ++i) {
        auto hNewXKey = QString("%1/Anchor/%2/X").arg(hNewCurveId).arg(i);
        auto hNewYKey = QString("%1/Anchor/%2/Y").arg(hNewCurveId).arg(i);
        APreset->setValue(QString("%1X%2").arg(hOldCurveId).arg(i), APreset->value(hNewXKey));
        APreset->setValue(QString("%1Y%2").arg(hOldCurveId).arg(i), APreset->value(hNewYKey));
        APreset->remove(hNewXKey);
        APreset->remove(hNewYKey);
        AKeys->removeOne(hNewXKey);
        AKeys->removeOne(hNewYKey);
      }
    }

    APreset->remove(hNewCurveId+"/CurveType");
    AKeys->removeOne(hNewCurveId+"/CurveType");

    APreset->remove(hNewCurveId+"/Mask");
    AKeys->removeOne(hNewCurveId+"/Mask");

    APreset->remove(hNewCurveId+"/FileName");
    AKeys->removeOne(hNewCurveId+"/FileName");

    APreset->remove(hNewCurveId+"/Anchor/size");
    AKeys->removeOne(hNewCurveId+"/Anchor/size");
  }
}

//==============================================================================

void ptFilterDM::TranslateSpecialToNew(QSettings *APreset, QStringList *AKeys) {
  AKeys->removeAll("Magic");

  /***** Channel mixer *****
    Filter had a combobox with fixed "None" and "Manual" entries. It got extended
    dynamically by the names of all manually loaded mixer files. For some reason
    this whole construction was saved to PTS in addition to the actual mix factors.
    Here we need to take care of obsolete keys ChannelMixer, ChannelMixerFileNames.
  */
  // "ChannelMixer" holds the combobox index. 0=None, 1=Manual, >1=File
  // If >1 read the filename list to fill the new-style mixer name entry.
  if (AKeys->contains("ChannelMixer")) {
    int chmixerMode = APreset->value("ChannelMixer").toInt();
    if (chmixerMode > 1) {
      const QStringList chmixerFiles = APreset->value("ChannelMixerFileNames").toStringList();
      const int chmixerIdx = chmixerMode - 2;

      if (chmixerIdx < chmixerFiles.size()) {
        APreset->setValue(
              "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/MixerName/MixerName",
              QFileInfo(chmixerFiles[chmixerIdx]).baseName());
      }
    }

    APreset->remove("ChannelMixer");
    APreset->remove("ChannelMixerFileNames");
    AKeys->removeAll("ChannelMixer");
    AKeys->removeAll("ChannelMixerFileNames");
  }


  /***** Exposure *****
    Integer values of the "Exposure mode" combobox have changed. Here’s the mapping:
    old name                    old value  new value  new name
    ptAutoExposureMode_Auto     0          1          TMode::Auto
    ptAutoExposureMode_Ufraw    1          2          TMode::LikeUfraw
    ptAutoExposureMode_Manual   2          0          TMode::Manual
    ptAutoExposureMode_Zero     3          -          removed, corresponds to "Manual, EV == 0"
  */
  if (AKeys->contains("AutoExposure")) {
    const QString newExpModeKey = FNameMap.value("AutoExposure");
    switch (APreset->value("AutoExposure").toInt()) {
      case 0:  APreset->setValue(newExpModeKey, 1); break;
      case 1:  APreset->setValue(newExpModeKey, 2); break;
      case 2:  // fall through
      case 3:  // fall through
      default: APreset->setValue(newExpModeKey, 0); break;
    }
    APreset->remove("AutoExposure");
    AKeys->removeAll("AutoExposure");
  }


  /***** Tone adjustment *****
    Filter unnecessarily had two off conditions: mask type "disabled" or amount 0.0.
    Removed the "disabled" mask type, i.e. old mask type values are off by +1.
  */
  auto hToneMaskKeys = QStringList("LABToneAdjust1MaskType") << "LABToneAdjust2MaskType";
  for (QString hOldMaskKey: hToneMaskKeys) {
    auto hComboIdx = APreset->value(hOldMaskKey, -9999).toInt();
    if (hComboIdx == -9999) {
      // key not present in settings object
      continue;
    }
    if (static_cast<TMaskType>(hComboIdx) == TMaskType::Disabled) {
      // Old filter was disabled via mask type. Ensure amount 0.0 to keep it disabled.
      hComboIdx = static_cast<int>(TMaskType::Shadows);
      auto hOldAmountKey = QString("LABToneAdjust%1Amount").arg(hOldMaskKey.at(13));
      APreset->setValue(FNameMap.value(hOldAmountKey), 0.0);
      APreset->remove(hOldAmountKey);
      AKeys->removeAll(hOldAmountKey);
    }
    APreset->setValue(FNameMap.value(hOldMaskKey), hComboIdx);
    APreset->remove(hOldMaskKey);
    AKeys->removeAll(hOldMaskKey);
  }
}

//==============================================================================

void ptFilterDM::TranslateSpecialToOld(QSettings *APreset, QStringList *AKeys) {
  /***** Channel mixer *****
    Write old-style active state and (empty) file name list. Corresponding new-style
    keys do not exist anymore. Note that the invalid QVariant is intentional. Older
    Photivo versions expect exactly that to represent an empty file name list.
  */
  if (AKeys->contains("ChannelMixerR2R")) {
    APreset->setValue(
          "ChannelMixer",
          static_cast<int>(GFilterDM->GetFilterFromName(Fuid::ChannelMixer_RGB)->isActive()));
    APreset->setValue("ChannelMixerFileNames", QVariant());
  }


  /***** Exposure *****
    Integer values of the "Exposure mode" combobox have changed. Here’s the mapping:
    old name                    old value  new value  new name
    ptAutoExposureMode_Auto     0          1          TMode::Auto
    ptAutoExposureMode_Ufraw    1          2          TMode::LikeUfraw
    ptAutoExposureMode_Manual   2          0          TMode::Manual
    ptAutoExposureMode_Zero     3          -          removed, corresponds to "Manual, EV == 0"
  */
  const QString expModeNewKey = FNameMap.value("AutoExposure");
  if (AKeys->contains(expModeNewKey)) {
    switch (APreset->value(expModeNewKey).toInt()) {
      case 1:  APreset->setValue(expModeNewKey, 0); break;
      case 2:  APreset->setValue(expModeNewKey, 1); break;
      case 0:  // fall through
      default: APreset->setValue(expModeNewKey, 2); break;
    }
  }
}

//==============================================================================

void ptFilterDM::FillNameMap() {
  FCurveNameMap.insert("CurveRGB",               "RgbCurve/"+Fuid::RgbCurve_RGB+"/Curve");
  FCurveNameMap.insert("CurveShadowsHighlights", "ShadowsHighlights/"+Fuid::ShadowsHighlights_LabCC+"/Curve");
  FCurveNameMap.insert("CurveTexture",           "TextureCurve/"+Fuid::TextureCurve_LabCC+"/Curve");
  FCurveNameMap.insert("CurveDenoise2",          "LumaDenoiseCurve/"+Fuid::LumaDenoiseCurve_LabSN+"/Curve");
  FCurveNameMap.insert("CurveDenoise",           "DetailCurve/"+Fuid::DetailCurve_LabSN+"/Curve");
  FCurveNameMap.insert("CurveOutline",           "Outline/"+Fuid::Outline_LabEyeCandy+"/Curve");
  FCurveNameMap.insert("CurveLByHue",            "LumaByHueCurve/"+Fuid::LumaByHueCurve_LabEyeCandy+"/Curve");
  FCurveNameMap.insert("CurveSaturation",        "SatCurve/"+Fuid::SatCurve_LabEyeCandy+"/Curve");
  FCurveNameMap.insert("CurveHue",               "HueCurve/"+Fuid::HueCurve_LabEyeCandy+"/Curve");
  FCurveNameMap.insert("CurveL",                 "LCurve/"+Fuid::LCurve_LabEyeCandy+"/Curve");
  FCurveNameMap.insert("CurveLa",                "ABCurves/"+Fuid::ABCurves_LabEyeCandy+"/ACurve");
  FCurveNameMap.insert("CurveLb",                "ABCurves/"+Fuid::ABCurves_LabEyeCandy+"/BCurve");
  FCurveNameMap.insert("CurveR",                 "RToneCurve/"+Fuid::RTone_EyeCandy+"/Curve");
  FCurveNameMap.insert("CurveG",                 "GToneCurve/"+Fuid::GTone_EyeCandy+"/Curve");
  FCurveNameMap.insert("CurveB",                 "BToneCurve/"+Fuid::BTone_EyeCandy+"/Curve");
  FCurveNameMap.insert("BaseCurve",              "RgbCurve/"+Fuid::RgbCurve_Out+"/Curve");
  FCurveNameMap.insert("BaseCurve2",             "AfterGammaCurve/"+Fuid::AfterGammaCurve_Out+"/Curve");

  FCurveMap.insert("CurveFileNamesRGB",               "RgbCurve/"+Fuid::RgbCurve_RGB+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesShadowsHighlights", "ShadowsHighlights/"+Fuid::ShadowsHighlights_LabCC+"/Curve/FileName");
  FCurveMap.insert("TextureCurveType",                "TextureCurve/"+Fuid::TextureCurve_LabCC+"/Curve/Mask");
  FCurveMap.insert("CurveFileNamesTexture",           "TextureCurve/"+Fuid::TextureCurve_LabCC+"/Curve/FileName");
  FCurveMap.insert("Denoise2CurveType",               "LumaDenoiseCurve/"+Fuid::LumaDenoiseCurve_LabSN+"/Curve/Mask");
  FCurveMap.insert("CurveFileNamesDenoise2",          "LumaDenoiseCurve/"+Fuid::LumaDenoiseCurve_LabSN+"/Curve/FileName");
  FCurveMap.insert("DenoiseCurveType",                "DetailCurve/"+Fuid::DetailCurve_LabSN+"/Curve/Mask");
  FCurveMap.insert("CurveFileNamesDenoise",           "DetailCurve/"+Fuid::DetailCurve_LabSN+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesOutline",           "Outline/"+Fuid::Outline_LabEyeCandy+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesLByHue",            "LumaByHueCurve/"+Fuid::LumaByHueCurve_LabEyeCandy+"/Curve/FileName");
  FCurveMap.insert("SatCurveType",                    "SatCurve/"+Fuid::SatCurve_LabEyeCandy+"/Curve/Mask");
  FCurveMap.insert("CurveFileNamesSaturation",        "SatCurve/"+Fuid::SatCurve_LabEyeCandy+"/Curve/FileName");
  FCurveMap.insert("HueCurveType",                    "HueCurve/"+Fuid::HueCurve_LabEyeCandy+"/Curve/Mask");
  FCurveMap.insert("CurveFileNamesHue",               "HueCurve/"+Fuid::HueCurve_LabEyeCandy+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesL",                 "LCurve/"+Fuid::LCurve_LabEyeCandy+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesLa",                "ABCurves/"+Fuid::ABCurves_LabEyeCandy+"/ACurve/FileName");
  FCurveMap.insert("CurveFileNamesLb",                "ABCurves/"+Fuid::ABCurves_LabEyeCandy+"/BCurve/FileName");
  FCurveMap.insert("CurveFileNamesR",                 "RToneCurve/"+Fuid::RTone_EyeCandy+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesG",                 "GToneCurve/"+Fuid::GTone_EyeCandy+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesB",                 "BToneCurve/"+Fuid::BTone_EyeCandy+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesBase",              "RgbCurve/"+Fuid::RgbCurve_Out+"/Curve/FileName");
  FCurveMap.insert("CurveFileNamesBase2",             "AfterGammaCurve/"+Fuid::AfterGammaCurve_Out+"/Curve/FileName");

  FNameMap.reserve(600);

  // ptCurve related stuff that does not need special handling
  FNameMap.insert("CurveRGBType",                    "RgbCurve/"+Fuid::RgbCurve_RGB+"/Curve/Interpolation");
  FNameMap.insert("CurveShadowsHighlightsType",      "ShadowsHighlights/"+Fuid::ShadowsHighlights_LabCC+"/Curve/Interpolation");
  FNameMap.insert("CurveTextureType",                "TextureCurve/"+Fuid::TextureCurve_LabCC+"/Curve/Interpolation");
  FNameMap.insert("CurveDenoise2Type",               "LumaDenoiseCurve/"+Fuid::LumaDenoiseCurve_LabSN+"/Curve/Interpolation");
  FNameMap.insert("CurveDenoiseType",                "DetailCurve/"+Fuid::DetailCurve_LabSN+"/Curve/Interpolation");
  FNameMap.insert("CurveOutlineType",                "Outline/"+Fuid::Outline_LabEyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveLByHueType",                 "LumaByHueCurve/"+Fuid::LumaByHueCurve_LabEyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveSaturationType",             "SatCurve/"+Fuid::SatCurve_LabEyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveHueType",                    "HueCurve/"+Fuid::HueCurve_LabEyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveLType",                      "LCurve/"+Fuid::LCurve_LabEyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveLaType",                     "ABCurves/"+Fuid::ABCurves_LabEyeCandy+"/ACurve/Interpolation");
  FNameMap.insert("CurveLbType",                     "ABCurves/"+Fuid::ABCurves_LabEyeCandy+"/BCurve/Interpolation");
  FNameMap.insert("CurveRType",                      "RToneCurve/"+Fuid::RTone_EyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveGType",                      "GToneCurve/"+Fuid::GTone_EyeCandy+"/Curve/Interpolation");
  FNameMap.insert("CurveBType",                      "BToneCurve/"+Fuid::BTone_EyeCandy+"/Curve/Interpolation");
  FNameMap.insert("BaseCurveType",                   "RgbCurve/"+Fuid::RgbCurve_Out+"/Curve/Interpolation");
  FNameMap.insert("BaseCurve2Type",                  "AfterGammaCurve/"+Fuid::AfterGammaCurve_Out+"/Curve/Interpolation");

  // NOTE: List is a simple dump from ptSettings ctor. May contain nuts - aka superfluous lines. :)
  // Order is generally the same as in ptSettings ctor.

  // NOTE: Keep the lines of still unported filters commented. Otherwise the conversion functions
  // will happily convert unported old-style settings names to empty strings, breaking
  // settings file reading.

  // former spinbox/slider entries
//  FNameMap.insert("FileMgrThumbnailSize",            "");
//  FNameMap.insert("FileMgrThumbnailPadding",         "");
//  FNameMap.insert("FileMgrThumbMaxRowCol",           "");
//  FNameMap.insert("FileMgrThumbSaveSize",            "");
//  FNameMap.insert("MemoryTest",                      "");
//  FNameMap.insert("TabStatusIndicator",              "");
//  FNameMap.insert("SliderWidth",                     "");
//  FNameMap.insert("Zoom",                            "");
//  FNameMap.insert("ColorTemperature",                "");
//  FNameMap.insert("GreenIntensity",                  "");
//  FNameMap.insert("RMultiplier",                     "");
//  FNameMap.insert("GMultiplier",                     "");
//  FNameMap.insert("BMultiplier",                     "");
//  FNameMap.insert("BlackPoint",                      "");
//  FNameMap.insert("WhitePoint",                      "");
//  FNameMap.insert("CaRed",                           "");
//  FNameMap.insert("CaBlue",                          "");
//  FNameMap.insert("GreenEquil",                      "");
//  FNameMap.insert("CfaLineDenoise",                  "");
//  FNameMap.insert("AdjustMaximumThreshold",          "");
//  FNameMap.insert("RawDenoiseThreshold",             "");
//  FNameMap.insert("HotpixelReduction",               "");
//  FNameMap.insert("InterpolationPasses",             "");
//  FNameMap.insert("MedianPasses",                    "");
//  FNameMap.insert("ESMedianPasses",                  "");
//  FNameMap.insert("ClipParameter",                   "");
//  FNameMap.insert("LfunFocal",                       "");
//  FNameMap.insert("LfunAperture",                    "");
//  FNameMap.insert("LfunDistance",                    "");
//  FNameMap.insert("LfunScale",                       "");
//  FNameMap.insert("LfunCALinearKr",                  "");
//  FNameMap.insert("LfunCALinearKb",                  "");
//  FNameMap.insert("LfunCAPoly3Vr",                   "");
//  FNameMap.insert("LfunCAPoly3Vb",                   "");
//  FNameMap.insert("LfunCAPoly3Cr",                   "");
//  FNameMap.insert("LfunCAPoly3Cb",                   "");
//  FNameMap.insert("LfunCAPoly3Br",                   "");
//  FNameMap.insert("LfunCAPoly3Bb",                   "");
//  FNameMap.insert("LfunVignettePoly6K1",             "");
//  FNameMap.insert("LfunVignettePoly6K2",             "");
//  FNameMap.insert("LfunVignettePoly6K3",             "");
//  FNameMap.insert("LfunDistPoly3K1",                 "");
//  FNameMap.insert("LfunDistPoly5K1",                 "");
//  FNameMap.insert("LfunDistPoly5K2",                 "");
//  FNameMap.insert("LfunDistFov1Omega",               "");
//  FNameMap.insert("LfunDistPTLensA",                 "");
//  FNameMap.insert("LfunDistPTLensB",                 "");
//  FNameMap.insert("LfunDistPTLensC",                 "");
//  FNameMap.insert("DefishFocalLength",               "");
//  FNameMap.insert("DefishScale",                     "");
//  FNameMap.insert("Rotate",                          "");
//  FNameMap.insert("PerspectiveFocalLength",          "");
//  FNameMap.insert("PerspectiveTilt",                 "");
//  FNameMap.insert("PerspectiveTurn",                 "");
//  FNameMap.insert("PerspectiveScaleX",               "");
//  FNameMap.insert("PerspectiveScaleY",               "");
//  FNameMap.insert("GridX",                           "");
//  FNameMap.insert("GridY",                           "");
//  FNameMap.insert("CropExposure",                    "");
//  FNameMap.insert("LqrHorScale",                     "");
//  FNameMap.insert("LqrVertScale",                    "");
//  FNameMap.insert("LqrWidth",                        "");
//  FNameMap.insert("LqrHeight",                       "");
//  FNameMap.insert("ResizeScale",                     "");
  FNameMap.insert("LevelsBlackPoint",                "LevelsRgb/"+Fuid::Levels_RGB+"/Blackpoint");
  FNameMap.insert("LevelsWhitePoint",                "LevelsRgb/"+Fuid::Levels_RGB+"/Whitepoint");
  FNameMap.insert("LabLevelsBlackPoint",             "LevelsLab/"+Fuid::Levels_LabCC+"/Blackpoint");
  FNameMap.insert("LabLevelsWhitePoint",             "LevelsLab/"+Fuid::Levels_LabCC+"/Whitepoint");
  FNameMap.insert("ChannelMixerR2R",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Red2Red");
  FNameMap.insert("ChannelMixerG2R",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Green2Red");
  FNameMap.insert("ChannelMixerB2R",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Blue2Red");
  FNameMap.insert("ChannelMixerR2G",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Red2Green");
  FNameMap.insert("ChannelMixerG2G",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Green2Green");
  FNameMap.insert("ChannelMixerB2G",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Blue2Green");
  FNameMap.insert("ChannelMixerR2B",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Red2Blue");
  FNameMap.insert("ChannelMixerG2B",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Green2Blue");
  FNameMap.insert("ChannelMixerB2B",                 "ChannelMixer/"+Fuid::ChannelMixer_RGB+"/Blue2Blue");
  FNameMap.insert("Vibrance",                        "ColorIntensity/"+Fuid::ColorIntensity_RGB+"/Vibrance");
  FNameMap.insert("IntensityRed",                    "ColorIntensity/"+Fuid::ColorIntensity_RGB+"/Red");
  FNameMap.insert("IntensityGreen",                  "ColorIntensity/"+Fuid::ColorIntensity_RGB+"/Green");
  FNameMap.insert("IntensityBlue",                   "ColorIntensity/"+Fuid::ColorIntensity_RGB+"/Blue");
  FNameMap.insert("ColorEnhanceShadows",             "ColorEnhancement/"+Fuid::ColorEnhancement_RGB+"/Shadows");
  FNameMap.insert("ColorEnhanceHighlights",          "ColorEnhancement/"+Fuid::ColorEnhancement_RGB+"/Highlights");
  FNameMap.insert("HighlightsR",                     "Highlights/"+Fuid::Highlights_RGB+"/HighlightsR");
  FNameMap.insert("HighlightsG",                     "Highlights/"+Fuid::Highlights_RGB+"/HighlightsG");
  FNameMap.insert("HighlightsB",                     "Highlights/"+Fuid::Highlights_RGB+"/HighlightsB");
  FNameMap.insert("WhiteFraction",                   "ExposureCorrection/"+Fuid::Exposure_RGB+"/WhiteFraction");
  FNameMap.insert("WhiteLevel",                      "ExposureCorrection/"+Fuid::Exposure_RGB+"/WhiteLevel");
  FNameMap.insert("Exposure",                        "ExposureCorrection/"+Fuid::Exposure_RGB+"/EVDelta");
  FNameMap.insert("Reinhard05Brightness",            "ReinhardBrighten/"+Fuid::ReinhardBrighten_RGB+"/Brightness");
  FNameMap.insert("Reinhard05Chroma",                "ReinhardBrighten/"+Fuid::ReinhardBrighten_RGB+"/Chroma");
  FNameMap.insert("Reinhard05Light",                 "ReinhardBrighten/"+Fuid::ReinhardBrighten_RGB+"/LightTweak");
  FNameMap.insert("CatchWhite",                      "Brightness/"+Fuid::Brightness_RGB+"/CatchWhite");
  FNameMap.insert("CatchBlack",                      "Brightness/"+Fuid::Brightness_RGB+"/CatchBlack");
  FNameMap.insert("ExposureGain",                    "Brightness/"+Fuid::Brightness_RGB+"/Gain");
  FNameMap.insert("LMHLightRecovery1Amount",         "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/Strength1");
  FNameMap.insert("LMHLightRecovery1LowerLimit",     "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/LowerLimit1");
  FNameMap.insert("LMHLightRecovery1UpperLimit",     "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/UpperLimit1");
  FNameMap.insert("LMHLightRecovery1Softness",       "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/Softness1");
  FNameMap.insert("LMHLightRecovery2Amount",         "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/Strength2");
  FNameMap.insert("LMHLightRecovery2LowerLimit",     "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/LowerLimit2");
  FNameMap.insert("LMHLightRecovery2UpperLimit",     "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/UpperLimit2");
  FNameMap.insert("LMHLightRecovery2Softness",       "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/Softness2");
  FNameMap.insert("RGBTextureContrastAmount",        "TextureContrastRgb/"+Fuid::TextureContrast_RGB+"/Strength");
  FNameMap.insert("RGBTextureContrastThreshold",     "TextureContrastRgb/"+Fuid::TextureContrast_RGB+"/Threshold");
  FNameMap.insert("RGBTextureContrastSoftness",      "TextureContrastRgb/"+Fuid::TextureContrast_RGB+"/Softness");
  FNameMap.insert("RGBTextureContrastOpacity",       "TextureContrastRgb/"+Fuid::TextureContrast_RGB+"/Opacity");
  FNameMap.insert("RGBTextureContrastEdgeControl",   "TextureContrastRgb/"+Fuid::TextureContrast_RGB+"/EdgeControl");
  FNameMap.insert("RGBTextureContrastMasking",       "TextureContrastRgb/"+Fuid::TextureContrast_RGB+"/Masking");
  FNameMap.insert("Microcontrast1Radius",            "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/Radius");
  FNameMap.insert("Microcontrast1Amount",            "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/Strength");
  FNameMap.insert("Microcontrast1Opacity",           "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/Opacity");
  FNameMap.insert("Microcontrast1HaloControl",       "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/HaloControl");
  FNameMap.insert("Microcontrast1LowerLimit",        "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/LowerLimit");
  FNameMap.insert("Microcontrast1UpperLimit",        "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/UpperLimit");
  FNameMap.insert("Microcontrast1Softness",          "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/Softness");
  FNameMap.insert("Microcontrast2Radius",            "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/Radius");
  FNameMap.insert("Microcontrast2Amount",            "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/Strength");
  FNameMap.insert("Microcontrast2Opacity",           "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/Opacity");
  FNameMap.insert("Microcontrast2HaloControl",       "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/HaloControl");
  FNameMap.insert("Microcontrast2LowerLimit",        "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/LowerLimit");
  FNameMap.insert("Microcontrast2UpperLimit",        "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/UpperLimit");
  FNameMap.insert("Microcontrast2Softness",          "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/Softness");
  FNameMap.insert("ShadowsHighlightsFine",           "ShadowsHighlights/"+Fuid::ShadowsHighlights_LabCC+"/FineDetail");
  FNameMap.insert("ShadowsHighlightsCoarse",         "ShadowsHighlights/"+Fuid::ShadowsHighlights_LabCC+"/CoarseDetail");
  FNameMap.insert("ShadowsHighlightsRadius",         "ShadowsHighlights/"+Fuid::ShadowsHighlights_LabCC+"/Scale");
  FNameMap.insert("DRCBeta",                         "Drc/"+Fuid::Drc_LabCC+"/Strength");
  FNameMap.insert("DRCAlpha",                        "Drc/"+Fuid::Drc_LabCC+"/Bias");
  FNameMap.insert("DRCColor",                        "Drc/"+Fuid::Drc_LabCC+"/ColorAdapt");
  FNameMap.insert("LabLMHLightRecovery1Amount",      "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/Strength1");
  FNameMap.insert("LabLMHLightRecovery1LowerLimit",  "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/LowerLimit1");
  FNameMap.insert("LabLMHLightRecovery1UpperLimit",  "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/UpperLimit1");
  FNameMap.insert("LabLMHLightRecovery1Softness",    "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/Softness1");
  FNameMap.insert("LabLMHLightRecovery2Amount",      "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/Strength2");
  FNameMap.insert("LabLMHLightRecovery2LowerLimit",  "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/LowerLimit2");
  FNameMap.insert("LabLMHLightRecovery2UpperLimit",  "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/UpperLimit2");
  FNameMap.insert("LabLMHLightRecovery2Softness",    "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/Softness2");
  FNameMap.insert("TextureContrast1Amount",          "TextureContrastLab/"+Fuid::TextureContrast1_LabCC+"/Strength");
  FNameMap.insert("TextureContrast1Threshold",       "TextureContrastLab/"+Fuid::TextureContrast1_LabCC+"/Threshold");
  FNameMap.insert("TextureContrast1Softness",        "TextureContrastLab/"+Fuid::TextureContrast1_LabCC+"/Softness");
  FNameMap.insert("TextureContrast1Opacity",         "TextureContrastLab/"+Fuid::TextureContrast1_LabCC+"/Opacity");
  FNameMap.insert("TextureContrast1EdgeControl",     "TextureContrastLab/"+Fuid::TextureContrast1_LabCC+"/EdgeControl");
  FNameMap.insert("TextureContrast1Masking",         "TextureContrastLab/"+Fuid::TextureContrast1_LabCC+"/Masking");
  FNameMap.insert("TextureContrast2Amount",          "TextureContrastLab/"+Fuid::TextureContrast2_LabCC+"/Strength");
  FNameMap.insert("TextureContrast2Threshold",       "TextureContrastLab/"+Fuid::TextureContrast2_LabCC+"/Threshold");
  FNameMap.insert("TextureContrast2Softness",        "TextureContrastLab/"+Fuid::TextureContrast2_LabCC+"/Softness");
  FNameMap.insert("TextureContrast2Opacity",         "TextureContrastLab/"+Fuid::TextureContrast2_LabCC+"/Opacity");
  FNameMap.insert("TextureContrast2EdgeControl",     "TextureContrastLab/"+Fuid::TextureContrast2_LabCC+"/EdgeControl");
  FNameMap.insert("TextureContrast2Masking",         "TextureContrastLab/"+Fuid::TextureContrast2_LabCC+"/Masking");
  FNameMap.insert("LabMicrocontrast1Radius",         "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/Radius");
  FNameMap.insert("LabMicrocontrast1Amount",         "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/Strength");
  FNameMap.insert("LabMicrocontrast1Opacity",        "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/Opacity");
  FNameMap.insert("LabMicrocontrast1HaloControl",    "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/HaloControl");
  FNameMap.insert("LabMicrocontrast1LowerLimit",     "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/LowerLimit");
  FNameMap.insert("LabMicrocontrast1UpperLimit",     "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/UpperLimit");
  FNameMap.insert("LabMicrocontrast1Softness",       "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/Softness");
  FNameMap.insert("LabMicrocontrast2Radius",         "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/Radius");
  FNameMap.insert("LabMicrocontrast2Amount",         "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/Strength");
  FNameMap.insert("LabMicrocontrast2Opacity",        "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/Opacity");
  FNameMap.insert("LabMicrocontrast2HaloControl",    "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/HaloControl");
  FNameMap.insert("LabMicrocontrast2LowerLimit",     "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/LowerLimit");
  FNameMap.insert("LabMicrocontrast2UpperLimit",     "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/UpperLimit");
  FNameMap.insert("LabMicrocontrast2Softness",       "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/Softness");
  FNameMap.insert("LC1Radius",                       "LocalContrastStretch/"+Fuid::LocalContrastStretch1_LabCC+"/Radius");
  FNameMap.insert("LC1Feather",                      "LocalContrastStretch/"+Fuid::LocalContrastStretch1_LabCC+"/Feather");
  FNameMap.insert("LC1Opacity",                      "LocalContrastStretch/"+Fuid::LocalContrastStretch1_LabCC+"/Opacity");
  FNameMap.insert("LC1m",                            "LocalContrastStretch/"+Fuid::LocalContrastStretch1_LabCC+"/Masking");
  FNameMap.insert("LC2Radius",                       "LocalContrastStretch/"+Fuid::LocalContrastStretch2_LabCC+"/Radius");
  FNameMap.insert("LC2Feather",                      "LocalContrastStretch/"+Fuid::LocalContrastStretch2_LabCC+"/Feather");
  FNameMap.insert("LC2Opacity",                      "LocalContrastStretch/"+Fuid::LocalContrastStretch2_LabCC+"/Opacity");
  FNameMap.insert("LC2m",                            "LocalContrastStretch/"+Fuid::LocalContrastStretch2_LabCC+"/Masking");
  FNameMap.insert("ColorcontrastOpacity",            "ColorContrast/"+Fuid::ColorContrast_LabEyeCandy+"/Opacity");
  FNameMap.insert("ColorcontrastRadius",             "ColorContrast/"+Fuid::ColorContrast_LabEyeCandy+"/Radius");
  FNameMap.insert("ColorcontrastAmount",             "ColorContrast/"+Fuid::ColorContrast_LabEyeCandy+"/Strength");
  FNameMap.insert("ColorcontrastHaloControl",        "ColorContrast/"+Fuid::ColorContrast_LabEyeCandy+"/HaloControl");
  FNameMap.insert("RGBGammaAmount",                  "GammaTool/"+Fuid::GammaTool_RGB+"/Gamma");
  FNameMap.insert("RGBGammaLinearity",               "GammaTool/"+Fuid::GammaTool_RGB+"/Linearity");
  FNameMap.insert("NormalizationOpacity",            "Normalization/"+Fuid::Normalization_RGB+"/Opacity");
  FNameMap.insert("RGBContrastAmount",               "SigContrastRgb/"+Fuid::SigContrastRgb_RGB+"/Strength");
  FNameMap.insert("RGBContrastThreshold",            "SigContrastRgb/"+Fuid::SigContrastRgb_RGB+"/Threshold");
  FNameMap.insert("ContrastAmount",                  "SigContrastLab/"+Fuid::SigContrastLab_LabCC+"/Strength");
  FNameMap.insert("ContrastThreshold",               "SigContrastLab/"+Fuid::SigContrastLab_LabCC+"/Threshold");
  FNameMap.insert("SaturationAmount",                "Saturation/"+Fuid::Saturation_LabCC+"/Strength");
  FNameMap.insert("ColorBoostValueA",                "ColorBoost/"+Fuid::ColorBoost_LabCC+"/StrengthA");
  FNameMap.insert("ColorBoostValueB",                "ColorBoost/"+Fuid::ColorBoost_LabCC+"/StrengthB");
  FNameMap.insert("ImpulseDenoiseThresholdL",        "ImpulseNR/"+Fuid::ImpulseNR_LabSN+"/ThresholdL");
  FNameMap.insert("ImpulseDenoiseThresholdAB",       "ImpulseNR/"+Fuid::ImpulseNR_LabSN+"/ThresholdAB");
  FNameMap.insert("EAWMaster",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Master");
  FNameMap.insert("EAWLevel1",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Level1");
  FNameMap.insert("EAWLevel2",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Level2");
  FNameMap.insert("EAWLevel3",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Level3");
  FNameMap.insert("EAWLevel4",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Level4");
  FNameMap.insert("EAWLevel5",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Level5");
  FNameMap.insert("EAWLevel6",                       "EAWavelets/"+Fuid::EAWavelets_LabSN+"/Level6");
  FNameMap.insert("GREYCLabOpacity",                 "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Opacity");
  FNameMap.insert("GREYCLabAmplitude",               "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Amplitude");
  FNameMap.insert("GREYCLabIterations",              "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Iterations");
  FNameMap.insert("GREYCLabSharpness",               "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Sharpness");
  FNameMap.insert("GREYCLabAnisotropy",              "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Anisotropy");
  FNameMap.insert("GREYCLabAlpha",                   "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/GradientSmooth");
  FNameMap.insert("GREYCLabSigma",                   "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/TensorSmooth");
  FNameMap.insert("GREYCLabdl",                      "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/SpacialPrec");
  FNameMap.insert("GREYCLabda",                      "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/AngularPrec");
  FNameMap.insert("GREYCLabGaussPrecision",          "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/ValuePrec");
  FNameMap.insert("DefringeRadius",                  "Defringe/"+Fuid::Defringe_LabSN+"/Radius");
  FNameMap.insert("DefringeThreshold",               "Defringe/"+Fuid::Defringe_LabSN+"/Threshold");
  FNameMap.insert("DefringeShift",                   "Defringe/"+Fuid::Defringe_LabSN+"/MaskTuning");
  FNameMap.insert("PyrDenoiseLAmount",               "PyramidDenoise/"+Fuid::PyramidDenoise_LabSN+"/LumaStrength");
  FNameMap.insert("PyrDenoiseABAmount",              "PyramidDenoise/"+Fuid::PyramidDenoise_LabSN+"/ChromaStrength");
  FNameMap.insert("PyrDenoiseGamma",                 "PyramidDenoise/"+Fuid::PyramidDenoise_LabSN+"/Gamma");
  FNameMap.insert("PyrDenoiseLevels",                "PyramidDenoise/"+Fuid::PyramidDenoise_LabSN+"/Levels");
  FNameMap.insert("BilateralLOpacity",               "LumaDenoise/"+Fuid::LumaDenoise_LabSN+"/Opacity");
  FNameMap.insert("BilateralLUseMask",               "LumaDenoise/"+Fuid::LumaDenoise_LabSN+"/EdgeThreshold");
  FNameMap.insert("BilateralLSigmaS",                "LumaDenoise/"+Fuid::LumaDenoise_LabSN+"/LScale");
  FNameMap.insert("BilateralLSigmaR",                "LumaDenoise/"+Fuid::LumaDenoise_LabSN+"/LStrength");
  FNameMap.insert("BilateralBSigmaR",                "ColorDenoise/"+Fuid::ColorDenoise_LabSN+"/AStrength");
  FNameMap.insert("BilateralASigmaS",                "ColorDenoise/"+Fuid::ColorDenoise_LabSN+"/AScale");
  FNameMap.insert("BilateralASigmaR",                "ColorDenoise/"+Fuid::ColorDenoise_LabSN+"/BStrength");
  FNameMap.insert("BilateralBSigmaS",                "ColorDenoise/"+Fuid::ColorDenoise_LabSN+"/BScale");
  FNameMap.insert("DenoiseCurveSigmaS",              "LumaDenoiseCurve/"+Fuid::LumaDenoiseCurve_LabSN+"/LScale");
  FNameMap.insert("DenoiseCurveSigmaR",              "LumaDenoiseCurve/"+Fuid::LumaDenoiseCurve_LabSN+"/LStrength");
  FNameMap.insert("WaveletDenoiseL",                 "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/LStrength");
  FNameMap.insert("WaveletDenoiseLSoftness",         "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/LSoftness");
  FNameMap.insert("WaveletDenoiseLSharpness",        "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/Sharpness");
  FNameMap.insert("WaveletDenoiseLAnisotropy",       "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/Anisotropy");
  FNameMap.insert("WaveletDenoiseLAlpha",            "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/GradientSmooth");
  FNameMap.insert("WaveletDenoiseLSigma",            "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/TensorSmooth");
  FNameMap.insert("WaveletDenoiseA",                 "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/AStrength");
  FNameMap.insert("WaveletDenoiseASoftness",         "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/ASoftness");
  FNameMap.insert("WaveletDenoiseB",                 "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/BStrength");
  FNameMap.insert("WaveletDenoiseBSoftness",         "WaveletDenoise/"+Fuid::WaveletDenoise_LabSN+"/BSoftness");
  FNameMap.insert("DetailCurveScaling",              "DetailCurve/"+Fuid::DetailCurve_LabSN+"/HaloControl");
  FNameMap.insert("DetailCurveWeight",               "DetailCurve/"+Fuid::DetailCurve_LabSN+"/Weight");
  FNameMap.insert("DetailCurveHotpixel",             "DetailCurve/"+Fuid::DetailCurve_LabSN+"/AntiBadpixel");
  FNameMap.insert("GradientSharpenPasses",           "GradientSharpen/"+Fuid::GradientSharpen_LabSN+"/Passes");
  FNameMap.insert("GradientSharpenStrength",         "GradientSharpen/"+Fuid::GradientSharpen_LabSN+"/Strength");
  FNameMap.insert("MLMicroContrastStrength",         "GradientSharpen/"+Fuid::GradientSharpen_LabSN+"/Microcontrast");
  FNameMap.insert("MLMicroContrastScaling",          "GradientSharpen/"+Fuid::GradientSharpen_LabSN+"/HaloControl");
  FNameMap.insert("MLMicroContrastWeight",           "GradientSharpen/"+Fuid::GradientSharpen_LabSN+"/Weight");
  FNameMap.insert("LabHotpixel",                     "GradientSharpen/"+Fuid::GradientSharpen_LabSN+"/Cleanup");
  FNameMap.insert("WienerFilterAmount",              "Wiener/"+Fuid::Wiener_LabSN+"/Strength");
  FNameMap.insert("WienerFilterGaussian",            "Wiener/"+Fuid::Wiener_LabSN+"/Gaussian");
  FNameMap.insert("WienerFilterBox",                 "Wiener/"+Fuid::Wiener_LabSN+"/Box");
  FNameMap.insert("WienerFilterLensBlur",            "Wiener/"+Fuid::Wiener_LabSN+"/LensBlur");
  FNameMap.insert("InverseDiffusionIterations",      "InvDiffSharpen/"+Fuid::InvDiffSharpen_LabSN+"/Iterations");
  FNameMap.insert("InverseDiffusionAmplitude",       "InvDiffSharpen/"+Fuid::InvDiffSharpen_LabSN+"/Amplitude");
  FNameMap.insert("USMRadius",                       "UnsharpMask/"+Fuid::Usm_LabSN+"/Radius");
  FNameMap.insert("USMAmount",                       "UnsharpMask/"+Fuid::Usm_LabSN+"/Strength");
  FNameMap.insert("USMThreshold",                    "UnsharpMask/"+Fuid::Usm_LabSN+"/Threshold");
  FNameMap.insert("HighpassAmount",                  "HighpassSharpen/"+Fuid::HighpassSharpen_LabSN+"/Strength");
  FNameMap.insert("HighpassRadius",                  "HighpassSharpen/"+Fuid::HighpassSharpen_LabSN+"/Radius");
  FNameMap.insert("HighpassDenoise",                 "HighpassSharpen/"+Fuid::HighpassSharpen_LabSN+"/Denoise");
  FNameMap.insert("Grain1Strength",                  "FilmGrain/"+Fuid::FilmGrain_LabSN+"/Strength1");
  FNameMap.insert("Grain1Radius",                    "FilmGrain/"+Fuid::FilmGrain_LabSN+"/Radius1");
  FNameMap.insert("Grain1Opacity",                   "FilmGrain/"+Fuid::FilmGrain_LabSN+"/Opacity1");
  FNameMap.insert("Grain1LowerLimit",                "FilmGrain/"+Fuid::FilmGrain_LabSN+"/LowerLimit1");
  FNameMap.insert("Grain1UpperLimit",                "FilmGrain/"+Fuid::FilmGrain_LabSN+"/UpperLimit1");
  FNameMap.insert("Grain2Strength",                  "FilmGrain/"+Fuid::FilmGrain_LabSN+"/Strength2");
  FNameMap.insert("Grain2Radius",                    "FilmGrain/"+Fuid::FilmGrain_LabSN+"/Radius2");
  FNameMap.insert("Grain2Opacity",                   "FilmGrain/"+Fuid::FilmGrain_LabSN+"/Opacity2");
  FNameMap.insert("Grain2LowerLimit",                "FilmGrain/"+Fuid::FilmGrain_LabSN+"/LowerLimit2");
  FNameMap.insert("Grain2UpperLimit",                "FilmGrain/"+Fuid::FilmGrain_LabSN+"/UpperLimit2");
  FNameMap.insert("OutlineWeight",                   "Outline/"+Fuid::Outline_LabEyeCandy+"/ColorWeight");
  FNameMap.insert("OutlineBlurRadius",               "Outline/"+Fuid::Outline_LabEyeCandy+"/BlurRadius");
  FNameMap.insert("LABToneAdjust1Saturation",        "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/Saturation");
  FNameMap.insert("LABToneAdjust1Amount",            "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/Strength");
  FNameMap.insert("LABToneAdjust1Hue",               "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/Hue");
  FNameMap.insert("LABToneAdjust1LowerLimit",        "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/LowerLimit");
  FNameMap.insert("LABToneAdjust1UpperLimit",        "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/UpperLimit");
  FNameMap.insert("LABToneAdjust1Softness",          "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/Softness");
  FNameMap.insert("LABToneAdjust2Saturation",        "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/Saturation");
  FNameMap.insert("LABToneAdjust2Amount",            "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/Strength");
  FNameMap.insert("LABToneAdjust2Hue",               "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/Hue");
  FNameMap.insert("LABToneAdjust2LowerLimit",        "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/LowerLimit");
  FNameMap.insert("LABToneAdjust2UpperLimit",        "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/UpperLimit");
  FNameMap.insert("LABToneAdjust2Softness",          "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/Softness");
  FNameMap.insert("LAdjustC1",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustRed");
  FNameMap.insert("LAdjustC2",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustOrange");
  FNameMap.insert("LAdjustC3",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustYellow");
  FNameMap.insert("LAdjustC4",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustLightGreen");
  FNameMap.insert("LAdjustC5",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustDarkGreen");
  FNameMap.insert("LAdjustC6",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustCyan");
  FNameMap.insert("LAdjustC7",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustBlue");
  FNameMap.insert("LAdjustC8",                       "LumaAdjust/"+Fuid::LumaAdjust_LabEyeCandy+"/AdjustMagenta");
  FNameMap.insert("LAdjustSC1",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustRed");
  FNameMap.insert("LAdjustSC2",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustOrange");
  FNameMap.insert("LAdjustSC3",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustYellow");
  FNameMap.insert("LAdjustSC4",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustLightGreen");
  FNameMap.insert("LAdjustSC5",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustDarkGreen");
  FNameMap.insert("LAdjustSC6",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustCyan");
  FNameMap.insert("LAdjustSC7",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustBlue");
  FNameMap.insert("LAdjustSC8",                      "SatAdjust/"+Fuid::SatAdjust_LabEyeCandy+"/AdjustMagenta");
  FNameMap.insert("LABToneSaturation",               "Tone/"+Fuid::Tone_LabEyeCandy+"/SaturationAll");
  FNameMap.insert("LABToneAmount",                   "Tone/"+Fuid::Tone_LabEyeCandy+"/StrengthAll");
  FNameMap.insert("LABToneHue",                      "Tone/"+Fuid::Tone_LabEyeCandy+"/HueAll");
  FNameMap.insert("LABToneSSaturation",              "Tone/"+Fuid::Tone_LabEyeCandy+"/SaturationShad");
  FNameMap.insert("LABToneSAmount",                  "Tone/"+Fuid::Tone_LabEyeCandy+"/StrengthShad");
  FNameMap.insert("LABToneSHue",                     "Tone/"+Fuid::Tone_LabEyeCandy+"/HueShad");
  FNameMap.insert("LABToneMSaturation",              "Tone/"+Fuid::Tone_LabEyeCandy+"/SaturationMid");
  FNameMap.insert("LABToneMAmount",                  "Tone/"+Fuid::Tone_LabEyeCandy+"/StrengthMid");
  FNameMap.insert("LABToneMHue",                     "Tone/"+Fuid::Tone_LabEyeCandy+"/HueMid");
  FNameMap.insert("LABToneHSaturation",              "Tone/"+Fuid::Tone_LabEyeCandy+"/SaturationLights");
  FNameMap.insert("LABToneHAmount",                  "Tone/"+Fuid::Tone_LabEyeCandy+"/StrengthLights");
  FNameMap.insert("LABToneHHue",                     "Tone/"+Fuid::Tone_LabEyeCandy+"/HueLights");
  FNameMap.insert("LabVignette",                     "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/Shape");
  FNameMap.insert("LabVignetteAmount",               "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/Strength");
  FNameMap.insert("LabVignetteInnerRadius",          "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/InnerRadius");
  FNameMap.insert("LabVignetteOuterRadius",          "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/OuterRadius");
  FNameMap.insert("LabVignetteRoundness",            "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/Roundness");
  FNameMap.insert("LabVignetteCenterX",              "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/CenterX");
  FNameMap.insert("LabVignetteCenterY",              "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/CenterY");
  FNameMap.insert("LabVignetteSoftness",             "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/Softness");
  FNameMap.insert("BWStylerOpacity",                 "BlackWhite/"+Fuid::BlackWhite_EyeCandy+"/Opacity");
  FNameMap.insert("BWStylerMultR",                   "BlackWhite/"+Fuid::BlackWhite_EyeCandy+"/MultRed");
  FNameMap.insert("BWStylerMultG",                   "BlackWhite/"+Fuid::BlackWhite_EyeCandy+"/MultGreen");
  FNameMap.insert("BWStylerMultB",                   "BlackWhite/"+Fuid::BlackWhite_EyeCandy+"/MultBlue");
  FNameMap.insert("SimpleToneR",                     "SimpleTone/"+Fuid::SimpleTone_EyeCandy+"/ChannelR");
  FNameMap.insert("SimpleToneG",                     "SimpleTone/"+Fuid::SimpleTone_EyeCandy+"/ChannelG");
  FNameMap.insert("SimpleToneB",                     "SimpleTone/"+Fuid::SimpleTone_EyeCandy+"/ChannelB");
//  FNameMap.insert("Tone1Amount",                     "");
//  FNameMap.insert("Tone1LowerLimit",                 "");
//  FNameMap.insert("Tone1UpperLimit",                 "");
//  FNameMap.insert("Tone1Softness",                   "");
//  FNameMap.insert("Tone2Amount",                     "");
//  FNameMap.insert("Tone2LowerLimit",                 "");
//  FNameMap.insert("Tone2UpperLimit",                 "");
//  FNameMap.insert("Tone2Softness",                   "");
  FNameMap.insert("CrossprocessingColor1",           "CrossProcessing/"+Fuid::CrossProcessing_EyeCandy+"/MainColor");
  FNameMap.insert("CrossprocessingColor2",           "CrossProcessing/"+Fuid::CrossProcessing_EyeCandy+"/SecondColor");
  FNameMap.insert("RGBContrast2Amount",              "SigContrastRgb/"+Fuid::SigContrastRgb_EyeCandy+"/Strength");
  FNameMap.insert("RGBContrast2Threshold",           "SigContrastRgb/"+Fuid::SigContrastRgb_EyeCandy+"/Threshold");
//  FNameMap.insert("TextureOverlayOpacity",           "");
//  FNameMap.insert("TextureOverlaySaturation",        "");
//  FNameMap.insert("TextureOverlayExponent",          "");
//  FNameMap.insert("TextureOverlayInnerRadius",       "");
//  FNameMap.insert("TextureOverlayOuterRadius",       "");
//  FNameMap.insert("TextureOverlayRoundness",         "");
//  FNameMap.insert("TextureOverlayCenterX",           "");
//  FNameMap.insert("TextureOverlayCenterY",           "");
//  FNameMap.insert("TextureOverlaySoftness",          "");
//  FNameMap.insert("TextureOverlay2Opacity",          "");
//  FNameMap.insert("TextureOverlay2Saturation",       "");
//  FNameMap.insert("TextureOverlay2Exponent",         "");
//  FNameMap.insert("TextureOverlay2InnerRadius",      "");
//  FNameMap.insert("TextureOverlay2OuterRadius",      "");
//  FNameMap.insert("TextureOverlay2Roundness",        "");
//  FNameMap.insert("TextureOverlay2CenterX",          "");
//  FNameMap.insert("TextureOverlay2CenterY",          "");
//  FNameMap.insert("TextureOverlay2Softness",         "");
//  FNameMap.insert("GradualOverlay1Amount",           "");
//  FNameMap.insert("GradualOverlay1Angle",            "");
//  FNameMap.insert("GradualOverlay1LowerLevel",       "");
//  FNameMap.insert("GradualOverlay1UpperLevel",       "");
//  FNameMap.insert("GradualOverlay1Softness",         "");
//  FNameMap.insert("GradualOverlay2Amount",           "");
//  FNameMap.insert("GradualOverlay2Angle",            "");
//  FNameMap.insert("GradualOverlay2LowerLevel",       "");
//  FNameMap.insert("GradualOverlay2UpperLevel",       "");
//  FNameMap.insert("GradualOverlay2Softness",         "");
  FNameMap.insert("Vignette",                        "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/Shape");
  FNameMap.insert("VignetteAmount",                  "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/Strength");
  FNameMap.insert("VignetteInnerRadius",             "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/InnerRadius");
  FNameMap.insert("VignetteOuterRadius",             "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/OuterRadius");
  FNameMap.insert("VignetteRoundness",               "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/Roundness");
  FNameMap.insert("VignetteCenterX",                 "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/CenterX");
  FNameMap.insert("VignetteCenterY",                 "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/CenterY");
  FNameMap.insert("VignetteSoftness",                "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/Softness");
  FNameMap.insert("GradBlur1Radius",                 "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/Radius");
  FNameMap.insert("GradBlur1LowerLevel",             "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/LowerLevel");
  FNameMap.insert("GradBlur1UpperLevel",             "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/UpperLevel");
  FNameMap.insert("GradBlur1Softness",               "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/Softness");
  FNameMap.insert("GradBlur1Angle",                  "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/Angle");
  FNameMap.insert("GradBlur1Vignette",               "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/Shape");
  FNameMap.insert("GradBlur1Roundness",              "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/Roundness");
  FNameMap.insert("GradBlur1CenterX",                "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/CenterX");
  FNameMap.insert("GradBlur1CenterY",                "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/CenterY");
  FNameMap.insert("GradBlur2Radius",                 "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/Radius");
  FNameMap.insert("GradBlur2LowerLevel",             "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/LowerLevel");
  FNameMap.insert("GradBlur2UpperLevel",             "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/UpperLevel");
  FNameMap.insert("GradBlur2Softness",               "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/Softness");
  FNameMap.insert("GradBlur2Angle",                  "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/Angle");
  FNameMap.insert("GradBlur2Vignette",               "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/Shape");
  FNameMap.insert("GradBlur2Roundness",              "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/Roundness");
  FNameMap.insert("GradBlur2CenterX",                "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/CenterX");
  FNameMap.insert("GradBlur2CenterY",                "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/CenterY");
  FNameMap.insert("SoftglowAmount",                  "SoftglowOrton/"+Fuid::SoftglowOrton_EyeCandy+"/Strength");
  FNameMap.insert("SoftglowRadius",                  "SoftglowOrton/"+Fuid::SoftglowOrton_EyeCandy+"/Radius");
  FNameMap.insert("SoftglowContrast",                "SoftglowOrton/"+Fuid::SoftglowOrton_EyeCandy+"/Contrast");
  FNameMap.insert("SoftglowSaturation",              "SoftglowOrton/"+Fuid::SoftglowOrton_EyeCandy+"/Saturation");
  FNameMap.insert("Vibrance2",                       "ColorIntensity/"+Fuid::ColorIntensity_EyeCandy+"/Vibrance");
  FNameMap.insert("Intensity2Red",                   "ColorIntensity/"+Fuid::ColorIntensity_EyeCandy+"/Red");
  FNameMap.insert("Intensity2Green",                 "ColorIntensity/"+Fuid::ColorIntensity_EyeCandy+"/Green");
  FNameMap.insert("Intensity2Blue",                  "ColorIntensity/"+Fuid::ColorIntensity_EyeCandy+"/Blue");
//  FNameMap.insert("OutputGamma",                     "");
//  FNameMap.insert("OutputLinearity",                 "");
  FNameMap.insert("RGBContrast3Amount",              "SigContrastRgb/"+Fuid::SigContrastRgb_Out+"/Strength");
  FNameMap.insert("RGBContrast3Threshold",           "SigContrastRgb/"+Fuid::SigContrastRgb_Out+"/Threshold");
//  FNameMap.insert("WebResizeScale",                  "");
  FNameMap.insert("WienerFilter2Amount",             "Wiener/"+Fuid::Wiener_Out+"/Strength");
  FNameMap.insert("WienerFilter2Gaussian",           "Wiener/"+Fuid::Wiener_Out+"/Gaussian");
  FNameMap.insert("WienerFilter2Box",                "Wiener/"+Fuid::Wiener_Out+"/Box");
  FNameMap.insert("WienerFilter2LensBlur",           "Wiener/"+Fuid::Wiener_Out+"/LensBlur");
//  FNameMap.insert("SaveQuality",                     "");
//  FNameMap.insert("SaveResolution",                  "");
//  FNameMap.insert("ImageRating",                     "");

  // former combobox entries
//  FNameMap.insert("BatchMgrAutosaveFile",            "");
//  FNameMap.insert("CameraColor",                     "");
//  FNameMap.insert("CameraColorProfileIntent",        "");
//  FNameMap.insert("CameraColorGamma",                "");
//  FNameMap.insert("WorkColor",                       "");
//  FNameMap.insert("CMQuality",                       "");
//  FNameMap.insert("PreviewColorProfileIntent",       "");
//  FNameMap.insert("OutputColorProfileIntent",        "");
//  FNameMap.insert("SaveButtonMode",                  "");
//  FNameMap.insert("ResetButtonMode",                 "");
//  FNameMap.insert("Style",                           "");
//  FNameMap.insert("StyleHighLight",                  "");
//  FNameMap.insert("StartupUIMode",                   "");
//  FNameMap.insert("PipeSize",                        "");
//  FNameMap.insert("StartupPipeSize",                 "");
//  FNameMap.insert("SpecialPreview",                  "");
//  FNameMap.insert("BadPixels",                       "");
//  FNameMap.insert("DarkFrame",                       "");
//  FNameMap.insert("WhiteBalance",                    "");
//  FNameMap.insert("CaCorrect",                       "");
//  FNameMap.insert("Interpolation",                   "");
//  FNameMap.insert("BayerDenoise",                    "");
//  FNameMap.insert("CropGuidelines",                  "");
//  FNameMap.insert("LightsOut",                       "");
//  FNameMap.insert("ClipMode",                        "");
//  FNameMap.insert("LfunCAModel",                     "");
//  FNameMap.insert("LfunVignetteModel",               "");
//  FNameMap.insert("LfunSrcGeo",                      "");
//  FNameMap.insert("LfunTargetGeo",                   "");
//  FNameMap.insert("LfunDistModel",                   "");
//  FNameMap.insert("LqrEnergy",                       "");
//  FNameMap.insert("LqrScaling",                      "");
//  FNameMap.insert("ResizeFilter",                    "");
//  FNameMap.insert("ResizeDimension",                 "");
//  FNameMap.insert("FlipMode",                        "");
//  FNameMap.insert("AspectRatioW",                    "");
//  FNameMap.insert("AspectRatioH",                    "");
  FNameMap.insert("ExposureClipMode",                "ExposureCorrection/"+Fuid::Exposure_RGB+"/ClipMode");
  FNameMap.insert("AutoExposure",                    "ExposureCorrection/"+Fuid::Exposure_RGB+"/Mode");
  FNameMap.insert("LABTransform",                    "LabTransform/"+Fuid::LabTransform_LabCC+"/Mode");
  FNameMap.insert("LMHLightRecovery1MaskType",       "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/MaskType1");
  FNameMap.insert("LMHLightRecovery2MaskType",       "LMHRecoveryRgb/"+Fuid::LMHRecovery_RGB+"/MaskType2");
  FNameMap.insert("Microcontrast1MaskType",          "LocalContrastRgb/"+Fuid::LocalContrast1_RGB+"/MaskType");
  FNameMap.insert("Microcontrast2MaskType",          "LocalContrastRgb/"+Fuid::LocalContrast2_RGB+"/MaskType");
  FNameMap.insert("LabLMHLightRecovery1MaskType",    "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/MaskType1");
  FNameMap.insert("LabLMHLightRecovery2MaskType",    "LMHRecoveryLab/"+Fuid::LMHRecovery_LabCC+"/MaskType2");
  FNameMap.insert("LabMicrocontrast1MaskType",       "LocalContrastLab/"+Fuid::LocalContrast1_LabCC+"/MaskType");
  FNameMap.insert("LabMicrocontrast2MaskType",       "LocalContrastLab/"+Fuid::LocalContrast2_LabCC+"/MaskType");
  FNameMap.insert("GREYCLab",                        "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Mode");
  FNameMap.insert("GREYCLabMaskType",                "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/MaskType");
  FNameMap.insert("GREYCLabInterpolation",           "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/Interpolation");
  FNameMap.insert("USM",                             "UnsharpMask/"+Fuid::Usm_LabSN+"/Mode");
  FNameMap.insert("Highpass",                        "HighpassSharpen/"+Fuid::HighpassSharpen_LabSN+"/Mode");
  FNameMap.insert("Grain1MaskType",                  "FilmGrain/"+Fuid::FilmGrain_LabSN+"/MaskType1");
  FNameMap.insert("Grain1Mode",                      "FilmGrain/"+Fuid::FilmGrain_LabSN+"/GrainType1");
  FNameMap.insert("Grain2MaskType",                  "FilmGrain/"+Fuid::FilmGrain_LabSN+"/MaskType2");
  FNameMap.insert("Grain2Mode",                      "FilmGrain/"+Fuid::FilmGrain_LabSN+"/GrainType2");
  FNameMap.insert("LabVignetteMode",                 "VignetteLab/"+Fuid::Vignette_LabEyeCandy+"/MaskType");
  FNameMap.insert("ViewLAB",                         "LabChannelView/"+Fuid::ViewLab_LabSN+"/ChannelSelection");
  FNameMap.insert("OutlineMode",                     "Outline/"+Fuid::Outline_LabEyeCandy+"/OverlayMode");
  FNameMap.insert("OutlineGradientMode",             "Outline/"+Fuid::Outline_LabEyeCandy+"/GradientMode");
  FNameMap.insert("LABToneAdjust1MaskType",          "ToneAdjust/"+Fuid::ToneAdjust1_LabEyeCandy+"/MaskMode");
  FNameMap.insert("LABToneAdjust2MaskType",          "ToneAdjust/"+Fuid::ToneAdjust2_LabEyeCandy+"/MaskMode");
  FNameMap.insert("BWStylerFilmType",                "BlackWhite/"+Fuid::BlackWhite_EyeCandy+"/FilmType");
  FNameMap.insert("BWStylerColorFilterType",         "BlackWhite/"+Fuid::BlackWhite_EyeCandy+"/ColorFilterType");
//  FNameMap.insert("Tone1MaskType",                   "");
//  FNameMap.insert("Tone2MaskType",                   "");
  FNameMap.insert("CrossprocessingMode",             "CrossProcessing/"+Fuid::CrossProcessing_EyeCandy+"/Mode");
//  FNameMap.insert("TextureOverlayMode",              "");
//  FNameMap.insert("TextureOverlayMask",              "");
//  FNameMap.insert("TextureOverlay2Mode",             "");
//  FNameMap.insert("TextureOverlay2Mask",             "");
//  FNameMap.insert("GradualOverlay1",                 "");
//  FNameMap.insert("GradualOverlay2",                 "");
  FNameMap.insert("VignetteMode",                    "VignetteRgb/"+Fuid::Vignette_EyeCandy+"/MaskType");
  FNameMap.insert("GradBlur1",                       "GradualBlur/"+Fuid::GradualBlur1_EyeCandy+"/Mode");
  FNameMap.insert("GradBlur2",                       "GradualBlur/"+Fuid::GradualBlur2_EyeCandy+"/Mode");
  FNameMap.insert("SoftglowMode",                    "SoftglowOrton/"+Fuid::SoftglowOrton_EyeCandy+"/Mode");
//  FNameMap.insert("WebResize",                       "");
//  FNameMap.insert("WebResizeDimension",              "");
//  FNameMap.insert("WebResizeFilter",                 "");
//  FNameMap.insert("SaveFormat",                      "");
//  FNameMap.insert("SaveSampling",                    "");
//  FNameMap.insert("OutputMode",                      "");
//  FNameMap.insert("CropInitialZoom",                 "");

    // former checkbox entries
//  FNameMap.insert("FileMgrUseThumbMaxRowCol",        "");
//  FNameMap.insert("BatchMgrAutosave",                "");
//  FNameMap.insert("BatchMgrAutoload",                "");
//  FNameMap.insert("StartupSettings",                 "");
//  FNameMap.insert("StartupSettingsReset",            "");
//  FNameMap.insert("StartupSwitchAR",                 "");
//  FNameMap.insert("InputsAddPowerLaw",               "");
//  FNameMap.insert("ExportToGimp",                    "");
//  FNameMap.insert("ToolBoxMode",                     "");
//  FNameMap.insert("PreviewTabMode",                  "");
//  FNameMap.insert("BackgroundColor",                 "");
//  FNameMap.insert("SearchBarEnable",                 "");
//  FNameMap.insert("WriteBackupSettings",             "");
//  FNameMap.insert("RunMode",                         "");
//  FNameMap.insert("MultiplierEnhance",               "");
//  FNameMap.insert("ManualBlackPoint",                "");
//  FNameMap.insert("ManualWhitePoint",                "");
//  FNameMap.insert("EeciRefine",                      "");
//  FNameMap.insert("LfunAutoScale",                   "");
//  FNameMap.insert("Defish",                          "");
//  FNameMap.insert("DefishAutoScale",                 "");
//  FNameMap.insert("Grid",                            "");
//  FNameMap.insert("Crop",                            "");
//  FNameMap.insert("FixedAspectRatio",                "");
//  FNameMap.insert("LqrVertFirst",                    "");
//  FNameMap.insert("Resize",                          "");
//  FNameMap.insert("AutomaticPipeSize",               "");
//  FNameMap.insert("GeometryBlock",                   "");
  FNameMap.insert("Reinhard05",                      "ReinhardBrighten/"+Fuid::ReinhardBrighten_RGB+"/Enabled");
  FNameMap.insert("GREYCLabFast",                    "GreyCStoration/"+Fuid::GreyCStoration_LabSN+"/FastGauss");
  FNameMap.insert("DefringeColor1",                  "Defringe/"+Fuid::Defringe_LabSN+"/ColorRed");
  FNameMap.insert("DefringeColor2",                  "Defringe/"+Fuid::Defringe_LabSN+"/ColorYellow");
  FNameMap.insert("DefringeColor3",                  "Defringe/"+Fuid::Defringe_LabSN+"/ColorGreen");
  FNameMap.insert("DefringeColor4",                  "Defringe/"+Fuid::Defringe_LabSN+"/ColorCyan");
  FNameMap.insert("DefringeColor5",                  "Defringe/"+Fuid::Defringe_LabSN+"/ColorBlue");
  FNameMap.insert("DefringeColor6",                  "Defringe/"+Fuid::Defringe_LabSN+"/ColorPurple");
  FNameMap.insert("WienerFilter",                    "Wiener/"+Fuid::Wiener_LabSN+"/Enabled");
  FNameMap.insert("WienerFilterUseEdgeMask",         "Wiener/"+Fuid::Wiener_LabSN+"/OnlyEdges");
  FNameMap.insert("InverseDiffusionUseEdgeMask",     "InvDiffSharpen/"+Fuid::InvDiffSharpen_LabSN+"/OnlyEdges");
  FNameMap.insert("OutlineSwitchLayer",              "Outline/"+Fuid::Outline_LabEyeCandy+"/ImageOnTop");
//  FNameMap.insert("WebResizeBeforeGamma",            "");
//  FNameMap.insert("OutputGammaCompensation",         "");
  FNameMap.insert("WienerFilter2",                   "Wiener/"+Fuid::Wiener_Out+"/Enabled");
  FNameMap.insert("WienerFilter2UseEdgeMask",        "Wiener/"+Fuid::Wiener_Out+"/OnlyEdges");
//  FNameMap.insert("IncludeExif",                     "");
//  FNameMap.insert("EraseExifThumbnail",              "");
//  FNameMap.insert("SaveConfirmation",                "");
//  FNameMap.insert("AutosaveSettings",                "");
//  FNameMap.insert("ResetSettingsConfirmation",       "");
//  FNameMap.insert("FullPipeConfirmation",            "");
//  FNameMap.insert("EscToExit",                       "");
//  FNameMap.insert("LoadTags",                       "");

    // former non-gui elements
//  FNameMap.insert("PipeIsRunning",                   "");
//  FNameMap.insert("BlockTools",                      "");
//  FNameMap.insert("InputPowerFactor",                "");
//  FNameMap.insert("PreviewMode",                     "");
//  FNameMap.insert("ZoomMode",                        "");
//  FNameMap.insert("Scaled",                          "");
//  FNameMap.insert("IsRAW",                           "");
//  FNameMap.insert("HaveImage",                       "");
//  FNameMap.insert("RawsDirectory",                   "");
//  FNameMap.insert("OutputDirectory",                 "");
//  FNameMap.insert("MainDirectory",                   "");
//  FNameMap.insert("ShareDirectory",                  "");
//  FNameMap.insert("UserDirectory",                   "");
//  FNameMap.insert("UIDirectory",                     "");
//  FNameMap.insert("TranslationsDirectory",           "");
//  FNameMap.insert("CurvesDirectory",                 "");
//  FNameMap.insert("PresetDirectory",                 "");
//  FNameMap.insert("CameraColorProfilesDirectory",    "");
//  FNameMap.insert("PreviewColorProfilesDirectory",   "");
//  FNameMap.insert("OutputColorProfilesDirectory",    "");
//  FNameMap.insert("StandardAdobeProfilesDirectory",  "");
//  FNameMap.insert("LensfunDatabaseDirectory",        "");
//  FNameMap.insert("PreviewColorProfile",             "");
//  FNameMap.insert("OutputColorProfile",              "");
//  FNameMap.insert("GimpExecCommand",                 "");
//  FNameMap.insert("StartupSettingsFile",             "");
//  FNameMap.insert("CameraMake",                      "");
//  FNameMap.insert("CameraModel",                     "");
//  FNameMap.insert("CameraColorProfile",              "");
//  FNameMap.insert("HaveBadPixels",                   "");
//  FNameMap.insert("BadPixelsFileName",               "");
//  FNameMap.insert("HaveDarkFrame",                   "");
//  FNameMap.insert("DarkFrameFileName",               "");
//  FNameMap.insert("VisualSelectionX",                "");
//  FNameMap.insert("VisualSelectionY",                "");
//  FNameMap.insert("VisualSelectionWidth",            "");
//  FNameMap.insert("VisualSelectionHeight",           "");
//  FNameMap.insert("ImageW",                          "");
//  FNameMap.insert("ImageH",                          "");
//  FNameMap.insert("PipeImageW",                      "");
//  FNameMap.insert("PipeImageH",                      "");
//  FNameMap.insert("CropX",                           "");
//  FNameMap.insert("CropY",                           "");
//  FNameMap.insert("CropW",                           "");
//  FNameMap.insert("CropH",                           "");
//  FNameMap.insert("RotateW",                         "");
//  FNameMap.insert("RotateH",                         "");
//  FNameMap.insert("OutputFileName",                  "");
//  FNameMap.insert("JobMode",                         "");
//  FNameMap.insert("InputFileNameList",               "");
//  FNameMap.insert("Tone1ColorRed",                   "");
//  FNameMap.insert("Tone1ColorGreen",                 "");
//  FNameMap.insert("Tone1ColorBlue",                  "");
//  FNameMap.insert("Tone2ColorRed",                   "");
//  FNameMap.insert("Tone2ColorGreen",                 "");
//  FNameMap.insert("Tone2ColorBlue",                  "");
//  FNameMap.insert("GradualOverlay1ColorRed",         "");
//  FNameMap.insert("GradualOverlay1ColorGreen",       "");
//  FNameMap.insert("GradualOverlay1ColorBlue",        "");
//  FNameMap.insert("GradualOverlay2ColorRed",         "");
//  FNameMap.insert("GradualOverlay2ColorGreen",       "");
//  FNameMap.insert("GradualOverlay2ColorBlue",        "");
//  FNameMap.insert("TextureOverlayFile",              "");
//  FNameMap.insert("TextureOverlay2File",             "");
//  FNameMap.insert("DigikamTagsList",                 "");
//  FNameMap.insert("TagsList",                        "");
//  FNameMap.insert("OutputFileNameSuffix",            "");
//  FNameMap.insert("ImageTitle",                      "");
//  FNameMap.insert("Copyright",                       "");
//  FNameMap.insert("BackgroundRed",                   "");
//  FNameMap.insert("BackgroundGreen",                 "");
//  FNameMap.insert("BackgroundBlue",                  "");
//  FNameMap.insert("HistogramChannel",                "");
//  FNameMap.insert("HistogramLogX",                   "");
//  FNameMap.insert("HistogramLogY",                   "");
//  FNameMap.insert("HistogramMode",                   "");
//  FNameMap.insert("HistogramCrop",                   "");
//  FNameMap.insert("HistogramCropX",                  "");
//  FNameMap.insert("HistogramCropY",                  "");
//  FNameMap.insert("HistogramCropW",                  "");
//  FNameMap.insert("HistogramCropH",                  "");
//  FNameMap.insert("PixelReader",                     "");
//  FNameMap.insert("ExposureIndicator",               "");
//  FNameMap.insert("ExposureIndicatorSensor",         "");
//  FNameMap.insert("ExposureIndicatorR",              "");
//  FNameMap.insert("ExposureIndicatorG",              "");
//  FNameMap.insert("ExposureIndicatorB",              "");
//  FNameMap.insert("ExposureIndicatorOver",           "");
//  FNameMap.insert("ExposureIndicatorUnder",          "");
//  FNameMap.insert("ShowExposureIndicatorSensor",     "");
//  FNameMap.insert("ShowBottomContainer",             "");
//  FNameMap.insert("ShowToolContainer",               "");
  FNameMap.insert("SatCurveMode",                    "SatCurve/"+Fuid::SatCurve_LabEyeCandy+"/Mode");
//  FNameMap.insert("FullOutput",                      "");
//  FNameMap.insert("HiddenTools",                     "");
//  FNameMap.insert("FavouriteTools",                  "");
//  FNameMap.insert("BlockedTools",                    "");
//  FNameMap.insert("DisabledTools",                   "");
//  FNameMap.insert("BlockUpdate",                     "");
//  FNameMap.insert("FocalLengthIn35mmFilm",           "");
//  FNameMap.insert("ApertureFromExif",                "");
//  FNameMap.insert("DetailViewActive",                "");
//  FNameMap.insert("DetailViewScale",                 "");
//  FNameMap.insert("DetailViewCropX",                 "");
//  FNameMap.insert("DetailViewCropY",                 "");
//  FNameMap.insert("DetailViewCropW",                 "");
//  FNameMap.insert("DetailViewCropH",                 "");
//  FNameMap.insert("TranslationMode",                 "");
//  FNameMap.insert("UiLanguage",                      "");
//  FNameMap.insert("CustomCSSFile",                   "");
//  FNameMap.insert("FullscreenActive",                "");
//  FNameMap.insert("FileMgrIsOpen",                   "");
//  FNameMap.insert("FileMgrStartupOpen",              "");
//  FNameMap.insert("LastFileMgrLocation",             "");
//  FNameMap.insert("FileMgrShowDirThumbs",            "");
//  FNameMap.insert("FileMgrShowImageView",            "");
//  FNameMap.insert("FileMgrShowSidebar",              "");
//  FNameMap.insert("FileMgrThumbLayoutType",          "");
//  FNameMap.insert("BatchIsOpen",                     "");
//  FNameMap.insert("BatchLogIsVisible",               "");
}


//==============================================================================
// iterator


ptFilterIterator::ptFilterIterator(TCacheGroupList *AList, const bool AIsEndMarker)
: FList(AList),
  FNode(nullptr),
  FNodeGroup(-1),
  FNodeIdx(-1),
  FBeyondEnd(AIsEndMarker)
{
  if (!FBeyondEnd) {
    FNodeGroup = ptFilterDM::FindBeginTab(*FList);
    FNodeIdx   = 0;
    FNode      = FList->at(FNodeGroup).at(0);
  }
}

//==============================================================================

bool ptFilterIterator::operator==(const ptFilterIterator &other) const {
  if (this->FBeyondEnd || other.FBeyondEnd) {
    return this->FBeyondEnd && other.FBeyondEnd;
  } else {
    return FNode == other.FNode;
  }
}

//==============================================================================

bool ptFilterIterator::operator!=(const ptFilterIterator &other) const {
  if (this->FBeyondEnd || other.FBeyondEnd) {
    return this->FBeyondEnd != other.FBeyondEnd;
  } else {
    return FNode != other.FNode;
  }
}

//==============================================================================

void ptFilterIterator::next() {
  if (FBeyondEnd) return;

  if(++FNodeIdx >= FList->at(FNodeGroup).size()) {
    do {
      if (++FNodeGroup >= FList->size()) {
        FBeyondEnd = true;
        return;
      }
    } while (FList->at(FNodeGroup).size() == 0);
    FNodeIdx = 0;
  }
  FNode = FList->at(FNodeGroup).at(FNodeIdx);
}

//==============================================================================

void ptFilterIterator::previous() {
  if (FBeyondEnd) {
    FNodeGroup = ptFilterDM::FindEndTab(*FList);
    FNodeIdx   = FList->at(FNodeGroup).size();
    FNode      = FList->at(FNodeGroup).at(FNodeIdx);
    FBeyondEnd = false;
    return;
  }

  if (--FNodeIdx < 0) {
    while (FList->at(--FNodeGroup).size() == 0);
    FNodeIdx = FList->at(FNodeGroup).size()-1;
  }
  FNode = FList->at(FNodeGroup).at(FNodeIdx);
}

//==============================================================================
