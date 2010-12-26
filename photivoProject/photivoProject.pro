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
#CONFIG += debug
TEMPLATE = app
TARGET = photivo
DEPENDPATH += .
DESTDIR = ..
OBJECTS_DIR = ../Objects
MOC_DIR = ../Objects
UI_HEADERS_DIR = ../Objects
RCC_DIR = ../Objects
QMAKE_CXXFLAGS_DEBUG += -DDLRAW_HAVE_GIMP
QMAKE_CXXFLAGS_DEBUG += -ffast-math -O0 -g
QMAKE_CXXFLAGS_RELEASE += -O3 -fopenmp
QMAKE_CXXFLAGS_RELEASE += -ffast-math
QMAKE_CXXFLAGS_RELEASE += -DDLRAW_HAVE_GIMP
QMAKE_CFLAGS_DEBUG += -DDLRAW_HAVE_GIMP
QMAKE_CFLAGS_DEBUG += -ffast-math -O0 -g
QMAKE_CFLAGS_RELEASE += -O3 -fopenmp
QMAKE_CFLAGS_RELEASE += -ffast-math
QMAKE_CFLAGS_RELEASE += -DDLRAW_HAVE_GIMP
QMAKE_LFLAGS_RELEASE += -fopenmp
QMAKE_LFLAGS_DEBUG += -rdynamic
LIBS += -ljpeg -llcms2 -lexiv2 -lfftw3
# LIBS += -lMagick++ -lMagickWand -lMagickCore
LIBS += -llensfun
LIBS += -lgomp -lpthread
unix {
  CONFIG += link_pkgconfig
  PKGCONFIG += GraphicsMagick++ GraphicsMagickWand
  LIBS += $$system(GraphicsMagick++-config --libs)
  QMAKE_CC = ccache /usr/bin/gcc
  QMAKE_CXX = ccache /usr/bin/g++
  PREFIX = $$system(more ./config)
  QMAKE_CXXFLAGS_DEBUG += -DPREFIX=$${PREFIX}
  QMAKE_CXXFLAGS_RELEASE += -DPREFIX=$${PREFIX}
  QMAKE_CFLAGS_DEBUG += -DPREFIX=$${PREFIX}
  QMAKE_CFLAGS_RELEASE += -DPREFIX=$${PREFIX}
  QMAKE_POST_LINK=strip $(TARGET)
}
win32 {
  LIBS += -lGraphicsMagick++ -lGraphicsMagickWand -lGraphicsMagick
  LIBS += -lwsock32 -lexpat -lregex -lgdi32 -liconv
  INCLUDEPATH += /mingw/include/GraphicsMagick
  RC_FILE = photivo.rc
#  CONFIG += console
}

# Input
HEADERS += ../Sources/ptConstants.h
HEADERS += ../Sources/ptCurve.h
HEADERS += ../Sources/ptChannelMixer.h
HEADERS += ../Sources/ptDcRaw.h
HEADERS += ../Sources/ptDefines.h
HEADERS += ../Sources/ptError.h
HEADERS += ../Sources/ptGuiOptions.h
HEADERS += ../Sources/ptSettings.h
HEADERS += ../Sources/ptImage.h
HEADERS += ../Sources/ptImage8.h
HEADERS += ../Sources/ptResizeFilters.h
HEADERS += ../Sources/ptKernel.h
HEADERS += ../Sources/ptMainWindow.h
HEADERS += ../Sources/ptRGBTemperature.h
HEADERS += ../Sources/ptWhiteBalances.h
HEADERS += ../Sources/ptCurveWindow.h
HEADERS += ../Sources/ptHistogramWindow.h
HEADERS += ../Sources/ptAdobeTable.h
HEADERS += ../Sources/ptRefocusMatrix.h
HEADERS += ../Sources/ptViewWindow.h
HEADERS += ../Sources/ptLensfun.h
HEADERS += ../Sources/ptProcessor.h
HEADERS += ../Sources/ptCimg.h
HEADERS += ../Sources/ptFastBilateral.h
HEADERS += ../Sources/ptWiener.h
# HEADERS += ../Sources/ptImageMagick.h
# HEADERS += ../Sources/ptImageMagickC.h
HEADERS += ../Sources/ptInput.h
HEADERS += ../Sources/ptChoice.h
HEADERS += ../Sources/ptCheck.h
HEADERS += ../Sources/ptCalloc.h
HEADERS += ../Sources/ptGroupBox.h
HEADERS += ../Sources/ptTheme.h
HEADERS += ../Sources/greyc/CImg.h
HEADERS += ../Sources/clapack/clapack.h
HEADERS += ../Sources/clapack/fp.h
HEADERS += ../Sources/clapack/fmt.h
HEADERS += ../Sources/clapack/fio.h
HEADERS += ../Sources/clapack/blaswrap.h
HEADERS += ../Sources/fastbilateral/array.h
HEADERS += ../Sources/fastbilateral/fast_lbf.h
HEADERS += ../Sources/fastbilateral/math_tools.h
HEADERS += ../Sources/fastbilateral/mixed_vector.h
FORMS +=   ../Sources/ptMainWindow.ui
SOURCES += ../Sources/ptCurve.cpp
SOURCES += ../Sources/ptChannelMixer.cpp
SOURCES += ../Sources/ptDcRaw.cpp
SOURCES += ../Sources/ptError.cpp
SOURCES += ../Sources/ptGuiOptions.cpp
SOURCES += ../Sources/ptSettings.cpp
SOURCES += ../Sources/ptImage.cpp
SOURCES += ../Sources/ptImage_DRC.cpp
SOURCES += ../Sources/ptImage_EAW.cpp
SOURCES += ../Sources/ptImage_GM.cpp
SOURCES += ../Sources/ptImage_GMC.cpp
SOURCES += ../Sources/ptImage_Pyramid.cpp
SOURCES += ../Sources/ptImage8.cpp
SOURCES += ../Sources/ptResizeFilters.cpp
SOURCES += ../Sources/ptKernel.cpp
SOURCES += ../Sources/ptMain.cpp
SOURCES += ../Sources/ptMainWindow.cpp
SOURCES += ../Sources/ptRGBTemperature.cpp
SOURCES += ../Sources/ptWhiteBalances.cpp
SOURCES += ../Sources/ptCurveWindow.cpp
SOURCES += ../Sources/ptHistogramWindow.cpp
SOURCES += ../Sources/ptRefocusMatrix.cpp
SOURCES += ../Sources/ptViewWindow.cpp
SOURCES += ../Sources/ptLensfun.cpp
SOURCES += ../Sources/ptProcessor.cpp
SOURCES += ../Sources/ptCimg.cpp
SOURCES += ../Sources/ptFastBilateral.cpp
SOURCES += ../Sources/ptWiener.cpp
# SOURCES += ../Sources/ptImageMagick.cpp
# SOURCES += ../Sources/ptImageMagickC.cpp
SOURCES += ../Sources/ptInput.cpp
SOURCES += ../Sources/ptChoice.cpp
SOURCES += ../Sources/ptCheck.cpp
SOURCES += ../Sources/ptCalloc.cpp
SOURCES += ../Sources/ptGroupBox.cpp
SOURCES += ../Sources/ptTheme.cpp
SOURCES += ../Sources/clapack/xerbla.c
SOURCES += ../Sources/clapack/open.c
SOURCES += ../Sources/clapack/ieeeck.c
SOURCES += ../Sources/clapack/dtrsm.c
SOURCES += ../Sources/clapack/util.c
SOURCES += ../Sources/clapack/wsfe.c
SOURCES += ../Sources/clapack/s_cmp.c
SOURCES += ../Sources/clapack/dgetrs.c
SOURCES += ../Sources/clapack/ptaswp.c
SOURCES += ../Sources/clapack/ilaenv.c
SOURCES += ../Sources/clapack/s_stop.c
SOURCES += ../Sources/clapack/dgemm.c
SOURCES += ../Sources/clapack/s_copy.c
SOURCES += ../Sources/clapack/abort_.c
SOURCES += ../Sources/clapack/dgesv.c
SOURCES += ../Sources/clapack/dgetrf.c
SOURCES += ../Sources/clapack/fmtlib.c
SOURCES += ../Sources/clapack/sig_die.c
SOURCES += ../Sources/clapack/idamax.c
SOURCES += ../Sources/clapack/close.c
SOURCES += ../Sources/clapack/dger.c
SOURCES += ../Sources/clapack/lsame.c
SOURCES += ../Sources/clapack/dscal.c
SOURCES += ../Sources/clapack/fmt.c
SOURCES += ../Sources/clapack/dswap.c
SOURCES += ../Sources/clapack/endfile.c
SOURCES += ../Sources/clapack/wref.c
SOURCES += ../Sources/clapack/dgetf2.c
SOURCES += ../Sources/clapack/err.c
SOURCES += ../Sources/clapack/wrtfmt.c
SOURCES += ../Sources/clapack/sfe.c
SOURCES += ../Sources/dcb/dcb_demosaicing.c
SOURCES += ../Sources/dcb/dcb_demosaicing_old.c
SOURCES += ../Sources/vcd/ahd_interpolate_mod.c
SOURCES += ../Sources/vcd/ahd_partial_interpolate.c
SOURCES += ../Sources/vcd/es_median_filter.c
SOURCES += ../Sources/vcd/median_filter_new.c
SOURCES += ../Sources/vcd/refinement.c
SOURCES += ../Sources/vcd/vcd_interpolate.c
SOURCES += ../Sources/perfectraw/lmmse_interpolate.c
SOURCES += ../Sources/rawtherapee/amaze_interpolate.c
SOURCES += ../Sources/rawtherapee/cfa_line_dn.c
SOURCES += ../Sources/rawtherapee/ca_correct.c
SOURCES += ../Sources/rawtherapee/green_equil.c
RESOURCES = ../photivo.qrc
TRANSLATIONS += ../Translations/photivo_de.ts

###############################################################################
