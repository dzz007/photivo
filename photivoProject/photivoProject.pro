################################################################################
##
## photivo
##
## Copyright (C) 2008,2009 Jos De Laender
## Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
## Copyright (C) 2011 Bernd Schoeler <brother.john@photivo.org>
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

CONFIG += silent
#CONFIG += release
#CONFIG += debug
TEMPLATE = app
TARGET = photivo
DEPENDPATH += .
INCLUDEPATH += $${PREFIX}/include

unix {
  DESTDIR = ..
  OBJECTS_DIR = ../Objects
  MOC_DIR = ../Objects
  UI_HEADERS_DIR = ../Objects
  RCC_DIR = ../Objects
}
win32 {
  BUILDDIR = $$system(cat ../builddir)
  DESTDIR = ../$${BUILDDIR}
  OBJECTS_DIR = ../$${BUILDDIR}/Objects
  MOC_DIR = ../$${BUILDDIR}/Objects
  UI_HEADERS_DIR = ../$${BUILDDIR}/Objects
  RCC_DIR = ../$${BUILDDIR}/Objects
}
#prevent qmake from adding -arch flags
macx{
  QMAKE_CFLAGS_X86_64 =-m64
  QMAKE_CXXFLAGS_X86_64 =-m64
  QMAKE_OBJECTIVE_CFLAGS_X86_64 =-m64
  QMAKE_LFLAGS_X86_64 =-headerpad_max_install_names
}

# Stuff for liquid rescale
QMAKE_CFLAGS_RELEASE += $$system(pkg-config --cflags-only-I lqr-1)
QMAKE_CFLAGS_DEBUG += $$system(pkg-config --cflags-only-I lqr-1)
QMAKE_CXXFLAGS_RELEASE += $$system(pkg-config --cflags-only-I lqr-1)
QMAKE_CXXFLAGS_DEBUG += $$system(pkg-config --cflags-only-I lqr-1)
LIBS += $$system(pkg-config --libs-only-l lqr-1)

QMAKE_CXXFLAGS_DEBUG += -DDLRAW_HAVE_GIMP -ffast-math -O0 -g
QMAKE_CXXFLAGS_RELEASE += -O3 -ftree-vectorize -fopenmp -ffast-math -DDLRAW_HAVE_GIMP
QMAKE_CFLAGS_DEBUG += -ffast-math -O0 -g -DDLRAW_HAVE_GIMP
QMAKE_CFLAGS_RELEASE += -O3 -ftree-vectorize -fopenmp -ffast-math -DDLRAW_HAVE_GIMP -fopenmp
QMAKE_LFLAGS_DEBUG += -rdynamic

APPVERSION = $$system(sh ./get_appversion)
!build_pass:message("Photivo $${APPVERSION}")
QMAKE_CXXFLAGS_DEBUG += -DAPPVERSION=\'$${APPVERSION}\'
QMAKE_CXXFLAGS_RELEASE += -DAPPVERSION=\'$${APPVERSION}\'
QMAKE_CFLAGS_DEBUG += -DAPPVERSION=\'$${APPVERSION}\'
QMAKE_CFLAGS_RELEASE += -DAPPVERSION=\'$${APPVERSION}\'

LIBS += -ljpeg -llcms2 -lexiv2 -lfftw3 -llensfun -lgomp -lpthread

unix {
  CONFIG += link_pkgconfig
  PKGCONFIG += GraphicsMagick++ GraphicsMagickWand
  LIBS += $$system(GraphicsMagick++-config --libs)
  QMAKE_CC = ccache /usr/bin/gcc
  QMAKE_CXX = ccache /usr/bin/g++
  PREFIX = $$system(cat ./install_prefix)
  QMAKE_CXXFLAGS_DEBUG += -DPREFIX=$${PREFIX} -I$${PREFIX}/include $$(CXXFLAGS)
  QMAKE_CXXFLAGS_RELEASE += -DPREFIX=$${PREFIX} -I$${PREFIX}/include $$(CXXFLAGS)
  QMAKE_CFLAGS_DEBUG += -DPREFIX=$${PREFIX} -L$${PREFIX}/lib $$(CFLAGS)
  QMAKE_CFLAGS_RELEASE += -DPREFIX=$${PREFIX} -L$${PREFIX}/lib $$(CFLAGS)
  #QMAKE_POST_LINK=strip $(TARGET)
  QT += network
}
win32 {
  QMAKE_CXXFLAGS_DEBUG += $$(CXXFLAGS)
  QMAKE_CXXFLAGS_RELEASE += $$(CXXFLAGS)
  QMAKE_CFLAGS_DEBUG += $$(CFLAGS)
  QMAKE_CFLAGS_RELEASE += $$(CFLAGS)
  QMAKE_LFLAGS_DEBUG += $$(LDFLAGS)
  QMAKE_LFLAGS_RELEASE += $$(LDFLAGS)
  LIBS += -lGraphicsMagick++ -lGraphicsMagickWand -lGraphicsMagick
  LIBS += -lwsock32 -lexpat -lregex -lgdi32 -liconv
  RC_FILE = photivo.rc
  #CONFIG += console
  QT += network
}
macx {
  PKGCONFIG += lcms2
  LIBS += $$system(pkg-config --libs lcms2)
  QMAKE_CC = /usr/bin/gcc
  QMAKE_CXX = /usr/bin/g++
  LIBS += -framework QtCore
  LIBS += -framework QtGui
  LIBS += -framework QtNetwork
}

# Input
HEADERS += ../Sources/ptAdobeTable.h \
    ../Sources/ptReportOverlay.h \
    ../Sources/ptAbstractInteraction.h \
    ../Sources/ptLineInteraction.h \
    ../Sources/ptSimpleRectInteraction.h \
    ../Sources/ptRichRectInteraction.h \
    ../Sources/ptGridInteraction.h \
    ../Sources/imagespot/ptRepairInteraction.h \
    ../Sources/imagespot/ptRepairSpotView.h \
    ../Sources/imagespot/ptRepairSpotEditor.h
HEADERS += ../Sources/ptCalloc.h
HEADERS += ../Sources/ptChannelMixer.h
HEADERS += ../Sources/ptCheck.h
HEADERS += ../Sources/ptChoice.h
HEADERS += ../Sources/ptCimg.h
HEADERS += ../Sources/ptConstants.h
HEADERS += ../Sources/ptCurve.h
HEADERS += ../Sources/ptCurveWindow.h
HEADERS += ../Sources/ptDcRaw.h
HEADERS += ../Sources/ptDefines.h
HEADERS += ../Sources/ptError.h
HEADERS += ../Sources/ptFastBilateral.h
HEADERS += ../Sources/ptGroupBox.h
HEADERS += ../Sources/ptGuiOptions.h
HEADERS += ../Sources/ptHistogramWindow.h
HEADERS += ../Sources/ptImage.h
HEADERS += ../Sources/ptImage8.h
HEADERS += ../Sources/ptInput.h
HEADERS += ../Sources/ptKernel.h
#HEADERS += ../Sources/ptLensfun.h
HEADERS += ../Sources/ptMainWindow.h
HEADERS += ../Sources/ptProcessor.h
HEADERS += ../Sources/ptRefocusMatrix.h
HEADERS += ../Sources/ptResizeFilters.h
HEADERS += ../Sources/ptRGBTemperature.h
HEADERS += ../Sources/ptSettings.h
HEADERS += ../Sources/ptTheme.h
HEADERS += ../Sources/ptViewWindow.h
HEADERS += ../Sources/ptVisibleToolsView.h
HEADERS += ../Sources/ptWhiteBalances.h
HEADERS += ../Sources/ptWiener.h
HEADERS += ../Sources/clapack/blaswrap.h
HEADERS += ../Sources/ptSlider.h
HEADERS += ../Sources/qtsingleapplication/qtsingleapplication.h
HEADERS += ../Sources/qtsingleapplication/qtlocalpeer.h
HEADERS += ../Sources/qtsingleapplication/qtlockedfile.h
HEADERS += ../Sources/clapack/clapack.h
HEADERS += ../Sources/clapack/fio.h
HEADERS += ../Sources/clapack/fmt.h
HEADERS += ../Sources/clapack/fp.h
HEADERS += ../Sources/fastbilateral/array.h
HEADERS += ../Sources/fastbilateral/fast_lbf.h
HEADERS += ../Sources/fastbilateral/math_tools.h
HEADERS += ../Sources/fastbilateral/mixed_vector.h
HEADERS += ../Sources/greyc/CImg.h
HEADERS += ../Sources/ptMessageBox.h
HEADERS += ../Sources/imagespot/ptImageSpot.h
HEADERS += ../Sources/imagespot/ptImageSpotList.h
HEADERS += ../Sources/imagespot/ptRepairSpot.h

SOURCES += ../Sources/ptCalloc.cpp \
    ../Sources/ptReportOverlay.cpp \
    ../Sources/ptAbstractInteraction.cpp \
    ../Sources/ptLineInteraction.cpp \
    ../Sources/ptSimpleRectInteraction.cpp \
    ../Sources/ptRichRectInteraction.cpp \
    ../Sources/ptGridInteraction.cpp \
    ../Sources/imagespot/ptRepairInteraction.cpp \
    ../Sources/imagespot/ptRepairSpotView.cpp \
    ../Sources/imagespot/ptRepairSpotEditor.cpp
SOURCES += ../Sources/ptChannelMixer.cpp
SOURCES += ../Sources/ptCheck.cpp
SOURCES += ../Sources/ptChoice.cpp
SOURCES += ../Sources/ptCimg.cpp
SOURCES += ../Sources/ptCurve.cpp
SOURCES += ../Sources/ptCurveWindow.cpp
SOURCES += ../Sources/ptDcRaw.cpp
SOURCES += ../Sources/ptError.cpp
SOURCES += ../Sources/ptFastBilateral.cpp
SOURCES += ../Sources/ptGroupBox.cpp
SOURCES += ../Sources/ptGuiOptions.cpp
SOURCES += ../Sources/ptHistogramWindow.cpp
SOURCES += ../Sources/ptImage.cpp
SOURCES += ../Sources/ptImage_Cimg.cpp
SOURCES += ../Sources/ptImage_DRC.cpp
SOURCES += ../Sources/ptImage_EAW.cpp
SOURCES += ../Sources/ptImage_GM.cpp
SOURCES += ../Sources/ptImage_GMC.cpp
SOURCES += ../Sources/ptImage_Lensfun.cpp
SOURCES += ../Sources/ptImage_Lqr.cpp
SOURCES += ../Sources/ptImage_Pyramid.cpp
SOURCES += ../Sources/ptImage8.cpp
SOURCES += ../Sources/ptInput.cpp
SOURCES += ../Sources/ptKernel.cpp
#SOURCES += ../Sources/ptLensfun.cpp
SOURCES += ../Sources/ptMain.cpp
SOURCES += ../Sources/ptMainWindow.cpp
SOURCES += ../Sources/ptProcessor.cpp
SOURCES += ../Sources/ptRefocusMatrix.cpp
SOURCES += ../Sources/ptResizeFilters.cpp
SOURCES += ../Sources/ptRGBTemperature.cpp
SOURCES += ../Sources/ptSettings.cpp
SOURCES += ../Sources/ptTheme.cpp
SOURCES += ../Sources/ptViewWindow.cpp
SOURCES += ../Sources/ptVisibleToolsView.cpp
SOURCES += ../Sources/ptWhiteBalances.cpp
SOURCES += ../Sources/ptWiener.cpp
SOURCES += ../Sources/ptSlider.cpp
SOURCES += ../Sources/qtsingleapplication/qtsingleapplication.cpp
SOURCES += ../Sources/qtsingleapplication/qtlocalpeer.cpp
SOURCES += ../Sources/qtsingleapplication/qtlockedfile.cpp
SOURCES += ../Sources/clapack/abort_.c
SOURCES += ../Sources/clapack/close.c
SOURCES += ../Sources/clapack/dgemm.c
SOURCES += ../Sources/clapack/dger.c
SOURCES += ../Sources/clapack/dgesv.c
SOURCES += ../Sources/clapack/dgetf2.c
SOURCES += ../Sources/clapack/dgetrf.c
SOURCES += ../Sources/clapack/dgetrs.c
SOURCES += ../Sources/clapack/dscal.c
SOURCES += ../Sources/clapack/dswap.c
SOURCES += ../Sources/clapack/dtrsm.c
SOURCES += ../Sources/clapack/endfile.c
SOURCES += ../Sources/clapack/err.c
SOURCES += ../Sources/clapack/fmt.c
SOURCES += ../Sources/clapack/fmtlib.c
SOURCES += ../Sources/clapack/idamax.c
SOURCES += ../Sources/clapack/ieeeck.c
SOURCES += ../Sources/clapack/ilaenv.c
SOURCES += ../Sources/clapack/lsame.c
SOURCES += ../Sources/clapack/open.c
SOURCES += ../Sources/clapack/ptaswp.c
SOURCES += ../Sources/clapack/s_cmp.c
SOURCES += ../Sources/clapack/s_copy.c
SOURCES += ../Sources/clapack/s_stop.c
SOURCES += ../Sources/clapack/sfe.c
SOURCES += ../Sources/clapack/sig_die.c
SOURCES += ../Sources/clapack/util.c
SOURCES += ../Sources/clapack/wref.c
SOURCES += ../Sources/clapack/wrtfmt.c
SOURCES += ../Sources/clapack/wsfe.c
SOURCES += ../Sources/clapack/xerbla.c
SOURCES += ../Sources/dcb/dcb_demosaicing_old.c
SOURCES += ../Sources/dcb/dcb_demosaicing.c
SOURCES += ../Sources/perfectraw/lmmse_interpolate.c
SOURCES += ../Sources/rawtherapee/amaze_interpolate.c
SOURCES += ../Sources/rawtherapee/ca_correct.c
SOURCES += ../Sources/rawtherapee/cfa_line_dn.c
SOURCES += ../Sources/rawtherapee/green_equil.c
SOURCES += ../Sources/vcd/ahd_interpolate_mod.c
SOURCES += ../Sources/vcd/ahd_partial_interpolate.c
SOURCES += ../Sources/vcd/es_median_filter.c
SOURCES += ../Sources/vcd/median_filter_new.c
SOURCES += ../Sources/vcd/refinement.c
SOURCES += ../Sources/vcd/vcd_interpolate.c
SOURCES += ../Sources/imagespot/ptImageSpot.cpp
SOURCES += ../Sources/imagespot/ptImageSpotList.cpp
SOURCES += ../Sources/imagespot/ptRepairSpot.cpp

FORMS += ../Sources/ptMainWindow.ui

RESOURCES += ../photivo.qrc

SOURCES += ../Sources/ptMessageBox.cpp
RESOURCES = ../qrc/photivo.qrc
TRANSLATIONS += ../Translations/photivo_Dutch.ts
TRANSLATIONS += ../Translations/photivo_German.ts
TRANSLATIONS += ../Translations/photivo_Italian.ts
TRANSLATIONS += ../Translations/photivo_Russian.ts
TRANSLATIONS += ../Translations/photivo_French.ts


###############################################################################
