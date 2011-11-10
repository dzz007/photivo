################################################################################
##
## Photivo
##
## Copyright (C) 2008,2009 Jos De Laender
## Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

APPVERSION = $$system(sh ./get_appversion)
!build_pass:system(echo "Photivo $${APPVERSION}")

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

CONFIG += silent
TEMPLATE     = app
TARGET       = photivo

DEPENDPATH     += .
INCLUDEPATH    += $${PREFIX}/include
DESTDIR         = ..
OBJECTS_DIR     = ../Objects
MOC_DIR         = ../Objects
UI_HEADERS_DIR  = ../Objects
RCC_DIR         = ../Objects

################################################################################

unix {
  QT         += network
  CONFIG     += link_pkgconfig
  PKGCONFIG  += GraphicsMagick++ GraphicsMagickWand
  QMAKE_CC    = ccache /usr/bin/gcc
  QMAKE_CXX   = ccache /usr/bin/g++

  QMAKE_CFLAGS_RELEASE    += -DPREFIX=$${PREFIX} -L$${PREFIX}/lib $$(CFLAGS)
  QMAKE_CXXFLAGS_RELEASE  += -DPREFIX=$${PREFIX} -I$${PREFIX}/include $$(CXXFLAGS)
  QMAKE_CFLAGS_DEBUG      += -DPREFIX=$${PREFIX} -L$${PREFIX}/lib $$(CFLAGS)
  QMAKE_CXXFLAGS_DEBUG    += -DPREFIX=$${PREFIX} -I$${PREFIX}/include $$(CXXFLAGS)
  QMAKE_LFLAGS_DEBUG      += -rdynamic
  
  LIBS += $$system(GraphicsMagick++-config --libs)
}

win32 {
  QT       += network
  RC_FILE   = photivo.rc

  QMAKE_CFLAGS_RELEASE    += $$(CFLAGS)
  QMAKE_CFLAGS_DEBUG      += $$(CFLAGS)
  QMAKE_CXXFLAGS_RELEASE  += $$(CXXFLAGS)
  QMAKE_CXXFLAGS_DEBUG    += $$(CXXFLAGS)
  QMAKE_LFLAGS_RELEASE    += $$(LDFLAGS)
  QMAKE_LFLAGS_DEBUG      += $$(LDFLAGS)
  
  LIBS += \
      -lGraphicsMagick++ -lGraphicsMagickWand -lGraphicsMagick \
      libole32 -lwsock32 -lexpat -lregex -lgdi32 -liconv \
}

macx{
  PKGCONFIG += lcms2
  QMAKE_CC   = /usr/bin/gcc
  QMAKE_CXX  = /usr/bin/g++

  # prevent qmake from adding -arch flags
  QMAKE_CFLAGS_X86_64           = -m64
  QMAKE_CXXFLAGS_X86_64         = -m64
  QMAKE_OBJECTIVE_CFLAGS_X86_64 = -m64
  QMAKE_LFLAGS_X86_64           = -headerpad_max_install_names
  
  LIBS += \
      $$system(pkg-config --libs lcms2) \
      -framework QtCore -framework QtGui -framework QtNetwork
}

################################################################################

LIBS += \
    $$system(pkg-config --libs-only-l lqr-1) \
    -ljpeg -llcms2 -lexiv2 -lfftw3 -llensfun -lgomp -lpthread

RELEASE_SPECIFIC = -O3 -ftree-vectorize -fopenmp
DEBUG_SPECIFIC   = -O0 -g
COMMON_FLAGS = \
    $$system(pkg-config --cflags-only-I lqr-1) \
    -DAPPVERSION=\'$${APPVERSION}\' \
    -ffast-math -DDLRAW_HAVE_GIMP

QMAKE_CFLAGS_RELEASE   += $${COMMON_FLAGS} $${RELEASE_SPECIFIC}
QMAKE_CXXFLAGS_RELEASE += $${COMMON_FLAGS} $${RELEASE_SPECIFIC} 
QMAKE_CFLAGS_DEBUG     += $${COMMON_FLAGS} $${DEBUG_SPECIFIC}
QMAKE_CXXFLAGS_DEBUG   += $${COMMON_FLAGS} $${DEBUG_SPECIFIC}

################################################################################

HEADERS += \
    ../Sources/clapack/blaswrap.h \
    ../Sources/clapack/clapack.h \
    ../Sources/clapack/fio.h \
    ../Sources/clapack/fmt.h \
    ../Sources/clapack/fp.h \
    ../Sources/fastbilateral/array.h \
    ../Sources/fastbilateral/fast_lbf.h \
    ../Sources/fastbilateral/math_tools.h \
    ../Sources/fastbilateral/mixed_vector.h \
    ../Sources/filemgmt/ptAbstractThumbnailLayouter.h \
    ../Sources/filemgmt/ptColumnGridThumbnailLayouter.h \
    ../Sources/filemgmt/ptFileMgrDM.h \
    ../Sources/filemgmt/ptFileMgrThumbnailer.h \
    ../Sources/filemgmt/ptFileMgrWindow.h \
    ../Sources/filemgmt/ptGraphicsSceneEmitter.h \
    ../Sources/filemgmt/ptGraphicsThumbGroup.h \
    ../Sources/filemgmt/ptGridThumbnailLayouter.h \
    ../Sources/filemgmt/ptRowGridThumbnailLayouter.h \
    ../Sources/filemgmt/ptThumbnailCache.h \
    ../Sources/greyc/CImg.h \
    ../Sources/ptAdobeTable.h \
    ../Sources/ptCalloc.h \
    ../Sources/ptChannelMixer.h \
    ../Sources/ptCheck.h \
    ../Sources/ptChoice.h \
    ../Sources/ptCimg.h \
    ../Sources/ptConfirmRequest.h \
    ../Sources/ptConstants.h \
    ../Sources/ptCurve.h \
    ../Sources/ptCurveWindow.h \
    ../Sources/ptDcRaw.h \
    ../Sources/ptDefines.h \
    ../Sources/ptEcWin7.h \
    ../Sources/ptError.h \
    ../Sources/ptFastBilateral.h \
    ../Sources/ptGridInteraction.h \
    ../Sources/ptGroupBox.h \
    ../Sources/ptGuiOptions.h \
    ../Sources/ptHistogramWindow.h \
    ../Sources/ptImage.h \
    ../Sources/ptImage8.h \
    ../Sources/ptImageInteraction.h \
    ../Sources/ptInput.h \
    ../Sources/ptKernel.h \
    ../Sources/ptLineInteraction.h \
    ../Sources/ptMainWindow.h \
    ../Sources/ptMessageBox.h \
    ../Sources/ptParseCli.h \
    ../Sources/ptProcessor.h \
    ../Sources/ptRefocusMatrix.h \
    ../Sources/ptReportOverlay.h \
    ../Sources/ptResizeFilters.h \
    ../Sources/ptRGBTemperature.h \
    ../Sources/ptRichRectInteraction.h \
    ../Sources/ptSettings.h \
    ../Sources/ptSimpleRectInteraction.h \
    ../Sources/ptSlider.h \
    ../Sources/ptTheme.h \
    ../Sources/ptViewWindow.h \
    ../Sources/ptVisibleToolsView.h \
    ../Sources/ptWhiteBalances.h \
    ../Sources/ptWiener.h \
    ../Sources/qtsingleapplication/qtlocalpeer.h \
    ../Sources/qtsingleapplication/qtlockedfile.h \
    ../Sources/qtsingleapplication/qtsingleapplication.h

SOURCES += \
    ../Sources/clapack/abort_.c \
    ../Sources/clapack/close.c \
    ../Sources/clapack/dgemm.c \
    ../Sources/clapack/dger.c \
    ../Sources/clapack/dgesv.c \
    ../Sources/clapack/dgetf2.c \
    ../Sources/clapack/dgetrf.c \
    ../Sources/clapack/dgetrs.c \
    ../Sources/clapack/dscal.c \
    ../Sources/clapack/dswap.c \
    ../Sources/clapack/dtrsm.c \
    ../Sources/clapack/endfile.c \
    ../Sources/clapack/err.c \
    ../Sources/clapack/fmt.c \
    ../Sources/clapack/fmtlib.c \
    ../Sources/clapack/idamax.c \
    ../Sources/clapack/ieeeck.c \
    ../Sources/clapack/ilaenv.c \
    ../Sources/clapack/lsame.c \
    ../Sources/clapack/open.c \
    ../Sources/clapack/ptaswp.c \
    ../Sources/clapack/s_cmp.c \
    ../Sources/clapack/s_copy.c \
    ../Sources/clapack/s_stop.c \
    ../Sources/clapack/sfe.c \
    ../Sources/clapack/sig_die.c \
    ../Sources/clapack/util.c \
    ../Sources/clapack/wref.c \
    ../Sources/clapack/wrtfmt.c \
    ../Sources/clapack/wsfe.c \
    ../Sources/clapack/xerbla.c \
    ../Sources/dcb/dcb_demosaicing.c \
    ../Sources/dcb/dcb_demosaicing_old.c \
    ../Sources/filemgmt/ptColumnGridThumbnailLayouter.cpp \
    ../Sources/filemgmt/ptFileMgrDM.cpp \
    ../Sources/filemgmt/ptFileMgrThumbnailer.cpp \
    ../Sources/filemgmt/ptFileMgrWindow.cpp \
    ../Sources/filemgmt/ptGraphicsSceneEmitter.cpp \
    ../Sources/filemgmt/ptGraphicsThumbGroup.cpp \
    ../Sources/filemgmt/ptGridThumbnailLayouter.cpp \
    ../Sources/filemgmt/ptRowGridThumbnailLayouter.cpp \
    ../Sources/filemgmt/ptThumbnailCache.cpp \
    ../Sources/perfectraw/lmmse_interpolate.c \
    ../Sources/ptCalloc.cpp \
    ../Sources/ptChannelMixer.cpp \
    ../Sources/ptCheck.cpp \
    ../Sources/ptChoice.cpp \
    ../Sources/ptCimg.cpp \
    ../Sources/ptConfirmRequest.cpp \
    ../Sources/ptCurve.cpp \
    ../Sources/ptCurveWindow.cpp \
    ../Sources/ptDcRaw.cpp \
    ../Sources/ptEcWin7.cpp \
    ../Sources/ptError.cpp \
    ../Sources/ptFastBilateral.cpp \
    ../Sources/ptGridInteraction.cpp \
    ../Sources/ptGroupBox.cpp \
    ../Sources/ptGuiOptions.cpp \
    ../Sources/ptHistogramWindow.cpp \
    ../Sources/ptImage.cpp \
    ../Sources/ptImage_Cimg.cpp \
    ../Sources/ptImage_DRC.cpp \
    ../Sources/ptImage_EAW.cpp \
    ../Sources/ptImage_GM.cpp \
    ../Sources/ptImage_GMC.cpp \
    ../Sources/ptImage_Lensfun.cpp \
    ../Sources/ptImage_Lqr.cpp \
    ../Sources/ptImage_Pyramid.cpp \
    ../Sources/ptImage8.cpp \
    ../Sources/ptImageInteraction.cpp \
    ../Sources/ptInput.cpp \
    ../Sources/ptKernel.cpp \
    ../Sources/ptLineInteraction.cpp \
    ../Sources/ptMain.cpp \
    ../Sources/ptMainWindow.cpp \
    ../Sources/ptMessageBox.cpp \
    ../Sources/ptParseCli.cpp \
    ../Sources/ptProcessor.cpp \
    ../Sources/ptRefocusMatrix.cpp \
    ../Sources/ptReportOverlay.cpp \
    ../Sources/ptResizeFilters.cpp \
    ../Sources/ptRGBTemperature.cpp \
    ../Sources/ptRichRectInteraction.cpp \
    ../Sources/ptSettings.cpp \
    ../Sources/ptSimpleRectInteraction.cpp \
    ../Sources/ptSlider.cpp \
    ../Sources/ptTheme.cpp \
    ../Sources/ptViewWindow.cpp \
    ../Sources/ptVisibleToolsView.cpp \
    ../Sources/ptWhiteBalances.cpp \
    ../Sources/ptWiener.cpp \
    ../Sources/qtsingleapplication/qtlocalpeer.cpp \
    ../Sources/qtsingleapplication/qtlockedfile.cpp \
    ../Sources/qtsingleapplication/qtsingleapplication.cpp \
    ../Sources/rawtherapee/amaze_interpolate.c \
    ../Sources/rawtherapee/ca_correct.c \
    ../Sources/rawtherapee/cfa_line_dn.c \
    ../Sources/rawtherapee/green_equil.c \
    ../Sources/vcd/ahd_interpolate_mod.c \
    ../Sources/vcd/ahd_partial_interpolate.c \
    ../Sources/vcd/es_median_filter.c \
    ../Sources/vcd/median_filter_new.c \
    ../Sources/vcd/refinement.c \
    ../Sources/vcd/vcd_interpolate.c

FORMS += \
    ../Sources/filemgmt/ptFileMgrWindow.ui \
    ../Sources/ptMainWindow.ui

RESOURCES += ../qrc/photivo.qrc

TRANSLATIONS += ../Translations/photivo_Dutch.ts
TRANSLATIONS += ../Translations/photivo_French.ts
TRANSLATIONS += ../Translations/photivo_German.ts
TRANSLATIONS += ../Translations/photivo_Italian.ts
TRANSLATIONS += ../Translations/photivo_Russian.ts

################################################################################

# Include PRO file for special local system specific settings, e.g.
# additional include paths for MinGW installations on Windows.
# These settings are only valid for one individual computer.
# Because of that local-system-specific.pro is not version controlled.
exists(../local-system-specific.pro) {
  include(../local-system-specific.pro)
}
