/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptToolBox.h"
#include "filters/ptFilterBase.h"
#include "filters/ptFilterDM.h"
#include "ptTheme.h"

#include "ptSettings.h"

#include <QVBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QAction>
#include <QMenu>
#include <QSettings>

//==============================================================================

extern QString SettingsFilePattern;

//==============================================================================

ptToolBox::ptToolBox(QWidget *ABodyWidget, ptFilterBase *AFilter)
: QWidget(nullptr),
  FIsFolded       (true),
  FFilter         (AFilter),
  FBodyWidget     (ABodyWidget),
  // QActions for context menu
  FFavAction      (nullptr),
  FHideAction     (nullptr),
  FBlockAction    (nullptr),
  FResetAction    (nullptr),
  FSaveAction     (nullptr),
  FAppendAction   (nullptr)
{
  FArrows.append({QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-expanded-normal.png")),
                 QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-expanded-active.png")),
                 QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-expanded-blocked.png"))});
  FArrows.append({QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-folded-normal.png")),
                 QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-folded-active.png")),
                 QPixmap(QString::fromUtf8(":/dark/ui-graphics/indicator-folded-blocked.png"))});
  createGui();
  updateGui();
}

//==============================================================================

ptToolBox::~ptToolBox() {
  /*  Resources handled by Qt parent or other objects (do not delete manually):
        FFilter
        all child widgets: FCaption, FHelpIcon, FSlowIcon, FStatusArrow,
                           FBodyWidget, FHeaderWidget
        all QAction members
  */
}

//==============================================================================

bool ptToolBox::eventFilter(QObject* ASender, QEvent* AEvent) {
  if (ASender == FHelpIcon && AEvent->type() == QEvent::MouseButtonRelease) {
    QMouseEvent* e = (QMouseEvent*)AEvent;
    if (e->button() == Qt::LeftButton) {
      QDesktopServices::openUrl(QUrl(FFilter->helpUri()));
      return true;
    } else {
      return false;
    }


  } else if (ASender == FHeaderWidget && AEvent->type() == QEvent::MouseButtonRelease) {
    QMouseEvent* hEvent = (QMouseEvent*)AEvent;
    if (hEvent->button() == Qt::LeftButton && hEvent->modifiers() == Qt::NoModifier) {
      toggleFolded();
      return true;

    } else if (hEvent->button() == Qt::LeftButton && hEvent->modifiers() == Qt::ControlModifier) {
      // toggle block
      createMenuActions();
      FBlockAction->activate(QAction::Trigger);
      return true;

    } else if (hEvent->button() == Qt::RightButton) {
      // context menu
      execContextMenu(hEvent->globalPos());
      return true;

    } else {
      return false;
    }


  } else {
    return QWidget::eventFilter(ASender, AEvent);
  }
}

//==============================================================================

void ptToolBox::createGui() {
  GInfo->Assert(FBodyWidget, "The toolbox cannot have a null body widget.", AT);
  GInfo->Assert(FFilter, "The filter associated with the toolbox cannot be null.", AT);

  this->setObjectName(FFilter->uniqueName());

  FIsFolded = Settings->m_IniSettings->value(this->objectName() + "/Folded", true).toBool();

  FHeaderWidget = new QWidget;
  FHeaderWidget->setContextMenuPolicy(Qt::NoContextMenu);  // event filter handles context menu
  FHeaderWidget->installEventFilter(this);

  // The header contains from left to right:
  // left-justified: FStatusArrow, FCaption; right-justified: FHelpIcon, FSlowIcon
  FStatusArrow = new QLabel;
  connect(FFilter, SIGNAL(activityChanged()), this, SLOT(updateGui()));

  FCaption = new QLabel(FFilter->caption());
  FCaption->setTextFormat(Qt::PlainText);
  FCaption->setTextInteractionFlags(Qt::NoTextInteraction);
  auto hFont = FCaption->font();
  hFont.setBold(true);
  FCaption->setFont(hFont);

  FHelpIcon = new QLabel;
  FHelpIcon->setPixmap(*Theme->ptIconQuestion);
  FHelpIcon->setCursor(QCursor(Qt::PointingHandCursor));
  FHelpIcon->setToolTip(tr("Open help page in web browser."));
  FHelpIcon->hide();
  FHelpIcon->installEventFilter(this);

  FSlowIcon = new QLabel;
  FSlowIcon->setPixmap(QPixmap(QString::fromUtf8(":/dark/ui-graphics/bubble-attention.png")));
  FSlowIcon->setToolTip(tr("Complex filter. Might be slow."));
  FSlowIcon->hide();

  // layout for the header
  auto hHeaderLayout = new QHBoxLayout(FHeaderWidget);
  hHeaderLayout->addWidget(FStatusArrow);
  hHeaderLayout->addWidget(FCaption);
  hHeaderLayout->addStretch();
  hHeaderLayout->addWidget(FHelpIcon);
  hHeaderLayout->addWidget(FSlowIcon);
  hHeaderLayout->setContentsMargins(3,3,3,3);
  hHeaderLayout->setSpacing(4);

  // body widget with the main filter config widgets
  FBodyWidget->setParent(this);
  FBodyWidget->setObjectName("ToolBoxBody");
  FBodyWidget->setVisible(!FIsFolded);

  // Toolbox handles margins and spacing of the body widget’s main layout to ensure consistency.
  auto hBodyLayout = FBodyWidget->layout();
  GInfo->Assert(hBodyLayout, QString("Error! GUI widget of filter \"%1\" does not have a layout.")
                             .arg(FFilter->uniqueName()), AT);
  hBodyLayout->setContentsMargins(8,5,2,5);
  hBodyLayout->setSpacing(4);

  // assemble the toolbox: place header and body in a vertical layout
  auto hBoxLayout = new QVBoxLayout(this);
  hBoxLayout->addWidget(FHeaderWidget);
  hBoxLayout->addWidget(FBodyWidget);
  hBoxLayout->setContentsMargins(0,0,0,0);
  hBoxLayout->setSpacing(0);
  hBoxLayout->setAlignment(Qt::AlignTop);
}

//==============================================================================

void ptToolBox::updateGui() {
  FHeaderWidget->blockSignals(true);

  // set fold/status indicator arrow
  if (FFilter->isBlocked()) {
    FStatusArrow->setPixmap(FArrows[FIsFolded].Blocked);
  } else if (FFilter->isActive()) {
    FStatusArrow->setPixmap(FArrows[FIsFolded].Active);
  } else {
    FStatusArrow->setPixmap(FArrows[FIsFolded].Normal);
  }

  // show help and slow icons only if toolbox is unfolded
  if (FIsFolded) {
    FHeaderWidget->setObjectName("");   // for the Qt stylesheet
    FStatusArrow->setObjectName("");
    FCaption->setObjectName("");
    FHelpIcon->setObjectName("");
    FSlowIcon->setObjectName("");
    FHelpIcon->hide();
    FSlowIcon->hide();
  } else {
    FHeaderWidget->setObjectName("ToolHeader");   // for the Qt stylesheet
    FStatusArrow->setObjectName("ToolHeader");
    FCaption->setObjectName("ToolHeader");
    FHelpIcon->setObjectName("ToolHeader");
    FSlowIcon->setObjectName("ToolHeader");
    if (FFilter->hasHelp()) FHelpIcon->show();
    if (FFilter->isSlow()) FSlowIcon->show();
  }

  FHeaderWidget->setStyleSheet(Theme->stylesheet());
  FHeaderWidget->blockSignals(false);
}

//==============================================================================

void ptToolBox::writePreset(const bool AAppend) {
  GFilterDM->WritePresetFile("", AAppend, false, FFilter);
}

//==============================================================================

void ptToolBox::createMenuActions() {
  if (FBlockAction) return;   // actions already exist.

  FBlockAction = new QAction(this);
  connect(FBlockAction, SIGNAL(triggered()), this, SLOT(toggleBlocked()));
  FBlockAction->setIcon(QIcon(*Theme->ptIconCircleRed));
  FBlockAction->setIconVisibleInMenu(true);

  FResetAction = new QAction(QIcon(*Theme->ptIconReset), tr("&Reset"), this);
  connect(FResetAction, SIGNAL(triggered()), this, SLOT(resetFilter()));
  FResetAction->setIconVisibleInMenu(true);

  FSaveAction = new QAction(QIcon(*Theme->ptIconSavePreset), tr("&Save preset"), this);
  connect(FSaveAction, SIGNAL(triggered()), this, SLOT(savePreset()));
  FSaveAction->setIconVisibleInMenu(true);

  FAppendAction = new QAction(QIcon(*Theme->ptIconAppendPreset), tr("&Append preset"), this);
  connect(FAppendAction, SIGNAL(triggered()), this, SLOT(appendPreset()));
  FAppendAction->setIconVisibleInMenu(true);

  FFavAction = new QAction(this);
  connect(FFavAction, SIGNAL(triggered()), this, SLOT(toggleFavourite()));
  FFavAction->setIconVisibleInMenu(true);

  FHideAction = new QAction(QIcon(*Theme->ptIconCrossRed), tr("&Hide"), this);
  connect(FHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
  FHideAction->setIconVisibleInMenu(true);
}

//==============================================================================

void ptToolBox::execContextMenu(const QPoint APos) {
  createMenuActions();

  // build the menu entries
  QMenu hMenu(nullptr);
  hMenu.setPalette(Theme->menuPalette());
  hMenu.setStyle(Theme->style());

  if (FFilter->flags() & ptFilterBase::FilterIsBlockable) {
    hMenu.addAction(FBlockAction);
    if (FFilter->isBlocked()) {
      FBlockAction->setText(tr("All&ow"));
      FBlockAction->setIcon(QIcon(*Theme->ptIconCircleGreen));
    } else {
      FBlockAction->setText(tr("Bl&ock"));
      FBlockAction->setIcon(QIcon(*Theme->ptIconCircleRed));
    }
  }
  if (FFilter->flags() & ptFilterBase::FilterHasDefault) {
    hMenu.addSeparator();
    hMenu.addAction(FResetAction);
  }
  if (FFilter->flags() & ptFilterBase::FilterIsSaveable) {
    hMenu.addSeparator();
    hMenu.addAction(FSaveAction);
    hMenu.addAction(FAppendAction);
  }
  if (FFilter->flags() & ptFilterBase::FilterIsFavouriteable) {
    hMenu.addSeparator();
    hMenu.addAction(FFavAction);
    if (FFilter->isFavourite()) {
      FFavAction->setText(tr("Remove from &favourites"));
      FFavAction->setIcon(QIcon(*Theme->ptIconStarGrey));
    } else {
      FFavAction->setText(tr("Add to &favourites"));
      FFavAction->setIcon(QIcon(*Theme->ptIconStar));
    }
  }
  if (FFilter->flags() & ptFilterBase::FilterIsHideable) {
    hMenu.addSeparator();
    hMenu.addAction(FHideAction);
  }

  hMenu.exec(APos);
}

//==============================================================================

void ptToolBox::appendPreset() {
  writePreset(true);
}

//==============================================================================

void ptToolBox::savePreset() {
  writePreset(false);
}

//==============================================================================

void ptToolBox::resetFilter() {
  FFilter->reset(true);
}

//==============================================================================

void ptToolBox::toggleBlocked() {
  FFilter->setBlocked(!FFilter->isBlocked());
}

//==============================================================================

void ptToolBox::toggleFavourite() {
  FFilter->setFavourite(!FFilter->isFavourite());
}

//==============================================================================

void ptToolBox::toggleHidden() {
  if (!(FFilter->flags() & ptFilterBase::FilterIsHideable)) {
    GInfo->Warning(QString("Trying to hide unhideable filter \"%1\". Abort.")
                   .arg(FFilter->uniqueName()), AT);
    return;
  }

  this->hide();
  FFilter->setHidden(!FFilter->isHidden());
}

//==============================================================================

void ptToolBox::toggleFolded() {
  FIsFolded = !FIsFolded;
  FBodyWidget->setVisible(!FIsFolded);
  Settings->m_IniSettings->setValue(this->objectName() + "/Folded", (int)FIsFolded);
  updateGui();
}

//==============================================================================
