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

#ifndef PTSPOTLISTWIDGET_H
#define PTSPOTLISTWIDGET_H

//==============================================================================

#include <QString>
#include <QListView>

#include "ptImageSpot.h"
#include "ptSpotListWidgetHelper.h"
#include "ui_ptSpotListWidget.h"

class ptImageSpotModel;
class ptImageSpotList;
class ptSpotListView;

//==============================================================================

/*!
  \class ptSpotListWidget
  \brief \c ptSpotListWidget encapsulates the ListView and buttons for a spot list.
 */
class ptSpotListWidget: public QWidget, private Ui::ptSpotListWidget {
Q_OBJECT
  
public:
  explicit ptSpotListWidget(QWidget *AParent);
  ~ptSpotListWidget();

  void init(ptImageSpotList *ASpotList);

  /*! Returns \c true if spot append action is in progress, \c false otherwise. */
  bool appendMode() { return FAppendOngoing; }

  ptImageSpotModel* model() { return FModel; }


  /*! Appends a new spot, selects and focuses it in the list. */
  void setAppendMode(const bool AAppendOngoing) { FAppendOngoing = AAppendOngoing; }

  /*! Deletes all spots from the ListView and model. */
  void clear();

  /*! Reimplemented from base class. */
  virtual bool eventFilter(QObject *watched, QEvent *event);

  /*! Updates the preview image in the ViewWindow and takes into account if the ViewWindow
      interaction is running or not.
   */
  void UpdatePreview();


public slots:
  /*! Deletes the selected spot. This slot gets called when the DEL key or the delete button
      of a spot is pressed. */
  void deleteSpot();


  /*! Moves the selected spot one position down or up in the list.
      The selection stays at that spot. */
  ///@{
  void moveSpotDown();
  void moveSpotUp();
  ///@}

  /*! This slot gets called when the user selects a position on the image. */
  void processCoordinates(const QPoint &APos);


private:
  bool                          FAppendOngoing;     // when true, “add spot” button was clicked
  ptSpotListView               *ListView;
  ptImageSpotModel             *FModel;
  bool                          FInteractionOngoing;    // ViewWindow interaction
  ptImageSpot::PCreateSpotFunc  FSpotCreator;  // pointer to spot factory method
  ptImageSpotList              *FSpotList;


private slots:
  void ActiveSpotsChanged();
  void UpdateButtonStates();
  void UpdateToolActiveState();
  void ToggleAppendMode();
  void ToggleEditMode();


signals:
  void editModeChanged(const bool AEditEnabled);
  void rowChanged(const QModelIndex &ANewIdx);

};


//===== ptSpotListView =========================================================


/*!
   \brief The \c ptImageSpotListView class is a small helper class for \c ptSpotListWidget
    that adds some small details to \c QListView.
 */
class ptSpotListView: public QListView {
Q_OBJECT
public:
  explicit ptSpotListView(QWidget *AParent);
  ~ptSpotListView();

  void setModel(ptImageSpotModel *AModel);


protected:
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);


protected slots:
  /*! Qt event triggered when the focused spot in the list changes. */
  void currentChanged(const QModelIndex &current, const QModelIndex &previous);


private:
  ptImageSpotModel *FModel;


signals:
  /*! \c QListView does not emit a signal when the focused row changes. So we use our own signal. */
  void rowChanged(const QModelIndex &ANewIdx);

  /*! Emitted when a spot is enabled or disabled. */
  void activeSpotsChanged();

};

#endif // PTSPOTLISTWIDGET_H
