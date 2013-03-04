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
/*!
    \class ptWidget
    \brief Abstract base class for Photivo’s customised widgets.
 */

#ifndef PTWIDGET_H
#define PTWIDGET_H

//==============================================================================

#include <QWidget>
#include <QVariant>

class ptCfgItem;

//==============================================================================

class ptWidget: public QWidget {
Q_OBJECT

public:
  /*! Creates a ptWidget instance.
      The ctor with a \c QWidget* as the sole parameter is necessary for Qt Designer support.
  */
  explicit ptWidget(QWidget *AParent);

  /*! Initializes a ptWidget.
      Because of the ctor signature required by Qt Designer this cannot be intergrated into
      the ctor.
  */
  virtual void init(const ptCfgItem &ACfgItem) = 0;

  /* NOTE: The old custom widgets have an additional parameter "const short BlockSignal = 1".
     That parameter was left out in the new-style setValue() because it is meaningless.
     Setting a widget’s value programmatically via the resp. setXXX() functions never
     triggers signals. If simulating user interaction is necessary (to trigger a signal)
     there’s usually a function click() or similar to do that.
  */
  virtual void setValue(const QVariant &AValue) = 0;
  

protected:
  /*! Flag that determines if a ptWidget is used within the new filter/GUI structure around
      ptFilterBase. Only important for the old-style custom widgets while they are
      used in both the old and new system. This flag will disappear once the transition to the
      new system is complete.

      THIS FLAG MUST NEVER BE USED ANYWHERE EXCEPT IN THE OLD ptCheck, ptChoice and ptInput.
  */
  bool FIsNewSchool;


signals:
  void valueChanged(const QString AId, const QVariant ANewValue);
  
};

#endif // PTWIDGET_H
