################################################################################
##
## photivo
##
## Copyright (C) 2008,2009 Jos De Laender
## Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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
TARGET = ptGimp
DEPENDPATH += .
DESTDIR = ..
OBJECTS_DIR = ../Objects_Gimp
MOC_DIR = ../Objects_Gimp
UI_HEADERS_DIR = ../Objects_Gimp
RCC_DIR = ../Objects_Gimp
# Bit funky for the glib. And gimp
QMAKE_CXXFLAGS_RELEASE += $$system(pkg-config --cflags-only-I glib-2.0)
QMAKE_CXXFLAGS_RELEASE += $$system(pkg-config --cflags-only-I gimp-2.0)
QMAKE_CXXFLAGS_RELEASE += $$system(pkg-config --cflags-only-I gtk+-2.0)
QMAKE_CXXFLAGS_RELEASE += -DDLRAW_GIMP_PLUGIN
QMAKE_CXXFLAGS_DEBUG += $$system(pkg-config --cflags-only-I glib-2.0)
QMAKE_CXXFLAGS_DEBUG += $$system(pkg-config --cflags-only-I gimp-2.0)
QMAKE_CXXFLAGS_DEBUG += $$system(pkg-config --cflags-only-I gtk+-2.0)
QMAKE_CXXFLAGS_DEBUG += -DDLRAW_GIMP_PLUGIN
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -ffast-math
QMAKE_CFLAGS_RELEASE += $$system(pkg-config --cflags-only-I glib-2.0)
QMAKE_CFLAGS_RELEASE += $$system(pkg-config --cflags-only-I gimp-2.0)
QMAKE_CFLAGS_RELEASE += $$system(pkg-config --cflags-only-I gtk+-2.0)
QMAKE_CFLAGS_RELEASE += -DDLRAW_GIMP_PLUGIN
QMAKE_CFLAGS_DEBUG += $$system(pkg-config --cflags-only-I glib-2.0)
QMAKE_CFLAGS_DEBUG += $$system(pkg-config --cflags-only-I gimp-2.0)
QMAKE_CFLAGS_DEBUG += $$system(pkg-config --cflags-only-I gtk+-2.0)
QMAKE_CFLAGS_DEBUG += -DDLRAW_GIMP_PLUGIN
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CFLAGS_RELEASE += -ffast-math
QMAKE_LFLAGS_RELEASE +=
QMAKE_LFLAGS_DEBUG += -rdynamic
LIBS += $$system(pkg-config --libs-only-l glib-2.0)
LIBS += $$system(pkg-config --libs-only-l gimp-2.0)
LIBS += $$system(pkg-config --libs-only-l gtk+-2.0)
unix {
  QMAKE_CC = ccache /usr/bin/gcc
  QMAKE_CXX = ccache /usr/bin/g++
}
win32 {
  LIBS += -lwsock32 -lexpat -lregex -lgdi32 -lgimp
}

# Input
HEADERS += ../Sources/ptCalloc.h
HEADERS += ../Sources/ptDefines.h
HEADERS += ../Sources/ptError.h
SOURCES += ../Sources/ptCalloc.cpp
SOURCES += ../Sources/ptError.cpp
SOURCES += ../Sources/ptGimp.cpp


###############################################################################
