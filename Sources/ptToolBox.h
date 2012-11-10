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

#ifndef PTTOOLBOX_H
#define PTTOOLBOX_H

#include <QWidget>
#include <QLabel>

class ptFilterBase;

//==============================================================================

/*! The \c ptToolBox class is a foldable groupbox widget. */
class ptToolBox: public QWidget {
Q_OBJECT

public:
  /*! Constructs a `ptToolBox` instance. */
  ptToolBox(QWidget *ABodyWidget, ptFilterBase *AFilter);
  ~ptToolBox();


public slots:
  void updateGui();


protected:
  bool eventFilter(QObject *ASender, QEvent *AEvent);


private:
  void createGui();
  void writePreset(const bool AAppend);

  struct TStatusPixmaps {
    QPixmap Normal;
    QPixmap Active;
    QPixmap Blocked;
  };
  QList<TStatusPixmaps> FArrows;    // idx 0 for unfolded, 1 for folded

  bool            FIsFolded;
  ptFilterBase   *FFilter;
  QLabel         *FCaption;
  QLabel         *FHelpIcon;
  QLabel         *FSlowIcon;     // warning for complex/slow filters
  QLabel         *FStatusArrow;
  QWidget        *FBodyWidget;
  QWidget        *FHeaderWidget;

  // Context menu
  void createMenuActions();
  void execContextMenu(const QPoint APos);
  QAction  *FFavAction;
  QAction  *FHideAction;
  QAction  *FBlockAction;
  QAction  *FResetAction;
  QAction  *FSaveAction;
  QAction  *FAppendAction;


private slots:
  // Context menu slots
  void appendPreset();
  void savePreset();
  void resetFilter();
  void toggleBlocked();
  void toggleFavourite();
  void toggleHidden();
  void toggleFolded();

};
#endif // PTTOOLBOX_H
