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

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

CONFIG    += silent
TEMPLATE   = app
TARGET     = ptClear

DEPENDPATH     += .
INCLUDEPATH    += $${PREFIX}/include
DESTDIR         = ..
OBJECTS_DIR     = ../Objects
MOC_DIR         = ../Objects
UI_HEADERS_DIR  = ../Objects
RCC_DIR         = ../Objects

QMAKE_CXXFLAGS_RELEASE += -O3 -ffast-math $$(CXXFLAGS) -std=gnu++0x
QMAKE_CXXFLAGS_DEBUG +=  -O0 -g -ffast-math $$(CXXFLAGS) -std=gnu++0x

QMAKE_CFLAGS_RELEASE += $$QMAKE_CXXFLAGS_RELEASE
QMAKE_CFLAGS_DEBUG += $$QMAKE_CXXFLAGS_DEBUG

QMAKE_LFLAGS_RELEASE += $$(LDFLAGS) -L$${PREFIX}/lib
QMAKE_LFLAGS_DEBUG += $$QMAKE_LFLAGS_RELEASE

unix {
  QMAKE_CC = ccache /usr/bin/gcc
  QMAKE_CXX = ccache /usr/bin/g++
  QMAKE_LFLAGS_DEBUG += -rdynamic
}

win32 {
  LIBS += -lwsock32 -lexpat -lgdi32
}

macx {
  QMAKE_CC = /usr/bin/gcc
  QMAKE_CXX = /usr/bin/g++
  
  #prevent qmake from adding -arch flags
  QMAKE_CFLAGS_X86_64           = -m64
  QMAKE_CXXFLAGS_X86_64         = -m64 -std=gnu++0x
  QMAKE_OBJECTIVE_CFLAGS_X86_64 = -m64
  QMAKE_LFLAGS_X86_64           = -headerpad_max_install_names
  QMAKE_LFLAGS_DEBUG           += -rdynamic
}

SOURCES += ../Sources/ptClear.cpp
