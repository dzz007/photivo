////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include <assert.h>

#include "ptSettings.h"
#include "ptError.h"
#include "ptRGBTemperature.h"
#include "ptGuiOptions.h"

#ifdef _
  #define ptRAW_OLD__ _
  #undef _
#endif
// Define a simple one character _ for translation.
#define _ QObject::tr

// Load in the gui input elements
const ptGuiInputItem GuiInputItems[] = {
#define DLRAW_GUI_INPUT_ITEM
#include "ptGuiItems.i"
#undef  DLRAW_GUI_INPUT_ITEM
};

// Load in the gui choice (combo) elements
const ptGuiChoiceItem GuiChoiceItems[] = {
#define DLRAW_GUI_CHOICE_ITEM
#include "ptGuiItems.i"
#undef  DLRAW_GUI_CHOICE_ITEM
};

// Load in the gui check elements
const ptGuiCheckItem GuiCheckItems[] = {
#define DLRAW_GUI_CHECK_ITEM
#include "ptGuiItems.i"
#undef  DLRAW_GUI_CHECK_ITEM
};

// Load in the non gui elements
const ptItem Items[] = {
#include "ptItems.i"
};

#ifdef ptRAW_OLD__
  #undef _
  #define _ ptRAW_OLD__
#endif

// Macro for inserting a key into the hash and checking it is a new one.
#define M_InsertKeyIntoHash(Key,Item)                      \
  if (m_Hash.contains(Key)) {                              \
    ptLogError(ptError_Argument,                           \
               "Inserting an existing key (%s)",           \
               Key.toAscii().data());                      \
    assert (!m_Hash.contains(Key));                        \
  }                                                        \
  m_Hash[Key] = Item;

///////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
///////////////////////////////////////////////////////////////////////////////

ptSettings::ptSettings(const short InitLevel) {

   assert(InitLevel<9); // 9 reserved for never to be remembered.

   // Gui Numerical inputs. Copy them from the const array in ptSettingItem.
  short NrSettings = sizeof(GuiInputItems)/sizeof(ptGuiInputItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiInputItem Descridlion = GuiInputItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType         = Descridlion.GuiType;
    SettingItem->InitLevel       = Descridlion.InitLevel;
    SettingItem->InJobFile       = Descridlion.InJobFile;
    SettingItem->HasDefaultValue = Descridlion.HasDefaultValue;
    SettingItem->DefaultValue    = Descridlion.DefaultValue;
    SettingItem->MinimumValue    = Descridlion.MinimumValue;
    SettingItem->MaximumValue    = Descridlion.MaximumValue;
    SettingItem->Step            = Descridlion.Step;
    SettingItem->NrDecimals      = Descridlion.NrDecimals;
    SettingItem->Label           = Descridlion.Label;
    SettingItem->ToolTip         = Descridlion.ToolTip;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }
  // Gui Choice inputs. Copy them from the const array in ptSettingItem.
  NrSettings = sizeof(GuiChoiceItems)/sizeof(ptGuiChoiceItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiChoiceItem Descridlion = GuiChoiceItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType         = Descridlion.GuiType;
    SettingItem->InitLevel       = Descridlion.InitLevel;
    SettingItem->InJobFile       = Descridlion.InJobFile;
    SettingItem->HasDefaultValue = Descridlion.HasDefaultValue;
    SettingItem->DefaultValue    = Descridlion.DefaultValue;
    SettingItem->Value           = Descridlion.DefaultValue;
    SettingItem->ToolTip         = Descridlion.ToolTip;
    SettingItem->InitialOptions  = Descridlion.InitialOptions;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }
  // Gui Check inputs. Copy them from the const array in ptSettingItem.
  NrSettings = sizeof(GuiCheckItems)/sizeof(ptGuiCheckItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiCheckItem Descridlion = GuiCheckItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType      = Descridlion.GuiType;
    SettingItem->InitLevel    = Descridlion.InitLevel;
    SettingItem->InJobFile    = Descridlion.InJobFile;
    SettingItem->DefaultValue = Descridlion.DefaultValue;
    SettingItem->Value        = Descridlion.DefaultValue;
    SettingItem->Label        = Descridlion.Label;
    SettingItem->ToolTip      = Descridlion.ToolTip;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }
  // Non gui elements
  NrSettings = sizeof(Items)/sizeof(ptItem);
  for (short i=0; i<NrSettings; i++) {
    ptItem Descridlion = Items[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType      = ptGT_None;
    SettingItem->InitLevel    = Descridlion.InitLevel;
    if (Descridlion.DefaultValue.type() == QVariant::String) {
      QString Tmp = Descridlion.DefaultValue.toString();
      Tmp.replace(QString("@INSTALL@"),QCoreApplication::applicationDirPath());
      Descridlion.DefaultValue = Tmp;
    }
    SettingItem->DefaultValue = Descridlion.DefaultValue;
    SettingItem->Value        = Descridlion.DefaultValue;
    SettingItem->InJobFile    = Descridlion.InJobFile;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }

  // Now we have initialized from static values.
  // In the second round we overwrite now with what's coming from the ini
  // files.

  // Persistent settings.
  QCoreApplication::setOrganizationName(CompanyName);
  QCoreApplication::setOrganizationDomain("mm-log.com/photivo");
  QCoreApplication::setApplicationName(ProgramName);
  // I strongly prefer ini files above register values as they
  // are readable and editeable (think of debug)
  // We don't want something in a windows registry, do we ?
  QSettings::setDefaultFormat(QSettings::IniFormat);
  m_IniSettings = new QSettings;

  QStringList Keys = m_Hash.keys();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    ptSettingItem* Setting = m_Hash[Key];

    if (InitLevel > Setting->InitLevel) {
      // Default needs to be overwritten by something coming from ini file.
      Setting->Value = m_IniSettings->value(Key,Setting->DefaultValue);
      // Correction needed as the values coming from the ini file are
      // often interpreted as strings even if they could be int or so.
      const QVariant::Type TargetType = Setting->DefaultValue.type();
      if (Setting->Value.type() != TargetType) {
        switch (TargetType) {
          case QVariant::Int:
          case QVariant::UInt:
            Setting->Value = Setting->Value.toInt();
            break;
          case QVariant::Double:
    case QMetaType::Float:
            Setting->Value = Setting->Value.toDouble();
            break;
          case QVariant::StringList:
            Setting->Value = Setting->Value.toStringList();
            break;
          default:
            ptLogError(ptError_Argument,"Unexpected type %d",TargetType);
            assert(0);
        }
      }
      // Seen above this shouldn't happen, but better safe then sorry.
      if (Setting->Value.type() != Setting->DefaultValue.type()) {
        ptLogError(ptError_Argument,
                   "Type conversion error from ini file. Should : %d Is : %d\n",
                   Setting->DefaultValue.type(),Setting->Value.type());
        assert(Setting->Value.type() == Setting->DefaultValue.type());
      }
    } else {
      Setting->Value = Setting->DefaultValue;
    }
  }

  // Some ad-hoc corrections
  QFileInfo PathInfo(GetValue("PreviewColorProfile").toString());
  SetValue("PreviewColorProfile",PathInfo.absoluteFilePath());
  PathInfo.setFile(GetValue("PreviewColorProfile").toString());
  SetValue("PreviewColorProfile",PathInfo.absoluteFilePath());
  SetValue("Scaled",GetValue("PipeSize"));
};

////////////////////////////////////////////////////////////////////////////////
//
// Destructor
// Basically dumping the whole Settings hash to ini files.
//
////////////////////////////////////////////////////////////////////////////////

ptSettings::~ptSettings() {
  QStringList Keys = m_Hash.keys();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    ptSettingItem* Setting = m_Hash[Key];
    m_IniSettings->setValue(Key,Setting->Value);
  }
  // Explicit call destructor (such that synced to disk)
  delete m_IniSettings;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetValue
// Access to the hash, but with protection on non existing key.
//
////////////////////////////////////////////////////////////////////////////////

const QVariant ptSettings::GetValue(const QString Key) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  return m_Hash[Key]->Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetInt
//
////////////////////////////////////////////////////////////////////////////////

int ptSettings::GetInt(const QString Key) {
  // Remark : UInt and Int are mixed here.
  // The only settings related type where u is important is uint16_t
  // (dimensions). uint16_t fits in an integer which is 32 bit.
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::Int && Tmp.type() != QVariant::UInt) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::(U)Int' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    if (Tmp.type() == QVariant::String) {
      ptLogError(ptError_Argument,
                 "Additionally : it's a string '%s'\n",
                 Tmp.toString().toAscii().data());
    }
    assert(Tmp.type() == QVariant::Int);
  }
  return Tmp.toInt();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetDouble
//
////////////////////////////////////////////////////////////////////////////////

double ptSettings::GetDouble(const QString Key) {
  QVariant Tmp = GetValue(Key);
  if (static_cast<QMetaType::Type>(Tmp.type()) == QMetaType::Float)
    Tmp.convert(QVariant::Double);
  if (Tmp.type() != QVariant::Double) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::Double' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    assert(Tmp.type() == QVariant::Double);
  }
  return Tmp.toDouble();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetString
//
////////////////////////////////////////////////////////////////////////////////

const QString ptSettings::GetString(const QString Key) {
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::String) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::String' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    assert(Tmp.type() == QVariant::String);
  }
  return Tmp.toString();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetStringList
//
////////////////////////////////////////////////////////////////////////////////

const QStringList ptSettings::GetStringList(const QString Key) {
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::StringList) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::StringList' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    assert(Tmp.type() == QVariant::StringList);
  }
  return Tmp.toStringList();
}

////////////////////////////////////////////////////////////////////////////////
//
// SetValue
// Access to the hash, but with protection on non existing key.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetValue(const QString Key, const QVariant Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->Value = Value;
  // In job mode there are no gui elements and we have to return.
  if (GetInt("JobMode")) return;
  // If it's a gui element, we have to update it at once for consistency.
  switch (m_Hash[Key]->GuiType) {
    case ptGT_Input :
    case ptGT_InputSlider :
    case ptGT_InputSliderHue :
      m_Hash[Key]->GuiInput->SetValue(Value);
      break;
    case ptGT_Choice :
      m_Hash[Key]->GuiChoice->SetValue(Value);
      break;
    case ptGT_Check :
      m_Hash[Key]->GuiCheck->SetValue(Value);
      break;
    default:
      assert(m_Hash[Key]->GuiType == ptGT_None); // Else we missed a gui one
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// SetEnabled
// Enable the underlying gui element (if one)
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetEnabled(const QString Key, const short Enabled) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  switch (m_Hash[Key]->GuiType) {
    case ptGT_Input :
    case ptGT_InputSlider :
    case ptGT_InputSliderHue :
      m_Hash[Key]->GuiInput->SetEnabled(Enabled);
      break;
    case ptGT_Choice :
      m_Hash[Key]->GuiChoice->SetEnabled(Enabled);
      break;
    case ptGT_Check :
      m_Hash[Key]->GuiCheck->SetEnabled(Enabled);
      break;
    default:
      ptLogError(ptError_Argument,
                 "%s is no (expected) gui element.",
                 Key.toAscii().data());
      assert(m_Hash[Key]->GuiType); // Should have gui type !
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// SetMaximum
// Makes only sense for gui input element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetMaximum(const QString Key, const QVariant Maximum) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiInput) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiInput\n",
               Key.toAscii().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiInput->SetMaximum(Maximum);
}

////////////////////////////////////////////////////////////////////////////////
//
// Show (or hide)
// Makes only sense for gui element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void  ptSettings::Show(const QString Key, const short Show) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  switch (m_Hash[Key]->GuiType) {
    case ptGT_Input :
    case ptGT_InputSlider :
    case ptGT_InputSliderHue :
      m_Hash[Key]->GuiInput->Show(Show);
      break;
    case ptGT_Choice :
      m_Hash[Key]->GuiChoice->Show(Show);
      break;
    case ptGT_Check :
      m_Hash[Key]->GuiCheck->Show(Show);
      break;
    default:
      ptLogError(ptError_Argument,
                 "%s is no (expected) gui element.",
                 Key.toAscii().data());
      assert(m_Hash[Key]->GuiType);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// AddOrReplaceOption
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::AddOrReplaceOption(const QString  Key,
                                    const QString  Text,
                                    const QVariant Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  m_Hash[Key]->GuiChoice->AddOrReplaceItem(Text,Value);
}

////////////////////////////////////////////////////////////////////////////////
//
// ClearOptions
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::ClearOptions(const QString  Key, const short WithDefault) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  m_Hash[Key]->GuiChoice->Clear(WithDefault);
}

////////////////////////////////////////////////////////////////////////////////
//
// GetNrOptions
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

int ptSettings::GetNrOptions(const QString Key) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiChoice->Count();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetOptionsValue (at index Index)
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

const QVariant ptSettings::GetOptionsValue(const QString Key,const int Index){
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiChoice->GetItemData(Index);
}

////////////////////////////////////////////////////////////////////////////////
//
// GetCurrentText
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

const QString ptSettings::GetCurrentText(const QString Key){
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiChoice->CurrentText();
}

////////////////////////////////////////////////////////////////////////////////
//
// SetGuiInput
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetGuiInput(const QString Key, ptInput* Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiInput = Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetGuiChoice
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetGuiChoice(const QString Key, ptChoice* Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiChoice = Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetGuiCheck
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetGuiCheck(const QString Key, ptCheck* Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiCheck = Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// Transfer the settings from Settings To DcRaw UserSettings.
// This is sometimes not straightforward as DcRaw assumes certain
// combinations. That's taken care of here.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::ToDcRaw(DcRaw* TheDcRaw) {

  if (!TheDcRaw) return;

  // Relying on m_PipeSize being as defined in the constants ! (1<<Size)
  TheDcRaw->m_UserSetting_HalfSize = GetInt("PipeSize");

  // Input file name

  QString InputFileName = GetStringList("InputFileNameList")[0];
  FREE(TheDcRaw->m_UserSetting_InputFileName);
  TheDcRaw->m_UserSetting_InputFileName =
    (char*) MALLOC(1+strlen(InputFileName.toAscii().data()));
  ptMemoryError(TheDcRaw->m_UserSetting_InputFileName,__FILE__,__LINE__);
  strcpy(TheDcRaw->m_UserSetting_InputFileName,InputFileName.toAscii().data());

  // Adjust Maximum
  TheDcRaw->m_UserSetting_AdjustMaximum = GetDouble("AdjustMaximumThreshold");

  // Denoise.
  TheDcRaw->m_UserSetting_DenoiseThreshold = GetInt("RawDenoiseThreshold");

  // Hotpixel reduction
  TheDcRaw->m_UserSetting_HotpixelReduction = GetDouble("HotpixelReduction");

  // Bayer denoise
  TheDcRaw->m_UserSetting_BayerDenoise = GetInt("BayerDenoise");

  // Green equilibration
  TheDcRaw->m_UserSetting_GreenEquil = GetInt("GreenEquil");

  // CA auto correction
  TheDcRaw->m_UserSetting_CaCorrect = GetInt("CaCorrect");

  // CFA Line denoise
  TheDcRaw->m_UserSetting_CfaLineDn = GetInt("CfaLineDenoise");

  // Interpolation
  TheDcRaw->m_UserSetting_Quality = GetInt("Interpolation");

  // White balance settings.
  switch (GetInt("WhiteBalance")) {
    case ptWhiteBalance_Camera :
      TheDcRaw->m_UserSetting_CameraWb = 1;
      TheDcRaw->m_UserSetting_AutoWb   = 0;
      TheDcRaw->m_UserSetting_Multiplier[0] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[1] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[2] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[3] = 0.0;
      TheDcRaw->m_UserSetting_GreyBox[0] = 0;
      TheDcRaw->m_UserSetting_GreyBox[1] = 0;
      TheDcRaw->m_UserSetting_GreyBox[2] = 0xFFFF;
      TheDcRaw->m_UserSetting_GreyBox[3] = 0xFFFF;
      break;
    case ptWhiteBalance_Auto :
      TheDcRaw->m_UserSetting_CameraWb = 0;
      TheDcRaw->m_UserSetting_AutoWb   = 1;
      TheDcRaw->m_UserSetting_Multiplier[0] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[1] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[2] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[3] = 0.0;
      TheDcRaw->m_UserSetting_GreyBox[0] = 0;
      TheDcRaw->m_UserSetting_GreyBox[1] = 0;
      TheDcRaw->m_UserSetting_GreyBox[2] = 0xFFFF;
      TheDcRaw->m_UserSetting_GreyBox[3] = 0xFFFF;
      break;
    case ptWhiteBalance_Spot :
      TheDcRaw->m_UserSetting_CameraWb = 0;
      TheDcRaw->m_UserSetting_AutoWb   = 1; // GreyBox must have auto on !
      TheDcRaw->m_UserSetting_Multiplier[0] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[1] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[2] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[3] = 0.0;

      // The selection, which is in preview coordinates, must
      // be transformed back to the original.
      // Express always in size of original image !
      // Also take into account TheDcRaw->m_Flip

      { // Jump to case label issue.
      int TmpPipeSize = GetInt("PipeSize");
      uint16_t X = (1<<TmpPipeSize) * GetInt("VisualSelectionX");
      uint16_t Y = (1<<TmpPipeSize) * GetInt("VisualSelectionY");
      uint16_t W = (1<<TmpPipeSize) * GetInt("VisualSelectionWidth");
      uint16_t H = (1<<TmpPipeSize) * GetInt("VisualSelectionHeight");

      uint16_t TargetW = W;
      uint16_t TargetH = H;
      if (TheDcRaw->m_Flip & 4) {
        SWAP(X,Y);
        SWAP(TargetW,TargetH);
      }
      if (TheDcRaw->m_Flip & 2) Y = GetInt("ImageH")-1-Y-TargetH;
      if (TheDcRaw->m_Flip & 1) X = GetInt("ImageW")-1-X-TargetW;
      TheDcRaw->m_UserSetting_GreyBox[0] = X;
      TheDcRaw->m_UserSetting_GreyBox[1] = Y;
      TheDcRaw->m_UserSetting_GreyBox[2] = TargetW;
      TheDcRaw->m_UserSetting_GreyBox[3] = TargetH;
      }

      TRACEKEYVALS("GreyBox[0]","%d",TheDcRaw->m_UserSetting_GreyBox[0]);
      TRACEKEYVALS("GreyBox[1]","%d",TheDcRaw->m_UserSetting_GreyBox[1]);
      TRACEKEYVALS("GreyBox[2]","%d",TheDcRaw->m_UserSetting_GreyBox[2]);
      TRACEKEYVALS("GreyBox[3]","%d",TheDcRaw->m_UserSetting_GreyBox[3]);

      break;
    default : // this entails as well manual as preset from ptWhiteBalances.
      TheDcRaw->m_UserSetting_CameraWb = 0;
      TheDcRaw->m_UserSetting_AutoWb   = 0;
      TheDcRaw->m_UserSetting_Multiplier[0]= GetDouble("RMultiplier");
      TheDcRaw->m_UserSetting_Multiplier[1]= GetDouble("GMultiplier");
      TheDcRaw->m_UserSetting_Multiplier[2] =GetDouble("BMultiplier");
      TheDcRaw->m_UserSetting_Multiplier[3] =
        TheDcRaw->m_Colors == 4 ? GetDouble("GMultiplier") : 0.0;
      TheDcRaw->m_UserSetting_GreyBox[0] = 0;
      TheDcRaw->m_UserSetting_GreyBox[1] = 0;
      TheDcRaw->m_UserSetting_GreyBox[2] = 0xFFFF;
      TheDcRaw->m_UserSetting_GreyBox[3] = 0xFFFF;
  }

  // Bad pixels settings.
  switch(GetInt("HaveBadPixels")) {
    case 0 : // None
      TheDcRaw->m_UserSetting_BadPixelsFileName = NULL;
      break;
    case 1 : // Load one : should not happen !
      assert(0);
      break;
    case 2 :
      FREE(TheDcRaw->m_UserSetting_BadPixelsFileName);
      TheDcRaw->m_UserSetting_BadPixelsFileName = (char *)
        MALLOC(1+
             strlen(GetString("BadPixelsFileName").toAscii().data()));
      ptMemoryError(TheDcRaw->m_UserSetting_BadPixelsFileName,
                    __FILE__,__LINE__);
      strcpy(TheDcRaw->m_UserSetting_BadPixelsFileName,
             GetString("BadPixelsFileName").toAscii().data());
      break;
    default :
      assert(0);
  }

  // Dark frame settings.
  switch(GetInt("HaveDarkFrame")) {
    case 0 : // None
      TheDcRaw->m_UserSetting_DarkFrameFileName = NULL;
      break;
    case 1 : // Load one : should not happen !
      assert(0);
      break;
    case 2 :
      FREE(TheDcRaw->m_UserSetting_DarkFrameFileName);
      TheDcRaw->m_UserSetting_DarkFrameFileName = (char *)
        MALLOC(1+
             strlen(GetString("DarkFrameFileName").toAscii().data()));
      ptMemoryError(TheDcRaw->m_UserSetting_DarkFrameFileName,
                    __FILE__,__LINE__);
      strcpy(TheDcRaw->m_UserSetting_DarkFrameFileName,
             GetString("DarkFrameFileName").toAscii().data());
      break;
    default :
      assert(0);
  }

  // Blackpoint settings.
  switch (GetInt("ManualBlackPoint")) {
    case 0 : // Automatic.
      TheDcRaw->m_UserSetting_BlackPoint = -1;
      break;
    default : // Manual set
      TheDcRaw->m_UserSetting_BlackPoint = GetInt("BlackPoint");
      break;
  }

  // Whitepoint settings.
  switch (GetInt("ManualWhitePoint")) {
    case 0 : // Automatic.
      TheDcRaw->m_UserSetting_Saturation = -1;
      break;
    default : // Manual set
      TheDcRaw->m_UserSetting_Saturation = GetInt("WhitePoint");
      break;
  }
  // Normalization of Multipliers
  TheDcRaw->m_UserSetting_MaxMultiplier = GetInt("MultiplierEnhance");

  // Interpolation passes.
  TheDcRaw->m_UserSetting_InterpolationPasses = GetInt("InterpolationPasses");

  // Median Filter.
  TheDcRaw->m_UserSetting_MedianPasses = GetInt("MedianPasses");
  TheDcRaw->m_UserSetting_ESMedianPasses = GetInt("ESMedianPasses");
  TheDcRaw->m_UserSetting_EeciRefine = GetInt("EeciRefine");

  // Clip factor
  TheDcRaw->m_UserSetting_photivo_ClipMode       = GetInt("ClipMode");
  TheDcRaw->m_UserSetting_photivo_ClipParameter  = GetInt("ClipParameter");
}

////////////////////////////////////////////////////////////////////////////////
//
// Transfers the settings from DcRaw back to the Gui.
// In some situations , DcRaw can overwrite or calculate values.
// The output of those are here fed back to the Gui settings such
// that they are correctly reflected.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::FromDcRaw(DcRaw* TheDcRaw) {

  if (!TheDcRaw) return;

  // Copy make and model to our gui settings (f.i.
  // white balances rely on it)
  SetValue("CameraMake",TheDcRaw->m_CameraMake);
  SetValue("CameraModel",TheDcRaw->m_CameraModel);

  // Multipliers.
  SetValue("RMultiplier",VALUE(TheDcRaw->m_PreMultipliers[0]));
  SetValue("GMultiplier",VALUE(TheDcRaw->m_PreMultipliers[1]));
  SetValue("BMultiplier",VALUE(TheDcRaw->m_PreMultipliers[2]));

  // (D65) Multipliers to ColorTemperature.
  //
  // m_D65Multipliers are supposed to be D65
  // (setting Pre to the ratio of the D65 delivers
  // rgbWB = (x,x,x) and 6500 temperature).

  double RefRGB[3];
  if (TheDcRaw->m_RawColor) {
    for (short c=0; c<3; c++) {
      RefRGB[c] = VALUE(TheDcRaw->m_D65Multipliers[c]) /
                  VALUE(TheDcRaw->m_PreMultipliers[c]);
    }
  } else {
    // If not raw, we have to calculate back sRGB references to the cam rgb.
    for (short c=0; c<3; c++) {
      RefRGB[c] = 0;
      for (short cc=0; cc<TheDcRaw->m_Colors ; cc++) {
        RefRGB[c] += TheDcRaw->m_MatrixCamRGBToSRGB[c][cc] *
                     VALUE(TheDcRaw->m_D65Multipliers[cc]) /
                     VALUE(TheDcRaw->m_PreMultipliers[cc]);
      }
    }
  }

  int    TmpColorTemperature;
  double TmpGreenIntensity;
  RGBToTemperature(RefRGB,
                   &TmpColorTemperature,
                   &TmpGreenIntensity);
  SetValue("ColorTemperature",TmpColorTemperature);
  SetValue("GreenIntensity",TmpGreenIntensity);

  // Blackpoint setting in case not manual.
  if (!GetInt("ManualBlackPoint")) {
    SetValue("BlackPoint",TheDcRaw->m_BlackLevel_AfterPhase1);
  }

  // Whitepoint setting in case not manual.
  if (!GetInt("ManualWhitePoint")) {
    SetValue("WhitePoint",TheDcRaw->m_WhiteLevel_AfterPhase1);
  }

  // EOS exposure normalization might be a result of running DcRaw
  // (ie we have to normalize the exposure further in the flow.
  // TODO This is coming from ufraw. Seems fair, but no clue
  // what's the logic behind the calculation. Someone ?

  double EOSExposureNormalization = 1.0;
  if (strcmp(TheDcRaw->m_CameraMake, "Canon")==0 &&
      strncmp(TheDcRaw->m_CameraModel, "EOS", 3)==0 ) {
    int Max = (int) VALUE(TheDcRaw->m_CameraMultipliers[0]);
    for (short c=1; c<TheDcRaw->m_Colors; c++) {
      if (VALUE(TheDcRaw->m_CameraMultipliers[c]) > Max) {
        Max = (int) VALUE(TheDcRaw->m_CameraMultipliers[c]);
      }
    }
    // The 100 is a pretty random value that makes sure above
    // was a 'right' number calculated and it's not bogus
    // due to for instance not being able reading CamMultipliers
    if ( Max > 100 ) {
      EOSExposureNormalization =
        (4096.0-TheDcRaw->m_BlackLevel_AfterPhase1)/Max;
    }
  }

  TRACEKEYVALS("EOSExposureNorm","%f",EOSExposureNormalization);

  SetValue("ExposureNormalization",
           // EV conversion !
           log(EOSExposureNormalization/TheDcRaw->m_MinPreMulti)/log(2));

  TRACEKEYVALS("ExposureNorm(EV)","%f", GetDouble("ExposureNormalization"));
}

////////////////////////////////////////////////////////////////////////////////
//
// Tool Info
// IsActive contains, if the filter will be processed!
//
////////////////////////////////////////////////////////////////////////////////

struct sToolInfo {
  QString               Name;
  int                   IsActive;
  int                   IsHidden;
  int                   IsBlocked;
};

sToolInfo ToolInfo (const QString GuiName) {
  sToolInfo Info = {"N.N.",0,0,0};
  // Tab Geometry
  if (GuiName == "TabRotation") {
      Info.Name = "Rotation";
      Info.IsActive = Settings->GetDouble("Rotate")!=0.0?1:0;
  } else if (GuiName == "TabCrop") {
      Info.Name = "Crop";
      Info.IsActive = Settings->GetInt("Crop");
  } else if (GuiName == "TabResize") {
      Info.Name = "Resize";
      Info.IsActive = Settings->GetInt("Resize");
  } else if (GuiName == "TabFlip") {
      Info.Name = "Flip";
      Info.IsActive = Settings->GetInt("FlipMode");
  } else if (GuiName == "TabBlock") {
      Info.Name = "Block";
      Info.IsActive = !Settings->GetInt("JobMode") &&
                      Settings->GetInt("GeometryBlock");
  }
  // Tab RGB
  else if (GuiName == "TabChannelMixer") {
      Info.Name = "Channel mixer";
      Info.IsActive = Settings->GetInt("ChannelMixer");
  } else if (GuiName == "TabHighlights") {
      Info.Name = "RGB highlights";
      Info.IsActive = (Settings->GetDouble("HighlightsR")!=0.0 ||
                       Settings->GetDouble("HighlightsG")!=0.0 ||
                       Settings->GetDouble("HighlightsB")!=0.0)!=0?1:0;
  } else if (GuiName == "TabColorIntensity") {
      Info.Name = "RGB color intensity";
      Info.IsActive = (Settings->GetInt("Vibrance") ||
                       Settings->GetInt("IntensityRed") ||
                       Settings->GetInt("IntensityGreen") ||
                       Settings->GetInt("IntensityBlue"))!=0?1:0;
  } else if (GuiName == "TabBrightness") {
      Info.Name = "RGB brightness";
      Info.IsActive = (Settings->GetDouble("CatchWhite")!=0.0 ||
                       Settings->GetDouble("CatchBlack")!=0.0 ||
                       Settings->GetDouble("ExposureGain")!=0.0)!=0?1:0;
  } else if (GuiName == "TabExposure") {
      Info.Name = "Exposure";
      Info.IsActive = Settings->GetDouble("Exposure")!=0.0?1:0;
  } else if (GuiName == "TabReinhard05") {
      Info.Name = "Reinhard 05";
      Info.IsActive = Settings->GetInt("Reinhard05");
  } else if (GuiName == "TabGammaTool") {
      Info.Name = "RGB gamma tool";
      Info.IsActive = Settings->GetDouble("RGBGammaAmount") != 1.0?1:0;
  } else if (GuiName == "TabNormalization") {
      Info.Name = "Normalization";
      Info.IsActive = Settings->GetDouble("NormalizationOpacity")!=0.0?1:0;
  } else if (GuiName == "TabColorEnhance") {
      Info.Name = "RGB color enhancement";
      Info.IsActive = (Settings->GetDouble("ColorEnhanceShadows")!=0.0 ||
                       Settings->GetDouble("ColorEnhanceHighlights")!=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBRecovery") {
      Info.Name = "RGB Low/Mid/Highlight Recovery";
      Info.IsActive = Settings->GetInt("LMHLightRecovery1MaskType") ||
                      Settings->GetInt("LMHLightRecovery2MaskType");
  } else if (GuiName == "TabRGBTextureContrast") {
      Info.Name = "RGB texture contrast";
      Info.IsActive = (Settings->GetDouble("RGBTextureContrastAmount")!=0.0 &&
                       Settings->GetDouble("RGBTextureContrastOpacity")!=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBLocalContrast1") {
      Info.Name = "RGB Local Contrast 1";
      Info.IsActive = Settings->GetInt("Microcontrast1MaskType");
  } else if (GuiName == "TabRGBLocalContrast2") {
      Info.Name = "RGB Local Contrast 2";
      Info.IsActive = Settings->GetInt("Microcontrast2MaskType");
  } else if (GuiName == "TabRGBContrast") {
      Info.Name = "RGB Contrast";
      Info.IsActive = Settings->GetDouble("RGBContrastAmount")!=0.0;
  } else if (GuiName == "TabRGBLevels") {
      Info.Name = "RGB Levels";
      Info.IsActive = (Settings->GetDouble("LevelsBlackPoint")!=0.0 ||
                      (Settings->GetDouble("LevelsWhitePoint")-1.0)!=0.0)?1:0;
  } else if (GuiName == "TabRGBCurve") {
      Info.Name = "RGB tone curve";
      Info.IsActive = Settings->GetInt("CurveRGB");
  }
  // Lab Color and Contrast
  else if (GuiName == "TabLABTransform") {
      Info.Name = "Lab Transform";
      Info.IsActive = Settings->GetInt("LABTransform");
  } else if (GuiName == "TabLABShadowsHighlights") {
      Info.Name = "Lab Shadows and Highlights";
      Info.IsActive = (Settings->GetDouble("ShadowsHighlightsFine")!=0.0 ||
                      Settings->GetDouble("ShadowsHighlightsCoarse")!=0.0 ||
                      Settings->GetInt("CurveShadowsHighlights"))?1:0;
  } else if (GuiName == "TabLABRecovery") {
      Info.Name = "Lab Low/Mid/Highlight Recovery";
      Info.IsActive = Settings->GetInt("LabLMHLightRecovery1MaskType") ||
                      Settings->GetInt("LabLMHLightRecovery2MaskType");
  } else if (GuiName == "TabLABDRC") {
      Info.Name = "Lab Dynamic Range Compression";
      Info.IsActive = Settings->GetDouble("DRCBeta")!=1.0?1:0;
  } else if (GuiName == "TabLABTextureCurve") {
      Info.Name = "Lab Texture Curve";
      Info.IsActive = Settings->GetInt("CurveTexture");
  } else if (GuiName == "TabLABTexture1") {
      Info.Name = "Lab Texture Contrast 1";
      Info.IsActive = (Settings->GetDouble("TextureContrast1Amount")!=0.0 &&
                      Settings->GetDouble("TextureContrast1Opacity")!=0.0)?1:0;
  } else if (GuiName == "TabLABTexture2") {
      Info.Name = "Lab Texture Contrast 2";
      Info.IsActive = (Settings->GetDouble("TextureContrast2Amount")!=0.0 &&
                      Settings->GetDouble("TextureContrast2Opacity")!=0.0)?1:0;
  } else if (GuiName == "TabLABLocalContrast1") {
      Info.Name = "Lab Local Contrast 1";
      Info.IsActive = Settings->GetInt("LabMicrocontrast1MaskType");
  } else if (GuiName == "TabLABLocalContrast2") {
      Info.Name = "Lab Local Contrast 2";
      Info.IsActive = Settings->GetInt("LabMicrocontrast2MaskType");
  } else if (GuiName == "TabLABLCStretch1") {
      Info.Name = "Lab Local Contrast Stretch 1";
      Info.IsActive = Settings->GetDouble("LC1Opacity")!=0.0?1:0;
  } else if (GuiName == "TabLABLCStretch2") {
      Info.Name = "Lab Local Contrast Stretch 2";
      Info.IsActive = Settings->GetDouble("LC2Opacity")!=0.0?1:0;
  } else if (GuiName == "TabLABContrast") {
      Info.Name = "Lab L* Contrast";
      Info.IsActive = Settings->GetDouble("ContrastAmount")!=0.0?1:0;
  } else if (GuiName == "TabLABSaturation") {
      Info.Name = "Lab Saturation";
      Info.IsActive = Settings->GetDouble("SaturationAmount")!=0.0?1:0;
  } else if (GuiName == "TabLABColorBoost") {
      Info.Name = "Lab Color Boost";
      Info.IsActive = ((Settings->GetDouble("ColorBoostValueA")-1)!=0.0 ||
                      (Settings->GetDouble("ColorBoostValueB")-1)!=0.0)?1:0;
  } else if (GuiName == "TabLABLevels") {
      Info.Name = "Lab Levels";
      Info.IsActive = (Settings->GetDouble("LabLevelsBlackPoint")!=0.0 ||
                      (Settings->GetDouble("LabLevelsWhitePoint")-1.0)!=0.0)?1:0;
  }
  // Lab Sharpen and Noise
  else if (GuiName == "TabLABEAW") {
      Info.Name = "Lab EAW equalizer";
      Info.IsActive = (Settings->GetDouble("EAWLevel1") != 0.0 ||
                       Settings->GetDouble("EAWLevel2") != 0.0 ||
                       Settings->GetDouble("EAWLevel3") != 0.0 ||
                       Settings->GetDouble("EAWLevel4") != 0.0 ||
                       Settings->GetDouble("EAWLevel5") != 0.0 ||
                       Settings->GetDouble("EAWLevel6") != 0.0)?1:0;
  } else if (GuiName == "TabLABGreyC") {
      Info.Name = "Lab GreyCStoration";
      Info.IsActive = ((Settings->GetInt("FullOutput") &&
                        Settings->GetInt("GREYCLab")) ||
                       Settings->GetInt("GREYCLab")>=2)!=0?1:0;
  } else if (GuiName == "TabWaveletDenoise") {
      Info.Name = "Lab Wavelet denoise";
      Info.IsActive = (Settings->GetDouble("WaveletDenoiseL")!=0.0 ||
                      Settings->GetDouble("WaveletDenoiseA")!=0.0 ||
                      Settings->GetDouble("WaveletDenoiseB")!=0.0)?1:0;
  } else if (GuiName == "TabLuminanceDenoise") {
      Info.Name = "Lab Luminance denoise";
      Info.IsActive = Settings->GetDouble("BilateralLOpacity")!=0.0?1:0;
  } else if (GuiName == "TabPyramidDenoise") {
      Info.Name = "Lab Pyramid denoise";
      Info.IsActive = (Settings->GetInt("PyrDenoiseLAmount")!=0.0||
                       Settings->GetInt("PyrDenoiseABAmount"))!=0.0?1:0;
  } else if (GuiName == "TabColorDenoise") {
      Info.Name = "Lab Levels";
      Info.IsActive = (Settings->GetDouble("BilateralASigmaR")!=0.0 ||
                      Settings->GetDouble("BilateralBSigmaR")!=0.0)?1:0;
  } else if (GuiName == "TabDetailCurve") {
      Info.Name = "Lab detail curve";
      Info.IsActive = Settings->GetInt("CurveDenoise") ||
                      Settings->GetDouble("DetailCurveHotpixel")!=0.0;
  } else if (GuiName == "TabLABGradientSharpen") {
      Info.Name = "Lab Gradient Sharpen";
      Info.IsActive = Settings->GetInt("GradientSharpenPasses") ||
                      Settings->GetDouble("MLMicroContrastStrength")!=0.0 ||
                      Settings->GetDouble("LabHotpixel")!=0.0;
  } else if (GuiName == "TabLABWiener") {
      Info.Name = "Lab Wiener filter";
      Info.IsActive = Settings->GetInt("WienerFilter");
  } else if (GuiName == "TabInverseDiffusion") {
      Info.Name = "Lab inverse diffusion";
      Info.IsActive = Settings->GetInt("InverseDiffusionIterations")!=0?1:0;
  } else if (GuiName == "TabLABUSM") {
      Info.Name = "Lab USM";
      Info.IsActive = ((Settings->GetInt("FullOutput") &&
                        Settings->GetInt("USM")) ||
                       Settings->GetInt("USM")==2)!=0?1:0;
  } else if (GuiName == "TabLABHighpass") {
      Info.Name = "Lab highpass";
      Info.IsActive = ((Settings->GetInt("FullOutput") &&
                        Settings->GetInt("Highpass")) ||
                       Settings->GetInt("Highpass")==2)!=0?1:0;
  } else if (GuiName == "TabLABFilmGrain") {
      Info.Name = "Lab film grain";
      Info.IsActive = (Settings->GetInt("Grain1MaskType") ||
                      Settings->GetInt("Grain2MaskType"))!=0?1:0;
  } else if (GuiName == "TabLABViewLab") {
      Info.Name = "Lab view Lab";
      Info.IsActive = Settings->GetInt("ViewLAB")!=0?1:0;
  }
  // Tab Lab EyeCandy
  else if (GuiName == "TabLbyHue") {
      Info.Name = "Lab luminance by hue curve";
      Info.IsActive = Settings->GetInt("CurveLByHue")!=0?1:0;
  } else if (GuiName == "TabSaturationCurve") {
      Info.Name = "Lab saturation curve";
      Info.IsActive = Settings->GetInt("CurveSaturation")!=0?1:0;
  } else if (GuiName == "TabLCurve") {
      Info.Name = "Lab luminance curve";
      Info.IsActive = Settings->GetInt("CurveL")!=0?1:0;
  } else if (GuiName == "TabABCurves") {
      Info.Name = "Lab color curves";
      Info.IsActive = (Settings->GetInt("CurveLa") ||
                       Settings->GetInt("CurveLb"))!=0?1:0;
  } else if (GuiName == "TabColorContrast") {
      Info.Name = "Lab color contrast";
      Info.IsActive = Settings->GetDouble("ColorcontrastOpacity")!=0.0?1:0;
  } else if (GuiName == "TabLABToneAdjust1") {
      Info.Name = "Lab tone adjustment 1";
      Info.IsActive = Settings->GetInt("LABToneAdjust1MaskType")!=0?1:0;
  } else if (GuiName == "TabLABToneAdjust2") {
      Info.Name = "Lab tone adjustment 2";
      Info.IsActive = Settings->GetInt("LABToneAdjust2MaskType")!=0?1:0;
  } else if (GuiName == "TabLAdjustment") {
      Info.Name = "Lab luminance adjustment";
      Info.IsActive = (Settings->GetDouble("LAdjustC1")!=0.0 ||
                      Settings->GetDouble("LAdjustC2")!=0.0 ||
                      Settings->GetDouble("LAdjustC3")!=0.0 ||
                      Settings->GetDouble("LAdjustC4")!=0.0 ||
                      Settings->GetDouble("LAdjustC5")!=0.0 ||
                      Settings->GetDouble("LAdjustC6")!=0.0 ||
                      Settings->GetDouble("LAdjustC7")!=0.0 ||
                      Settings->GetDouble("LAdjustC8")!=0.0)!=0?1:0;
  } else if (GuiName == "TabSaturationAdjustment") {
      Info.Name = "Lab saturation adjustment";
      Info.IsActive = (Settings->GetDouble("LAdjustSC1")!=0.0 ||
                      Settings->GetDouble("LAdjustSC2")!=0.0 ||
                      Settings->GetDouble("LAdjustSC3")!=0.0 ||
                      Settings->GetDouble("LAdjustSC4")!=0.0 ||
                      Settings->GetDouble("LAdjustSC5")!=0.0 ||
                      Settings->GetDouble("LAdjustSC6")!=0.0 ||
                      Settings->GetDouble("LAdjustSC7")!=0.0 ||
                      Settings->GetDouble("LAdjustSC8")!=0.0)!=0?1:0;
  } else if (GuiName == "TabLABTone") {
      Info.Name = "Lab toning";
      Info.IsActive = (Settings->GetDouble("LABToneAmount")!=0.0 ||
                       Settings->GetDouble("LABToneSaturation") != 1.0 ||
                       Settings->GetDouble("LABToneSAmount") != 0.0 ||
                       Settings->GetDouble("LABToneSSaturation") != 1.0 ||
                       Settings->GetDouble("LABToneMAmount")!=0.0 ||
                       Settings->GetDouble("LABToneMSaturation") != 1.0 ||
                       Settings->GetDouble("LABToneHAmount")!=0.0 ||
                       Settings->GetDouble("LABToneHSaturation") != 1.0)!=0?1:0;
  } else if (GuiName == "TabLABVignette") {
      Info.Name = "Lab vignette";
      Info.IsActive = (Settings->GetInt("LabVignetteMode") != 0 &&
                       Settings->GetDouble("LabVignetteAmount") != 0.0)!=0?1:0;
  }
  // Tab EyeCandy
  else if (GuiName == "TabBW") {
      Info.Name = "Black and White";
      Info.IsActive = Settings->GetDouble("BWStylerOpacity")!=0.0?1:0;
  } else if (GuiName == "TabSimpleTone") {
      Info.Name = "Simple toning";
      Info.IsActive = (Settings->GetDouble("SimpleToneR") !=0.0 ||
                       Settings->GetDouble("SimpleToneG") !=0.0 ||
                       Settings->GetDouble("SimpleToneB") !=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBTone") {
      Info.Name = "RGB Toning";
      Info.IsActive = (Settings->GetInt("Tone1MaskType") ||
                       Settings->GetInt("Tone2MaskType"))!=0?1:0;
  } else if (GuiName == "TabCrossProcessing") {
      Info.Name = "Cross processing";
      Info.IsActive = Settings->GetInt("CrossprocessingMode")!=0?1:0;
  } else if (GuiName == "TabECContrast") {
      Info.Name = "Eyecandy contrast";
      Info.IsActive = Settings->GetDouble("RGBContrast2Amount")!=0.0?1:0;
  } else if (GuiName == "TabGradualOverlay1") {
      Info.Name = "Gradual Overlay 1";
      Info.IsActive = (Settings->GetInt("GradualOverlay1") &&
                       Settings->GetDouble("GradualOverlay1Amount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabGradualOverlay2") {
      Info.Name = "Gradual Overlay 2";
      Info.IsActive = (Settings->GetInt("GradualOverlay2") &&
                       Settings->GetDouble("GradualOverlay2Amount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBVignette") {
      Info.Name = "Eyecandy vignette";
      Info.IsActive = (Settings->GetInt("VignetteMode") &&
                       Settings->GetDouble("VignetteAmount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabSoftglow") {
      Info.Name = "Softglow / Orton";
      Info.IsActive = (Settings->GetInt("SoftglowMode") &&
                       Settings->GetDouble("SoftglowAmount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabECColorIntensity") {
      Info.Name = "Eyecandy color intensity";
      Info.IsActive = (Settings->GetInt("Vibrance2") ||
                       Settings->GetInt("Intensity2Red") ||
                       Settings->GetInt("Intensity2Green") ||
                       Settings->GetInt("Intensity2Blue"))!=0?1:0;
  } else if (GuiName == "TabRToneCurve") {
      Info.Name = "Eyecandy R tone curve";
      Info.IsActive = Settings->GetInt("CurveR");
  } else if (GuiName == "TabGToneCurve") {
      Info.Name = "Eyecandy G tone curve";
      Info.IsActive = Settings->GetInt("CurveG");
  } else if (GuiName == "TabBToneCurve") {
      Info.Name = "Eyecandy B tone curve";
      Info.IsActive = Settings->GetInt("CurveB");
  }
  // Tab Output
  else if (GuiName == "TabBaseCurve") {
      Info.Name = "Output base curve";
      Info.IsActive = Settings->GetInt("BaseCurve")!=0?1:0;
  } else if (GuiName == "TabGammaCompensation") {
      Info.Name = "Output gamma compensation";
      Info.IsActive = Settings->GetInt("OutputGammaCompensation")!=0?1:0;
  } else if (GuiName == "TabWebResize") {
      Info.Name = "Output webreisze";
      Info.IsActive = (Settings->GetInt("WebResize")==2 ||
                       (Settings->GetInt("FullOutput") &&
                        Settings->GetInt("WebResize")==1))!=0?1:0;
  } else if (GuiName == "TabAfterGammaCurve") {
      Info.Name = "Output after gamma curve";
      Info.IsActive = Settings->GetInt("BaseCurve2")!=0?1:0;
  } else if (GuiName == "TabOutContrast") {
      Info.Name = "Output sigmiodal contrast";
      Info.IsActive = Settings->GetDouble("RGBContrast3Amount")!=0.0?1:0;
  } else if (GuiName == "TabOutWiener") {
      Info.Name = "Output wiener filter";
      Info.IsActive = Settings->GetInt("WienerFilter2")!=0?1:0;
  }

  // tool blocked?
  Info.IsBlocked = (Settings->GetStringList("BlockedTools")).contains(GuiName)?1:0;

  // tool hidden?
  Info.IsHidden = (Settings->GetStringList("HiddenTools")).contains(GuiName)?1:0;
  return Info;
}

int ptSettings::ToolAlwaysVisible(const QString GuiName) {
  QStringList VisibleTools =
  (QStringList()
    // Settings tab
    << "TabWorkColorSpace"
    << "TabPreviewColorSpace"
    << "TabGimpCommand"
    << "TabRememberSettings"
    << "TabInputControl"
    << "TabToolBoxControl"
    << "TabTabStatusIndicator"
    << "TabPreviewControl"
    << "TabTheming"
    << "TabTranslation"
    << "TabMemoryTest"
    // Input tab
    << "TabInput"
    << "TabCameraColorSpace"
    << "TabGenCorrections"
    << "TabWhiteBalance"
    << "TabDemosaicing"
    << "TabHighlightRecovery"
    // Output Tab
    << "TabOutputColorSpace"
    << "TabOutParameters"
    << "TabOutput");
  if (VisibleTools.contains(GuiName)) return 1;
  return 0;
}

QString ptSettings::ToolGetName (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.Name;
}

int ptSettings::ToolIsActive (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return (Info.IsHidden || Info.IsBlocked)?0:Info.IsActive;
}

int ptSettings::ToolIsBlocked (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.IsBlocked;
}

int ptSettings::ToolIsHidden (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.IsHidden;
}
///////////////////////////////////////////////////////////////////////////////
