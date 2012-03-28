/*******************************************************************************
**
** Photivo
**
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
/*!
  \class ptImageSpotListView

  \brief The toolpane ListView for repair spots.

  This class represents the ListView in the toolpane holding the repair spots. It also takes
  care of updating the spot config widgets in the toolpane below the ListView.
*/

#ifndef PTIMAGESPOTLISTVIEW_H
#define PTIMAGESPOTLISTVIEW_H

//==============================================================================

#include <QListView>

#include "ptImageSpot.h"

class ptImageSpotModel;

//==============================================================================

class ptImageSpotListView: public QListView {
  Q_OBJECT

public:
  /*!
   * Creates a \c ptImageSpotListView object.
   * \param AParent
   *    Parent widget of the ListView.
   * \param ASpotCreator
   *    A function pointer to the appropriate factory method for spot creation. Each specialised
   *    spot class has one, called \c CreateSpot().
   */
  explicit ptImageSpotListView(QWidget *AParent,
                               ptImageSpot::PCreateSpotFunc ASpotCreator);
  ~ptImageSpotListView();

  /*! Returns \c true if spot append action is in progress, \c false otherwise. */
  bool appendMode() { return FAppendOngoing; }

  /*! Appends a new spot, selects and focuses it in the list. */
  void setAppendMode(const bool AAppendOngoing);

  /*! Set to true if the related ViewWindow spot interaction starts to ensure that the
   *  preview image gets updated correctly.
   */
  void setInteractionOngoing(const bool AOngoing);

  /*! Reimplemented from base class for convenience to avoid casting to ptImageSpotModel*
   *  all the time.
   */
  void setModel(ptImageSpotModel *model);

  /*! Updates the preview image in the ViewWindow and takes into account if the ViewWindow
   *  interaction is running or not.
   */
  void UpdatePreview();

  void UpdateToolActiveState();

//------------------------------------------------------------------------------

protected:
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  virtual void keyPressEvent(QKeyEvent *event);

//------------------------------------------------------------------------------

private:
  bool                          FAppendOngoing;     // when true, add spot button was clicked
  bool                          FInteractionOngoing;    // ViewWindow interaction
  ptImageSpotModel              *FModel;
  ptImageSpot::PCreateSpotFunc  FSpotCreator;  // pointer to spot factory method

//------------------------------------------------------------------------------

signals:
  /*! QListView does not emit a signal when the list focus changes. So we use our own signal. */
  void rowChanged(const QModelIndex &ANewIdx);

//------------------------------------------------------------------------------

public slots:
  /*! Deletes the selcted spot. This slot gets called when the DEL key or the delete button
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

//------------------------------------------------------------------------------

protected slots:
  /*! [EVENT] Triggered when the focused spot in the list changes. */
  void currentChanged(const QModelIndex &current, const QModelIndex &previous);


};
#endif // PTIMAGESPOTLISTVIEW_H
