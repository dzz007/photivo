################################################################################
##
## Photivo
##
## Copyright (C) 2008 Jos De Laender
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

CONFIG += silent
TEMPLATE = app
TARGET = ptCreateCurves

DEPENDPATH     += .
INCLUDEPATH    += $${PREFIX}/include
DESTDIR         = ..
OBJECTS_DIR     = ../Objects
MOC_DIR         = ../Objects
UI_HEADERS_DIR  = ../Objects

#prevent qmake from adding -arch flags
macx{
  QMAKE_CFLAGS_X86_64 =-m64
  QMAKE_CXXFLAGS_X86_64 =-m64
  QMAKE_OBJECTIVE_CFLAGS_X86_64 =-m64
  QMAKE_LFLAGS_X86_64 =-headerpad_max_install_names
}
QMAKE_CXXFLAGS_RELEASE += -O3 $$(CXXFLAGS) -I$${PREFIX}/include -std=gnu++0x
QMAKE_CXXFLAGS_RELEASE += -ffast-math
QMAKE_CXXFLAGS_DEBUG += -O3 $$(CXXFLAGS)  -I$${PREFIX}/include -std=gnu++0x
QMAKE_CXXFLAGS_DEBUG += -ffast-math
QMAKE_CFLAGS_RELEASE += -O3 $$(CFLAGS)  -I$${PREFIX}/include
QMAKE_CFLAGS_RELEASE += -ffast-math
QMAKE_CFLAGS_DEBUG += -O3 $$(CFLAGS)  -I$${PREFIX}/include
QMAKE_CFLAGS_DEBUG += -ffast-math
QMAKE_LFLAGS_DEBUG += $$(LDFLAGS) -L$${PREFIX}/lib
QMAKE_LFLAGS_RELEASE += $$(LDFLAGS) -L$${PREFIX}/lib
LIBS += -ljpeg -llcms2 -lexiv2
win32 {
  QMAKE_CXXFLAGS_DEBUG += $$(CXXFLAGS)
  QMAKE_CXXFLAGS_RELEASE += $$(CXXFLAGS)
  QMAKE_CFLAGS_DEBUG += $$(CFLAGS)
  QMAKE_CFLAGS_RELEASE += $$(CFLAGS)
  QMAKE_LFLAGS_DEBUG += $$(LDFLAGS)
  QMAKE_LFLAGS_RELEASE += $$(LDFLAGS)
  LIBS += -lwsock32 -lexpat
}
macx {
  QMAKE_CC = /usr/bin/gcc
  QMAKE_CXX = /usr/bin/g++
  QMAKE_CXXFLAGS_RELEASE += $$system(pkg-config --cflags lcms2) 
  QMAKE_CXXFLAGS_DEBUG += $$system(pkg-config --cflags lcms2) 
}

# Input
HEADERS += ../Sources/ptCurve.h
HEADERS += ../Sources/ptError.h
HEADERS += ../Sources/ptCalloc.h
SOURCES += ../Sources/ptCreateSomeCurves.cpp
SOURCES += ../Sources/ptCurve.cpp
SOURCES += ../Sources/ptError.cpp
SOURCES += ../Sources/ptCalloc.cpp

###############################################################################
