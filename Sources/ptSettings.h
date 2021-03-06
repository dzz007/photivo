/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#ifndef DLSETTINGS_H
#define DLSETTINGS_H

#include "ptConstants.h"
#include "ptGuiOptions.h"
#include "ptInput.h"
#include "ptChoice.h"
#include "ptCheck.h"

#include <QtCore>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>
#include <QLabel>

//==============================================================================

class ptDcRaw;

////////////////////////////////////////////////////////////////////////////////
//
// ptSettingItem
// Description of any setting.
// Gui elements will be attached and handled via the setting.
// Remark that one never will use this class directly but only
// via its friend class ptSettings. So nothing public.
//
////////////////////////////////////////////////////////////////////////////////

// Some forward declarations.
struct ptGuiInputItem;
struct ptGuiChoiceItem;
struct ptGuiCheckItem;
struct ptItem;

class ptSettingItem {
  // All will be accessed via ptSettings
  friend class ptSettings;

  short    InitLevel;       // the smaller, the longer coming from ini fle
  QVariant DefaultValue;    // Give always one. Maybe nonsensical.
  QVariant Value;
  short    InJobFile;       // Should be in job file ?
  short    GuiType;         // Associated gui type
  short    HasDefaultValue; // For gui : is the default sensical.
  // Remainder is gui related stuff.
  QVariant MinimumValue;
  QVariant MaximumValue;
  QVariant Step;
  short    NrDecimals;
  QString  Label;
  QString  ToolTip;
  const ptGuiOptionsItem * InitialOptions; // For choice (combo) gui elements.

  // Reference to the associated Gui stuff
  ptInput*  GuiInput;
  ptChoice* GuiChoice;
  ptCheck*  GuiCheck;

  // Constructor of an item :
  // Make sure non QVariants all have sensible defaults.
  void ptSettingsItem() {
    InitLevel       = 9; // Never reached.
    InJobFile       = 0;
    GuiType         = ptGT_None;
    HasDefaultValue = 0;
    NrDecimals      = 0;
    InitialOptions  = NULL;
    GuiInput        = NULL;
    GuiCheck        = NULL;
    GuiChoice       = NULL;
  }
}; // End ptSettingItem class.


////////////////////////////////////////////////////////////////////////////////
//
// ptSettings
// This class finally works on underlying ptSettingItem, which
// are shielded. All access and changes via this one.
//
////////////////////////////////////////////////////////////////////////////////

class ptSettings {
Q_DECLARE_TR_FUNCTIONS(ptSettings)

//-------------------------------------

private:
  // We have this private section on top for being able
  // referring to m_Hash already during declaration of class.
  // Hash with the items.
  QHash <QString,ptSettingItem*> m_Hash;

 //-------------------------------------

public:
  // First of all a number of simple accessors to the ptSettingItem
  // characterized by 'Key'
  const QStringList GetKeys()                         { return m_Hash.keys(); }
  short GetGuiType(const QString Key)                 { return m_Hash[Key]->GuiType;}
  short GetHasDefaultValue(const QString Key)         { return m_Hash[Key]->HasDefaultValue;}
  const QVariant GetDefaultValue(const QString Key)   { return m_Hash[Key]->DefaultValue;}
  const QVariant GetMinimumValue(const QString Key)   { return m_Hash[Key]->MinimumValue;}
  const QVariant GetMaximumValue(const QString Key)   { return m_Hash[Key]->MaximumValue;}
  const QVariant GetStep(const QString Key)           { return m_Hash[Key]->Step;}
  short GetNrDecimals(const QString Key)              { return m_Hash[Key]->NrDecimals;}
  const QString GetLabel(const QString Key)           { return m_Hash[Key]->Label;}
  const QString GetToolTip(const QString Key)         { return m_Hash[Key]->ToolTip;}
  const ptGuiOptionsItem* GetInitialOptions(const QString Key)    { return m_Hash[Key]->InitialOptions;}
  short GetInJobFile(const QString Key)               { return m_Hash[Key]->InJobFile;}

  // Low level access to the underlying QWidget
  QWidget* GetGuiWidget(const QString Key);

  // Constructor
  // The InitLevel determines how much of the .ini information is preserved.
  // If InitLevel > InitLevelOfItem the initialization of that Item is
  // preserved from the .ini.
  // So ptSettings(0) would keep no setting at all. As InitLevelOfItem > 0.
  // If InitLevelOfItem = 9 then the item will never be initialized from
  // the .ini as InitLevel<9 (asserted).
  ptSettings(const short InitLevel, const QString Path);

  // Destructor
  ~ptSettings();

  // Accessors to the Value part of a setting.
  // Selfexplaining.
  int    GetInt(const QString Key) const;
  double GetDouble(const QString Key) const;
  const QString     GetString(const QString Key) const;
  const QStringList GetStringList(const QString Key) const;
  // This should be avoided as much as possible and replaced by above.
  // It is needed though, for instance while writing a job file.
  const QVariant GetValue(const QString Key) const;

  // Setting of value. Implies update for gui element, if one.
  void  SetValue(const QString Key, const QVariant Value);
  // Other selfexplaining settings. Make only sense for gui elements.
  void  SetEnabled(const QString Key, const short Enabled);
  void  SetMaximum(const QString Key, const QVariant Maximum);
  void  Show(const QString Key, const short Show);

  // Some accessors by GuiName
  QString ToolGetName (const QString GuiName) const;
  int ToolAlwaysVisible(const QString GuiName) const;
  int ToolIsActive (const QString GuiName) const;
  int ToolIsBlocked (const QString GuiName) const;
  int ToolIsHidden (const QString GuiName) const;

  // Methods specific for choice (combo) type of gui elements.

  // Keeps Value unique in the choice.
  void  AddOrReplaceOption(const QString  Key,
                           const QString  Text,
                           const QVariant Value);
  void  ClearOptions(const QString Key, const short WithDefault = 0);
  int   GetNrOptions(const QString Key);
  // Value of the combobox at a certain index position.
  const QVariant    GetOptionsValue(const QString Key, const int Index);
  // Current text of the combobox.
  const QString     GetCurrentText(const QString Key);

  // Following are only used by ptMainWindow when inserting
  // the gui elements in the ptMainWindow. They provide an access
  // to one of the underlying gui elements.
  void SetGuiInput(const QString Key, ptInput* Input);
  void SetGuiChoice(const QString Key, ptChoice* Choice);
  void SetGuiCheck(const QString Key, ptCheck* Check);

  // Interface to update the settings to/from dcraw
  void ToDcRaw(ptDcRaw* TheDcRaw);
  void FromDcRaw(ptDcRaw* TheDcRaw);

  // Helper function to decide, if we start with a bitmap or a RAW
  bool useRAWHandling() const;

  // Persistent Settings. Initialization code in ptMain
  // will access it directly.
  QSettings* m_IniSettings;
}; // end class ptSettings

// And we will instantiate one toplevel.
extern ptSettings* Settings;

////////////////////////////////////////////////////////////////////////////////
//
// ptGui*Item
// Initial description (from .i files) of items.
//
////////////////////////////////////////////////////////////////////////////////

// Initial description of a numerical input gui element.
struct ptGuiInputItem {
  QString  KeyName;
  short    GuiType;
  short    InitLevel;
  short    InJobFile;
  short    HasDefaultValue;
  QVariant DefaultValue;
  QVariant MinimumValue;
  QVariant MaximumValue;
  QVariant Step;
  short    NrDecimals;
  QString  Label;
  QString  ToolTip;
};

// Initial description of a choice (combobox) input gui element.
struct ptGuiChoiceItem {
  QString                 KeyName;
  short                   GuiType;
  short                   InitLevel;
  short                   InJobFile;
  short                   HasDefaultValue;
  QVariant                DefaultValue;
  const ptGuiOptionsItem* InitialOptions;
  QString                 ToolTip;
};

// Initial description of a check input gui element.
struct ptGuiCheckItem {
  QString  KeyName;
  short    GuiType;
  short    InitLevel;
  short    InJobFile;
  QVariant DefaultValue;
  QString  Label;
  QString  ToolTip;
};

// Initial description of a simple setting element.
struct ptItem {
  QString  KeyName;
  short    InitLevel;
  QVariant DefaultValue;
  short    InJobFile;
};

#endif

////////////////////////////////////////////////////////////////////////////////
