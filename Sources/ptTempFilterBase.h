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
  DEPRECATED!
  Common base class for ptGroupBox and ptFilterBase. Strictly for compatibility as long as
  old ptGroupBox and new ptToolBox/ptFilterBase exist in parallel. Will be gone once everything
  is ported.
  Do not rely on this class or use it in any way except for old groupbox related compatibility code.
  You have been warned!
*/

#ifndef PTTEMPFILTERBASE_H
#define PTTEMPFILTERBASE_H

#include <QString>
#include <QWidget>

class ptTempFilterBase: public QWidget
{
Q_OBJECT
public:
  ptTempFilterBase(QWidget * parent = 0, Qt::WindowFlags f = 0): QWidget(parent, f) {}
  virtual int                   idxInParentTab() const = 0;
  virtual int                   parentTabIdx()   const = 0;
  virtual QString               uniqueName() const =0;
  virtual QWidget*              guiWidget() = 0;
  virtual QString               caption() const =0;
  virtual bool                  isActive() const =0;
  virtual bool                  canHide() const =0;
  virtual bool                  isBlocked() const =0;
};

#endif // PTTEMPFILTERBASE_H
