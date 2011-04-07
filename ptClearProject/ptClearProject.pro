################################################################################
##
## photivo
##
## Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
##
## This file is part of photivo.
##
## photivo is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 3
## as published by the Free Software Foundation.on.
##
## photivo is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with photivo.  If not, see <http://www.gnu.org/licenses/>.
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
TARGET = ptClear
DEPENDPATH += .
win32 {
  BUILDDIR = $$system(cat ../builddir)
  DESTDIR = ../$${BUILDDIR}
  OBJECTS_DIR = ../$${BUILDDIR}/Objects
  MOC_DIR = ../$${BUILDDIR}/Objects
  UI_HEADERS_DIR = ../$${BUILDDIR}/Objects
  RCC_DIR = ../$${BUILDDIR}/Objects
}
unix {
  DESTDIR = ..
  OBJECTS_DIR = ../Objects
  MOC_DIR = ../Objects
  UI_HEADERS_DIR = ../Objects
  RCC_DIR = ../Objects
}
QMAKE_CXXFLAGS_DEBUG += -DDLRAW_HAVE_GIMP
QMAKE_CXXFLAGS_DEBUG += -ffast-math -O0 -g
QMAKE_CXXFLAGS_RELEASE += -O3 -fopenmp
QMAKE_CXXFLAGS_RELEASE += -ffast-math
QMAKE_CXXFLAGS_RELEASE += -DDLRAW_HAVE_GIMP
QMAKE_CFLAGS_DEBUG += -DDLRAW_HAVE_GIMP
QMAKE_CFLAGS_DEBUG += -ffast-math -O0 -g
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CFLAGS_RELEASE += -ffast-math
QMAKE_LFLAGS_RELEASE +=
QMAKE_LFLAGS_DEBUG += -rdynamic
unix {
  QMAKE_CC = ccache /usr/bin/gcc
  QMAKE_CXX = ccache /usr/bin/g++
  QMAKE_POST_LINK=strip $(TARGET)
}
win32 {
  LIBS += -lwsock32 -lexpat -lgdi32
  QMAKE_CXXFLAGS_RELEASE += -I/my/include
  QMAKE_CFLAGS_RELEASE += -I/my/include
  QMAKE_LFLAGS_RELEASE += -L/my/lib
}

# Input
SOURCES += ../Sources/ptClear.cpp


###############################################################################
