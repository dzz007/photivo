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

#include <memory>

#include "ptSpotListWidget.h"
#include "ptImageSpotModel.h"
#include "ptImageSpotItemDelegate.h"
#include <ptTheme.h>
#include <ptImage.h>
#include <ptProcessor.h>
#include <ptInfo.h>

extern ptProcessor *TheProcessor;

extern void Update(short Phase,
                   short SubPhase       = -1,
                   short WithIdentify   = 1,
                   short ProcessorMode  = ptProcessorMode_Preview);
extern void UpdatePreviewImage(const ptImage* ForcedImage   = NULL,
                               const short    OnlyHistogram = 0);

//==============================================================================

ptSpotListWidget::ptSpotListWidget(QWidget *AParent)
: QWidget(AParent),
  FAppendOngoing(false),
  FInteractionOngoing(false),
  FSpotCreator(nullptr),
  FSpotList(nullptr)
{
  setupUi(this);
}

//==============================================================================

ptSpotListWidget::~ptSpotListWidget() {
/*
  Resources managed by Qt parent or other objects. Do not delete manually.
    Everything in the .ui
    ListView
    FModel
    FSpotList
*/
}

//==============================================================================

void ptSpotListWidget::init(ptImageSpotList *ASpotList) {
  FSpotList = ASpotList;
  ListView = new ptSpotListView(this);
  ListView->setFixedHeight(this->maximumHeight());
  HorizontalLayout->addWidget(ListView);
  ListView->installEventFilter(this);

  FModel = new ptImageSpotModel(QSize(0, (int)(ListView->fontMetrics().lineSpacing()*1.5)),
                                ASpotList, this);

  ListView->setModel(FModel);
  ListView->setItemDelegate(new ptImageSpotItemDelegate(this));

  connect(ListView,   SIGNAL(rowChanged(QModelIndex)),  this,     SLOT(UpdateButtonStates()));
  connect(ListView,   SIGNAL(rowChanged(QModelIndex)),  this,     SIGNAL(rowChanged(QModelIndex)));
  connect(ListView,   SIGNAL(activeSpotsChanged()),     this,     SLOT(ActiveSpotsChanged()));
  connect(DownButton, SIGNAL(clicked()),                this,     SLOT(moveSpotDown()));
  connect(UpButton,   SIGNAL(clicked()),                this,     SLOT(moveSpotUp()));
  connect(DelButton,  SIGNAL(clicked()),                this,     SLOT(deleteSpot()));
  connect(AddButton,  SIGNAL(clicked()),                this,     SLOT(ToggleAppendMode()));
  connect(EditButton, SIGNAL(clicked()),                this,     SLOT(ToggleEditMode()));

  UpdateButtonStates();
}

//==============================================================================

void ptSpotListWidget::clear() {
  FModel->removeAll();
  UpdateToolActiveState();
  UpdatePreview();
}

//==============================================================================

bool ptSpotListWidget::eventFilter(QObject *watched, QEvent *event) {
  // Filter key events from ListView
  if (!((watched == ListView) && (event->type() == QEvent::KeyPress)))
    return QWidget::eventFilter(watched, event);

  auto hEvent = (QKeyEvent*)event;
  if (hEvent->key() == Qt::Key_Delete && hEvent->modifiers() == Qt::NoModifier) {
    if (ListView->currentIndex().isValid())
      this->deleteSpot();

  } else if (hEvent->key() == Qt::Key_Down && hEvent->modifiers() == Qt::ControlModifier) {
    this->moveSpotDown();

  } else if (hEvent->key() == Qt::Key_Up && hEvent->modifiers() == Qt::ControlModifier) {
    this->moveSpotUp();

  } else {
    return QWidget::eventFilter(watched, event);
  }

  return true;
}

//==============================================================================

void ptSpotListWidget::ToggleAppendMode() {
  // For the add button “checked” means append mode is on, “unchecked” means append mode is off.

  if (FAppendOngoing) {
  // cancel appending a spot
  FAppendOngoing = false;
  AddButton->setToolTip(tr("Append spot mode"));

  } else {
    // start appending a spot
    FAppendOngoing = true;
    AddButton->setToolTip(tr("Exit append spot mode"));

    if (!FInteractionOngoing) {
      // Appending is only possible in editing mode. Start it if it is not already active.
      // First force button enabled because only enabled buttons react to click().
      EditButton->setEnabled(true);
      EditButton->click();
    }
  }
}

//==============================================================================

void ptSpotListWidget::ToggleEditMode() {
  if (FInteractionOngoing) {
    // turn edit mode OFF
    EditButton->setToolTip(tr("Edit spots"));
    if (FAppendOngoing) AddButton->click();
    FInteractionOngoing = false;
    emit editModeChanged(FInteractionOngoing);

  } else {
    // turn edit mode ON
    EditButton->setToolTip(tr("Leave edit mode"));
    FInteractionOngoing = true;
    emit editModeChanged(FInteractionOngoing);

    if (FModel->rowCount() > 0) {
      // Pipe stops *before* spot processing to provide a clean basis for the quick
      // preview in interactive mode. To make present spots immediately visibe when
      // entering the interaction, we trigger spot processing now.
      UpdatePreview();
    }
  }

}

//==============================================================================

void ptSpotListWidget::UpdatePreview() {
  UpdateButtonStates();

  if (FInteractionOngoing) {
    // We’re in interactive mode: only recalc spots
    std::unique_ptr<ptImage> hImage(new ptImage);
    hImage->Set(TheProcessor->m_Image_AfterLocalEdit);
    hImage->RGBToLch();
    FModel->RunFiltering(hImage.get());
    hImage->LchToRGB(Settings->GetInt("WorkColor"));
    UpdatePreviewImage(hImage.get());

  } else {
    // not in interactive mode: run pipe
    Update(ptProcessorPhase_LocalEdit);
  }
}

//==============================================================================

void ptSpotListWidget::UpdateToolActiveState() {
  // Copied from ptMain::Standard_CB_SetAndRun().
  QWidget* CurrentControl = this->parentWidget();
  ptGroupBox* CurrentTool = dynamic_cast<ptGroupBox*>(CurrentControl);

  while (CurrentTool == NULL) {
    CurrentControl = CurrentControl->parentWidget();
    CurrentTool = dynamic_cast<ptGroupBox*>(CurrentControl);
  }

  CurrentTool->SetActive(FModel->hasEnabledSpots());
}

//==============================================================================

void ptSpotListWidget::deleteSpot() {
  FModel->removeRows(ListView->currentIndex().row(), 1, QModelIndex());
  emit rowChanged(ListView->currentIndex());
  UpdatePreview();
}

//==============================================================================

void ptSpotListWidget::moveSpotDown() {
  int hNewRow = FModel->moveRow(ListView->currentIndex(), +1);
  if (hNewRow > -1) {
    ListView->setCurrentIndex(FModel->index(hNewRow, 0));
    UpdatePreview();
  }
}

//==============================================================================

void ptSpotListWidget::moveSpotUp() {
  int hNewRow = FModel->moveRow(ListView->currentIndex(), -1);
  if (hNewRow > -1) {
    ListView->setCurrentIndex(FModel->index(hNewRow, 0));
    UpdatePreview();
  }
}

//==============================================================================

void ptSpotListWidget::processCoordinates(const QPoint &APos) {
  if (FAppendOngoing) {
    // append new spot after append-spot button was clicked
    ptImageSpot *hSpot = FSpotCreator();
    hSpot->setPos(APos.x(), APos.y());
    hSpot->setName(tr("Spot"));

    FModel->appendSpot(hSpot);
    ListView->setCurrentIndex(FModel->index(FModel->rowCount()-1, 0));

  } else if (ListView->currentIndex().isValid()) {
    // move currently selected spot to new position
    FModel->setSpotPos(ListView->currentIndex().row(), APos.x(), APos.y());
  }

  UpdatePreview();
}

//==============================================================================

void ptSpotListWidget::ActiveSpotsChanged() {
  UpdateToolActiveState();
  UpdatePreview();
}

//==============================================================================

void ptSpotListWidget::UpdateButtonStates() {
  bool hIdxValid = ListView->currentIndex().isValid();
  DownButton->setEnabled(hIdxValid && (ListView->currentIndex().row() < FModel->rowCount()-1));
  UpButton->setEnabled(hIdxValid && (ListView->currentIndex().row() > 0));
  DelButton->setEnabled(hIdxValid);
  EditButton->setEnabled(hIdxValid || FAppendOngoing);
}


//===== ptSpotListView =========================================================


ptSpotListView::ptSpotListView(QWidget *AParent)
: QListView(AParent),
  FModel(nullptr)
{
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());

  setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
  setSelectionMode(QAbstractItemView::SingleSelection);
}

//==============================================================================

ptSpotListView::~ptSpotListView() {}

//==============================================================================

void ptSpotListView::setModel(ptImageSpotModel *AModel) {
  FModel = AModel;
  QListView::setModel(AModel);
}

//==============================================================================

void ptSpotListView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QListView::dataChanged(topLeft, bottomRight);
  if (FModel->lastChangedRole() == Qt::CheckStateRole) {
    emit activeSpotsChanged();
  }
}

//==============================================================================

void ptSpotListView::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  QListView::currentChanged(current, previous);
  emit rowChanged(current);
}

//==============================================================================

