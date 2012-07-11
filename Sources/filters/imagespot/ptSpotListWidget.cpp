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

#include <QKeyEvent>

#include "ptSpotListWidget.h"
#include "ptImageSpotModel.h"
#include "ptImageSpotItemDelegate.h"
#include <ptTheme.h>
#include <ptImage.h>
#include <ptInfo.h>


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

void ptSpotListWidget::init(ptImageSpotList *ASpotList, PCreateSpotFunc ASpotCreator) {
  FSpotList = ASpotList;
  FSpotCreator = ASpotCreator;
  ListView = new ptSpotListView(this);
  ListView->setFixedHeight(this->maximumHeight());
  HorizontalLayout->addWidget(ListView);
  ListView->installEventFilter(this);

  FModel = new ptImageSpotModel(QSize(0, (int)(ListView->fontMetrics().lineSpacing()*1.5)),
                                FSpotList, this);

  ListView->setModel(FModel);
  ListView->setItemDelegate(new ptImageSpotItemDelegate(this));

  connect(ListView,   SIGNAL(rowChanged(QModelIndex)),  this,     SLOT(updateButtonStates()));
  connect(ListView,   SIGNAL(rowChanged(QModelIndex)),  this,     SLOT(emitRowChanged(QModelIndex)));
  connect(ListView,   SIGNAL(activeSpotsChanged()),     this,     SLOT(activeSpotsChanged()));
  connect(DownButton, SIGNAL(clicked()),                this,     SLOT(moveSpotDown()));
  connect(UpButton,   SIGNAL(clicked()),                this,     SLOT(moveSpotUp()));
  connect(DelButton,  SIGNAL(clicked()),                this,     SLOT(deleteSpot()));
  connect(AddButton,  SIGNAL(clicked()),                this,     SLOT(toggleAppendMode()));
  connect(EditButton, SIGNAL(clicked()),                this,     SLOT(toggleEditMode()));

  updateButtonStates();
}

//==============================================================================

void ptSpotListWidget::setEditMode(bool AIsEdit) {
  EditButton->setChecked(AIsEdit);
  if (!AIsEdit)
    FAppendOngoing = false;
  updateButtonStates();
}

//==============================================================================

void ptSpotListWidget::clear() {
  FModel->removeAll();
  updatePreview();
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

void ptSpotListWidget::toggleAppendMode() {
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

void ptSpotListWidget::toggleEditMode() {
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
      updatePreview();
    }
  }

}

//==============================================================================

void ptSpotListWidget::deleteSpot() {
  FModel->removeRows(ListView->currentIndex().row(), 1, QModelIndex());
  emit rowChanged(ListView->currentIndex().row());
  updatePreview();
}

//==============================================================================

void ptSpotListWidget::moveSpotDown() {
  int hNewRow = FModel->moveRow(ListView->currentIndex(), +1);
  if (hNewRow > -1) {
    ListView->setCurrentIndex(FModel->index(hNewRow, 0));
    updatePreview();
  }
}

//==============================================================================

void ptSpotListWidget::moveSpotUp() {
  int hNewRow = FModel->moveRow(ListView->currentIndex(), -1);
  if (hNewRow > -1) {
    ListView->setCurrentIndex(FModel->index(hNewRow, 0));
    updatePreview();
  }
}

//==============================================================================

void ptSpotListWidget::processCoordinates(const QPoint &APos) {
  if (FAppendOngoing) {
    // append new spot after append-spot button was clicked
    ptImageSpot *hSpot = FSpotCreator();
    hSpot->setPos(APos);
    hSpot->setName(tr("Spot"));

    FModel->appendSpot(hSpot);
    ListView->setCurrentIndex(FModel->index(FModel->rowCount()-1, 0));

  } else if (ListView->currentIndex().isValid()) {
    // move currently selected spot to new position
    FModel->setSpotPos(ListView->currentIndex().row(), APos.x(), APos.y());
  }

  updatePreview();
}

//==============================================================================

void ptSpotListWidget::updatePreview() {
  emit dataChanged();
}

//==============================================================================

void ptSpotListWidget::emitRowChanged(const QModelIndex &AIdx) {
  emit rowChanged(AIdx.row());
}

//==============================================================================

void ptSpotListWidget::activeSpotsChanged() {
  updatePreview();
}

//==============================================================================

void ptSpotListWidget::updateButtonStates() {
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

