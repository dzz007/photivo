################################################################################
##
## Photivo
##
## Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
## Copyright (C) 2011 Bernd Schoeler <brother.john@photivo.org>
##
## This file is part of Photivo.
##
## Photivo is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 3
## as published by the Free Software Foundation.
##
## Photivo is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
##
################################################################################
#
# This is a Qt project file for Photivo.
# All Photivo project files are heavily tuned.
# Do not overwrite any with "qmake -project"!
#
################################################################################

#--- Qt configuration ---
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets 

TEMPLATE   = app
TARGET     = ptClear
CONFIG      += silent

DESTDIR = ..

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

QMAKE_CXXFLAGS += $$(CXXFLAGS) -std=gnu++0x
QMAKE_CFLAGS   += $$(CFLAGS)
QMAKE_LFLAGS   += $$(LDFLAGS)

macx {
  #prevent qmake from adding -arch flags
  QMAKE_CFLAGS_X86_64           = -m64
  QMAKE_CXXFLAGS_X86_64         = -m64 -std=gnu++0x
  QMAKE_OBJECTIVE_CFLAGS_X86_64 = -m64
  QMAKE_LFLAGS_X86_64           = -headerpad_max_install_names
  
  LIBS += -framework QtCore -framework QtGui
}

SOURCES += ../Sources/ptClear.cpp
