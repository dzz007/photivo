################################################################################
##
## photivo
##
## Copyright (C) 2008 Jos De Laender
##
## This file is part of photivo.
##
## photivo is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 3
## as published by the Free Software Foundation.
##
## photivo is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with photivo.  If not, see <http:##www.gnu.org#licenses#>.
##
################################################################################

######################################################################
#
# This is the Qt project file for photivo.
# Don't let it overwrite by qmake -project !
# A number of settings is tuned.
#
# qmake will make a platform dependent makefile of it.
#
######################################################################

CONFIG += release silent
TEMPLATE = app
TARGET = ptCreateCurves
DEPENDPATH += .
INCLUDEPATH += .
win32 {
  BUILDDIR = $$system(cat ../builddir)
  DESTDIR = ../$${BUILDDIR}
  OBJECTS_DIR = ../$${BUILDDIR}/Objects
  MOC_DIR = ../$${BUILDDIR}/Objects
  UI_HEADERS_DIR = ../$${BUILDDIR}/Objects
}
unix {
  DESTDIR = ..
  OBJECTS_DIR = ../Objects
  MOC_DIR = ../Objects
  UI_HEADERS_DIR = ../Objects
}
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -ffast-math
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CFLAGS_RELEASE += -ffast-math
LIBS += -ljpeg -llcms2 -lexiv2
win32 {
  LIBS += -lwsock32 -lexpat
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
