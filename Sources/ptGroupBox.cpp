/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010-2011 Michael Munzert <mail@mm-log.com>
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

#include <cassert>

#include "ptMessageBox.h"
#include "ptGroupBox.h"
#include "ptSettings.h"
#include "ptConstants.h"
#include "ptTheme.h"
#include "ptCurveWindow.h"
#include "ptViewWindow.h"

extern ptTheme* Theme;
extern QStringList CurveKeys;
extern QString SettingsFilePattern;
extern ptCurve* Curve[17];
extern ptViewWindow* ViewWindow;

//==============================================================================

// Prototypes
void Update(const QString GuiName);
int GetProcessorPhase(const QString GuiName);

//==============================================================================

ptGroupBox::ptGroupBox(const QString Title,
                       QWidget* Parent,
                       const QString Name,
                       const QString TabName,
                       const short TabNumber,
                       const short IndexInTab)
{
  QVBoxLayout *Layout = new QVBoxLayout(this);

  setParent(Parent);

  m_Name = Name;
  setObjectName(Name);
  m_TabName = TabName;
  m_TabNumber = TabNumber;
  m_IndexInTab = IndexInTab;

  RightArrow = QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-folded-normal.png"));
  DownArrow = QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-expanded-normal.png"));
  ActiveRightArrow = QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-folded-active.png"));
  ActiveDownArrow = QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-expanded-active.png"));
  BlockedRightArrow = QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-folded-blocked.png"));
  BlockedDownArrow = QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-expanded-blocked.png"));

  m_Widget = new QWidget();
  m_Widget->setContentsMargins(8,5,0,5);

  m_Header = new QWidget();

  m_Icon = new QLabel();
  m_Icon->setPixmap(DownArrow);


  m_SlowIcon = new QLabel();
  m_SlowIcon->setPixmap(QPixmap(QString::fromUtf8(":/dark/ui-graphics/bubble-attention.png")));
  m_SlowIcon->setToolTip(tr("Complex filter. Might be slow."));
  m_SlowIcon->hide();
  m_SlowIcon->setEnabled(false);

  m_Title = Title;
  if (m_Title.contains("(*)")) {
    m_Title.replace("(*)","");
    m_Title = m_Title.trimmed();
    m_SlowIcon->setEnabled(true);
  }

  m_HelpIcon = new QLabel();
  m_HelpIcon->setPixmap(*Theme->ptIconQuestion);
  m_HelpIcon->setCursor(QCursor(Qt::PointingHandCursor));
  m_HelpIcon->setToolTip(tr("Open help page in web browser."));
  m_HelpIcon->hide();
  m_HelpIcon->setEnabled(false);
  m_HelpUri = "";

  m_TitleLabel = new QLabel();
  m_TitleLabel->setObjectName("Title");
  m_TitleLabel->setText("<b>"+m_Title+"</b>");
  m_TitleLabel->setTextFormat(Qt::RichText);
  m_TitleLabel->setTextInteractionFlags(Qt::NoTextInteraction);

  QHBoxLayout *ButtonLayout = new QHBoxLayout(m_Header);

  ButtonLayout->addWidget(m_Icon);
  ButtonLayout->addWidget(m_TitleLabel);
  ButtonLayout->addStretch();
  ButtonLayout->addWidget(m_HelpIcon);
  ButtonLayout->addWidget(m_SlowIcon);
  ButtonLayout->setContentsMargins(-3,0,3,0);
  ButtonLayout->setSpacing(4);
  ButtonLayout->setMargin(3);

  Layout->addWidget(m_Header);
  Layout->addWidget(m_Widget);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->setMargin(0);
  Layout->setAlignment(Qt::AlignTop);

  m_Folded = Settings->m_IniSettings->value(m_Name,1).toInt();
  m_IsActive = Settings->ToolIsActive(m_Name);
  m_IsBlocked = Settings->ToolIsBlocked(m_Name)?1:0;
  m_IsEnabled = 1;

  m_AtnFav = new QAction(this);
  connect(m_AtnFav, SIGNAL(triggered()), this, SLOT(SetFavourite()));
  m_AtnFav->setIconVisibleInMenu(true);

  m_AtnHide = new QAction(tr("&Hide"), this);
  connect(m_AtnHide, SIGNAL(triggered()), this, SLOT(Hide()));
  m_AtnHide->setIcon(QIcon(*Theme->ptIconCrossRed));
  m_AtnHide->setIconVisibleInMenu(true);

  m_AtnBlock = new QAction(this);
  connect(m_AtnBlock, SIGNAL(triggered()), this, SLOT(SetBlocked()));
  m_AtnBlock->setIcon(QIcon(*Theme->ptIconCircleRed));
  m_AtnBlock->setIconVisibleInMenu(true);

  m_AtnReset = new QAction(tr("&Reset"), this);
  connect(m_AtnReset, SIGNAL(triggered()), this, SLOT(Reset()));
  m_AtnReset->setIcon(QIcon(*Theme->ptIconReset));
  m_AtnReset->setIconVisibleInMenu(true);

  m_AtnSavePreset = new QAction(tr("&Save preset"), this);
  connect(m_AtnSavePreset, SIGNAL(triggered()), this, SLOT(SaveSettings()));
  m_AtnSavePreset->setIcon(QIcon(*Theme->ptIconSavePreset));
  m_AtnSavePreset->setIconVisibleInMenu(true);

  m_AtnAppendPreset = new QAction(tr("&Append preset"), this);
  connect(m_AtnAppendPreset, SIGNAL(triggered()), this, SLOT(AppendSettings()));
  m_AtnAppendPreset->setIcon(QIcon(*Theme->ptIconAppendPreset));
  m_AtnAppendPreset->setIconVisibleInMenu(true);

  UpdateView();

  m_Timer = new QTimer(this);

  connect(m_Timer,SIGNAL(timeout()),
          this,SLOT(PipeUpdate()));

  m_NeedPipeUpdate = 0;
}

//==============================================================================
// Information about position in tabs

QString ptGroupBox::GetTitle() {
  return m_Title;
}

QString ptGroupBox::GetTabName() {
  return m_TabName;
}

short ptGroupBox::GetTabNumber() {
  return m_TabNumber;
}

short ptGroupBox::GetIndexInTab() {
  return m_IndexInTab;
}

//==============================================================================

void ptGroupBox::Update() {
  m_IsBlocked = Settings->ToolIsBlocked(m_Name)?1:0;
  UpdateView();
}

//==============================================================================

void ptGroupBox::UpdateView() {
  if (m_IsBlocked == 1 || m_IsEnabled == 0) {
    if (!(m_TabName == "EyeCandyTab" || m_TabName == "OutTab"))
      m_Widget->setEnabled(0);
  } else {
    m_Widget->setEnabled(1);
  }

  if (m_Folded==1) {
    m_Widget->setVisible(false);
    m_SlowIcon->hide();
    m_HelpIcon->hide();
    m_Icon->clear();
    if (m_IsBlocked) m_Icon->setPixmap(BlockedRightArrow);
    else if (m_IsActive) m_Icon->setPixmap(ActiveRightArrow);
    else m_Icon->setPixmap(RightArrow);
  } else {
    m_Widget->setVisible(true);
    if (m_HelpIcon->isEnabled()) m_HelpIcon->show();
    if (m_SlowIcon->isEnabled()) m_SlowIcon->show();
    m_Icon->clear();
    if (m_IsBlocked) m_Icon->setPixmap(BlockedDownArrow);
    else if (m_IsActive) m_Icon->setPixmap(ActiveDownArrow);
    else m_Icon->setPixmap(DownArrow);
  }

  if (m_Folded!=1) {
    m_Header->setObjectName("ToolHeader");
    m_TitleLabel->setObjectName("ToolHeader");
    m_SlowIcon->setObjectName("ToolHeader");
    m_HelpIcon->setObjectName("ToolHeader");
    m_Icon->setObjectName("ToolHeader");
  } else {
    m_Header->setObjectName("");
    m_TitleLabel->setObjectName("");
    m_SlowIcon->setObjectName("");
    m_HelpIcon->setObjectName("");
    m_Icon->setObjectName("");
  }
  m_Header->setStyleSheet(Theme->ptStyleSheet);
}

//==============================================================================

void ptGroupBox::SetActive(const short IsActive) {
  if (IsActive == m_IsActive)
    return;

  m_IsActive = IsActive;
  UpdateView();
}

//==============================================================================

void ptGroupBox::PipeUpdate() {
  Settings->SetValue("BlockUpdate",0);
  if (m_NeedPipeUpdate == 1) {
    m_NeedPipeUpdate = 0;
    ::Update(m_Name);
  }
}

//==============================================================================

void ptGroupBox::Reset() {
  // immediate response
  SetActive(0);

  m_NeedPipeUpdate = Settings->ToolIsActive(m_Name);

  Settings->SetValue("BlockUpdate",1);
  QList <ptInput *> Inputs = findChildren <ptInput *> ();
  for (int i = 0; i < Inputs.size(); i++) {
    (Inputs.at(i))->Reset();
  }
  QList <ptChoice *> Combos = findChildren <ptChoice *> ();
  for (int i = 0; i < Combos.size(); i++) {
    (Combos.at(i))->Reset();
  }
  QList <ptCheck *> Checks = findChildren <ptCheck *> ();
  for (int i = 0; i < Checks.size(); i++) {
    (Checks.at(i))->Reset();
  }

  m_Timer->start(ptTimeout_Input+100);
}

//==============================================================================

void ptGroupBox::SaveSettings() {
  WriteSettings(0);
}

void ptGroupBox::AppendSettings() {
  WriteSettings(1);
}

void ptGroupBox::WriteSettings(const short Append) {
  QStringList Keys;
  QStringList Curves;
  QList <ptInput *> Inputs = findChildren <ptInput *> ();
  for (int i = 0; i < Inputs.size(); i++) {
    Keys << (Inputs.at(i))->GetName();
  }
  QList <ptChoice *> Combos = findChildren <ptChoice *> ();
  for (int i = 0; i < Combos.size(); i++) {
    Keys << (Combos.at(i))->GetName();
  }
  QList <ptCheck *> Checks = findChildren <ptCheck *> ();
  for (int i = 0; i < Checks.size(); i++) {
    Keys << (Checks.at(i))->GetName();
  }
  QList <ptCurveWindow *> CurveWindows = findChildren <ptCurveWindow *> ();
  for (int i = 0; i < CurveWindows.size(); i++) {
    Curves << CurveKeys.at((CurveWindows.at(i))->m_Channel);
  }

  // Additional for Camera color profile
  if (m_Name == "TabCameraColorSpace")
    Keys << "CameraColorProfile";

  // Additional for Curves
  if (Curves.contains("CurveSaturation"))
    Keys << "SatCurveMode" << "SatCurveType";
  if (Curves.contains("CurveTexture"))
    Keys << "TextureCurveType";
  if (Curves.contains("CurveDenoise"))
    Keys << "DenoiseCurveType";
  if (Curves.contains("CurveDenoise2"))
    Keys << "Denoise2CurveType";
  if (Curves.contains("CurveHue"))
    Keys << "HueCurveType";
  if(m_Name == "TabRGBTone")
    Keys << "Tone1ColorRed" << "Tone1ColorGreen" << "Tone1ColorBlue"
         << "Tone2ColorRed" << "Tone2ColorGreen" << "Tone2ColorBlue";
  if (m_Name == "TabGradualOverlay1")
    Keys << "GradualOverlay1ColorRed" << "GradualOverlay1ColorGreen" << "GradualOverlay1ColorBlue";
  if (m_Name == "TabGradualOverlay2")
    Keys << "GradualOverlay2ColorRed" << "GradualOverlay2ColorGreen" << "GradualOverlay2ColorBlue";

  for (int i = 0; i < Curves.size(); i++) {
    if (Settings->GetInt(Curves.at(i)) > ptCurveChoice_Manual) {
      ptMessageBox::information(0,"Curve problem",
        "Only manual curves are supported in presets.\nNo preset file will be written!");
      return;
    }
  }

  // Additional for Crop
  if (m_Name == "TabCrop" && Settings->GetInt("Crop")==1)
    Keys << "CropX" << "CropY" << "CropW" << "CropH";

  // Additional for texture overlay
  if (m_Name == "TabTextureOverlay")
    Keys << "TextureOverlayFile";

  QString SuggestedFileName = Settings->GetString("PresetDirectory") + "/preset.pts";
  QString FileName;

  FileName = QFileDialog::getSaveFileName(NULL,
                                          tr("Settings File"),
                                          SuggestedFileName,
                                          SettingsFilePattern);

  if (FileName.size() == 0) return;

  QFileInfo PathInfo(FileName);
  Settings->SetValue("PresetDirectory",PathInfo.absolutePath());

  QSettings JobSettings(FileName,QSettings::IniFormat);
  if (Append == 0 ||
      !(JobSettings.value("Magic") == "photivoJobFile" ||
        JobSettings.value("Magic") == "photivoSettingsFile" ||
        JobSettings.value("Magic") == "dlRawJobFile" ||
        JobSettings.value("Magic") == "dlRawSettingsFile" ||
        JobSettings.value("Magic") == "photivoPresetFile")) JobSettings.clear();

  if (JobSettings.value("Magic") == "photivoJobFile" ||
      JobSettings.value("Magic") == "dlRawJobFile")
    JobSettings.setValue("Magic","photivoJobFile");
  else if (JobSettings.value("Magic") == "photivoSettingsFile" ||
      JobSettings.value("Magic") == "dlRawSettingsFile")
    JobSettings.setValue("Magic","photivoSettingsFile");
  else
    JobSettings.setValue("Magic","photivoPresetFile");


  // e.g. a full settings file which should be altered should not get NextPhase
  if (Append == 1 && JobSettings.contains("NextPhase")) {
    JobSettings.setValue("NextPhase",
      MIN(GetProcessorPhase(m_Name),JobSettings.value("NextPhase").toInt()));
  }
  if (Append == 0) {
    JobSettings.setValue("NextPhase",GetProcessorPhase(m_Name));
  }

  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    if (!Settings->GetInJobFile(Key)) continue;
    if (Keys.at(i) == "ChannelMixer") {// set ChannelMixer to manual if needed
      JobSettings.setValue("ChannelMixer",MIN((Settings->GetValue(Key)).toInt(),1));
      continue;
    }
    JobSettings.setValue(Key,Settings->GetValue(Key));
  }

  // save the manual curves
  for (int i = 0; i < Curves.size(); i++) {
    if (Settings->GetInt(Curves.at(i))==ptCurveChoice_Manual) {
      JobSettings.setValue(Curves.at(i) + "Counter",Curve[CurveKeys.indexOf(Curves.at(i))]->m_NrAnchors);
      for (int j = 0; j < Curve[CurveKeys.indexOf(Curves.at(i))]->m_NrAnchors; j++) {
        JobSettings.setValue(Curves.at(i) + "X" + QString::number(j),Curve[CurveKeys.indexOf(Curves.at(i))]->m_XAnchor[j]);
        JobSettings.setValue(Curves.at(i) + "Y" + QString::number(j),Curve[CurveKeys.indexOf(Curves.at(i))]->m_YAnchor[j]);
      }
      JobSettings.setValue(Curves.at(i) + "Type",Curve[CurveKeys.indexOf(Curves.at(i))]->m_IntType);
    }
  }
  JobSettings.sync();
  if (JobSettings.status() != QSettings::NoError)
    ptMessageBox::critical(0,"Error","Error while writing preset file!");
}

//==============================================================================

void ptGroupBox::SetEnabled(const short Enabled) {
  QStringList Temp = Settings->GetStringList("DisabledTools");
  Temp.removeDuplicates();
  if (Enabled == 0) {
    Temp.append(m_Name);
    m_IsEnabled = 0;
  } else {
    Temp.removeOne(m_Name);
    m_IsEnabled = 1;
  }
  Settings->SetValue("DisabledTools",Temp);
  UpdateView();
}

//==============================================================================

void ptGroupBox::SetBlocked() {
  m_IsBlocked = 1 - m_IsBlocked;
  if (!(m_TabName == "EyeCandyTab" || m_TabName == "OutTab"))
    m_Folded = m_IsBlocked;
  UpdateView();
  int Active = Settings->ToolIsActive(m_Name);
  QStringList Temp = Settings->GetStringList("BlockedTools");
  Temp.removeDuplicates();
  if (m_IsBlocked) {
    if (!Temp.contains(m_Name)) Temp.append(m_Name);
  } else {
    Temp.removeOne(m_Name);
  }
  Settings->SetValue("BlockedTools",Temp);
  // processor only needed after RAW since those tools are always visible
  if (Active || Settings->ToolIsActive(m_Name))
    ::Update(m_Name);
}

//==============================================================================

void ptGroupBox::Hide() {
  int Active = Settings->ToolIsActive(m_Name);
  QStringList Temp = Settings->GetStringList("HiddenTools");
  if (!Temp.contains(m_Name)) Temp.append(m_Name);
  Settings->SetValue("HiddenTools",Temp);
  hide();
  // processor only needed after RAW since those tools are always visible
  if (Active) ::Update(m_Name);
}

//==============================================================================

void ptGroupBox::SetFavourite() {
  QStringList Temp = Settings->GetStringList("FavouriteTools");
  if (!Temp.contains(m_Name)) {
    Temp.append(m_Name);
  } else {
    Temp.removeAll(m_Name);
  }
  Settings->SetValue("FavouriteTools", Temp);
}

//==============================================================================

void ptGroupBox::SetHelpUri(const QString Uri) {
  m_HelpUri = Uri;
  m_HelpIcon->setEnabled(true);
  if (!m_Folded)
    m_HelpIcon->show();
}

//==============================================================================

void ptGroupBox::changeEvent(QEvent *) {
}

//==============================================================================

void ptGroupBox::mousePressEvent(QMouseEvent *event) {
  QPoint Position = event->pos()-m_HelpIcon->pos();
  if (Position.x() > 0 && Position.x() < 14 &&
      Position.y() > 0 && Position.y() < 14 &&
      event->button()==Qt::LeftButton &&
      m_HelpIcon->isVisible()) {
    QDesktopServices::openUrl(m_HelpUri);
  } else if (event->y()<20 && event->x()<250) {
    if (event->button()==Qt::LeftButton) {
      //ptMessageBox::critical(0,m_TabName,QString::number(m_IndexInTab+1)+ " in Tab: "+QString::number(m_TabNumber+1));
      m_Folded = 1 - m_Folded;
      UpdateView();
      Settings->m_IniSettings->setValue(m_Name,m_Folded);
    } else if (event->button() == Qt::RightButton &&
               ViewWindow->interaction() != iaCrop &&
               m_IsEnabled == 1) {
      if (!Settings->ToolAlwaysVisible(m_Name)) {
        if (m_IsBlocked == 1) {
          m_AtnBlock->setIcon(QIcon(*Theme->ptIconCircleGreen));
          m_AtnBlock->setText(tr("All&ow"));
        } else {
          m_AtnBlock->setIcon(QIcon(*Theme->ptIconCircleRed));
          m_AtnBlock->setText(tr("Bl&ock"));
        }
        QMenu Menu(NULL);
        Menu.setPalette(Theme->ptMenuPalette);
        Menu.setStyle(Theme->ptStyle);
        Menu.addAction(m_AtnBlock);
        Menu.addSeparator();
        Menu.addAction(m_AtnReset);
        Menu.addSeparator();
        Menu.addAction(m_AtnSavePreset);
        Menu.addAction(m_AtnAppendPreset);
        Menu.addSeparator();
        QStringList Temp = Settings->GetStringList("FavouriteTools");
        if (!Temp.contains(m_Name)) {
          m_AtnFav->setIcon(QIcon(*Theme->ptIconStar));
          m_AtnFav->setText(tr("Add to &favourites"));
        } else {
          m_AtnFav->setIcon(QIcon(*Theme->ptIconStarGrey));
          m_AtnFav->setText(tr("Remove from &favourites"));
        }
        Menu.addAction(m_AtnFav);
        Menu.addSeparator();
        Menu.addAction(m_AtnHide);
        Menu.exec(event->globalPos());
      } else if (m_Name == "TabCameraColorSpace" ||
                 m_Name == "TabWhiteBalance" ||
                 m_Name == "TabDemosaicing" ||
                 m_Name == "TabHighlightRecovery") {
        QMenu Menu(NULL);
        Menu.setPalette(Theme->ptMenuPalette);
        Menu.setStyle(Theme->ptStyle);
        Menu.addAction(m_AtnSavePreset);
        Menu.addAction(m_AtnAppendPreset);
        Menu.addSeparator();
        QStringList Temp = Settings->GetStringList("FavouriteTools");
        if (!Temp.contains(m_Name)) {
          m_AtnFav->setIcon(QIcon(*Theme->ptIconStar));
          m_AtnFav->setText(tr("Add to &favourites"));
        } else {
          m_AtnFav->setIcon(QIcon(*Theme->ptIconStarGrey));
          m_AtnFav->setText(tr("Remove from &favourites"));
        }
        Menu.addAction(m_AtnFav);
        Menu.exec(event->globalPos());
      } else if (parentWidget()->parentWidget()->parentWidget()->parentWidget()->objectName() != "TabSetting" &&
                 parentWidget()->parentWidget()->parentWidget()->parentWidget()->objectName() != "TabInfo") {
        QMenu Menu(NULL);
        Menu.setPalette(Theme->ptMenuPalette);
        Menu.setStyle(Theme->ptStyle);
        QStringList Temp = Settings->GetStringList("FavouriteTools");
        if (!Temp.contains(m_Name)) {
          m_AtnFav->setIcon(QIcon(*Theme->ptIconStar));
          m_AtnFav->setText(tr("Add to &favourites"));
        } else {
          m_AtnFav->setIcon(QIcon(*Theme->ptIconStarGrey));
          m_AtnFav->setText(tr("Remove from &favourites"));
        }
        Menu.addAction(m_AtnFav);
        Menu.exec(event->globalPos());
      }

    }/* else if (event->button()==Qt::RightButton &&
              event->modifiers() == Qt::ControlModifier) {
      if (!Settings->ToolAlwaysVisible(m_Name))
        m_IsBlocked = 1 - m_IsBlocked;
    }*/
  }
}

//==============================================================================

void ptGroupBox::paintEvent(QPaintEvent *)
{}

//==============================================================================
