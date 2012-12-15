################################################################################
##
## Photivo
##
## Copyright (C) 2008-2009 Jos De Laender
## Copyright (C) 2009-2012 Michael Munzert <mail@mm-log.com>
## Copyright (C) 2011-2012 Bernd Schoeler <brother.john@photivo.org>
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
!build_pass:message("Photivo $${APPVERSION}")

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

# Add path to sources folder to the include search paths.
# Necessary for GCC to find the .h files of (in Designer) promoted widgets.
# When you promote widgets you must specify the .h relative to the "Sources" folder.
INCLUDEPATH    += $${_PRO_FILE_PWD_}/../Sources

################################################################################

unix {
  QT         += network
  CONFIG     += link_pkgconfig
  PKGCONFIG  += GraphicsMagick++ GraphicsMagickWand
  QMAKE_CC    = ccache /usr/bin/gcc
  QMAKE_CXX   = ccache /usr/bin/g++

  # use a CImg include provided by the system instead of the local copy
  message($$CONFIG)
  CONFIG(WithSystemCImg) {
    PKGCONFIG  += CImg
    QMAKE_CXXFLAGS_RELEASE     += -DSYSTEM_CIMG
    QMAKE_CXXFLAGS_DEBUG       += -DSYSTEM_CIMG

    system(echo "Using system supplied CImg library")
  }

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

  QMAKE_CC   = gcc
  QMAKE_CXX  = g++

  QMAKE_CFLAGS_RELEASE    += $$(CFLAGS)
  QMAKE_CFLAGS_DEBUG      += $$(CFLAGS)
  QMAKE_CXXFLAGS_RELEASE  += $$(CXXFLAGS)
  QMAKE_CXXFLAGS_DEBUG    += $$(CXXFLAGS)
  QMAKE_LFLAGS_RELEASE    += $$(LDFLAGS)
  QMAKE_LFLAGS_DEBUG      += $$(LDFLAGS)

  LIBS += \
      -lGraphicsMagick++ -lGraphicsMagickWand -lGraphicsMagick \
      libole32 -lwsock32 -lexpat -lgdi32 -liconv \

  HEADERS +=  ../Sources/ptEcWin7.h \
              ../Sources/ptWinApi.h
  SOURCES +=  ../Sources/ptEcWin7.cpp \
              ../Sources/ptWinApi.cpp
}

macx{
  PKGCONFIG += lcms2
  QMAKE_CC   = /usr/bin/gcc
  QMAKE_CXX  = /usr/bin/g++

  # prevent qmake from adding -arch flags
  QMAKE_CFLAGS_X86_64           = -m64
  QMAKE_CXXFLAGS_X86_64         = -m64 -std=gnu++0x
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

RELEASE_SPECIFIC = -funroll-loops -ftree-vectorize -fopenmp
DEBUG_SPECIFIC   = -g -Wno-unknown-pragmas
COMMON_FLAGS = \
    $$system(pkg-config --cflags-only-I lqr-1) \
    -DAPPVERSION=\'$${APPVERSION}\' \
    -ffast-math -DDLRAW_HAVE_GIMP

!contains(QMAKE_HOST.arch, x86_64) {
  COMMON_FLAGS+=-march=i686
}

QMAKE_CFLAGS_RELEASE   += $${COMMON_FLAGS} $${RELEASE_SPECIFIC}
QMAKE_CXXFLAGS_RELEASE += $${COMMON_FLAGS} $${RELEASE_SPECIFIC} -std=gnu++0x
QMAKE_CFLAGS_DEBUG     += $${COMMON_FLAGS} $${DEBUG_SPECIFIC}
QMAKE_CXXFLAGS_DEBUG   += $${COMMON_FLAGS} $${DEBUG_SPECIFIC} -std=gnu++0x

################################################################################

HEADERS += \
    ../Sources/fastbilateral/array.h \
    ../Sources/fastbilateral/array_n.h \
    ../Sources/fastbilateral/chrono.h \
    ../Sources/fastbilateral/fast_lbf.h \
    ../Sources/fastbilateral/geom.h \
    ../Sources/fastbilateral/math_tools.h \
    ../Sources/fastbilateral/mixed_vector.h \
    ../Sources/fastbilateral/msg_stream.h \
    ../Sources/filemgmt/ptAbstractThumbnailLayouter.h \
    ../Sources/filemgmt/ptColumnGridThumbnailLayouter.h \
    ../Sources/filemgmt/ptFileMgrConstants.h \
    ../Sources/filemgmt/ptFileMgrDM.h \
    ../Sources/filemgmt/ptFileMgrWindow.h \
    ../Sources/filemgmt/ptGraphicsSceneEmitter.h \
    ../Sources/filemgmt/ptGraphicsThumbGroup.h \
    ../Sources/filemgmt/ptGridThumbnailLayouter.h \
    ../Sources/filemgmt/ptImageView.h \
    ../Sources/filemgmt/ptPathBar.h \
    ../Sources/filemgmt/ptRowGridThumbnailLayouter.h \
    ../Sources/filemgmt/ptSingleDirModel.h \
    ../Sources/filemgmt/ptTagList.h \
    ../Sources/filemgmt/ptTagModel.h \
    ../Sources/filemgmt/ptThumbnailCache.h \
    ../Sources/filemgmt/ptThumbnailer.h \
    ../Sources/filters/ptCfgItem.h \
    ../Sources/filters/ptFilter_ABCurves.h \
    ../Sources/filters/ptFilter_ColorContrast.h \
    ../Sources/filters/ptFilter_DetailCurve.h \
    ../Sources/filters/ptFilter_GammaTool.h \
    ../Sources/filters/ptFilter_Highlights.h \
    ../Sources/filters/ptFilter_LumaDenoiseCurve.h \
    ../Sources/filters/ptFilter_LumaSatAdjust.h \
    ../Sources/filters/ptFilter_Outline.h \
    ../Sources/filters/ptFilter_SatCurve.h \
    ../Sources/filters/ptFilter_ShadowsHighlights.h \
    ../Sources/filters/ptFilter_SigContrast.h \
    ../Sources/filters/ptFilter_StdCurve.h \
    ../Sources/filters/ptFilter_ToneAdjust.h \
    ../Sources/filters/ptFilter_Wiener.h \
    ../Sources/filters/ptFilterBase.h \
    ../Sources/filters/ptFilterConfig.h \
    ../Sources/filters/ptFilterDM.h \
    ../Sources/filters/ptFilterFactory.h \
    ../Sources/filters/ptFilterUids.h \
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
    ../Sources/ptError.h \
    ../Sources/ptFastBilateral.h \
    ../Sources/ptGridInteraction.h \
    ../Sources/ptGroupBox.h \
    ../Sources/ptGuiOptions.h \
    ../Sources/ptHistogramWindow.h \
    ../Sources/ptImage.h \
    ../Sources/ptImage8.h \
    ../Sources/ptAbstractInteraction.h \
    ../Sources/ptImageHelper.h \
    ../Sources/ptInfo.h \
    ../Sources/ptInput.h \
    ../Sources/ptKernel.h \
    ../Sources/ptLensfun.h \
    ../Sources/ptLineInteraction.h \
    ../Sources/ptMainWindow.h \
    ../Sources/ptMessageBox.h \
    ../Sources/ptParseCli.h \
    ../Sources/ptProcessor.h \
    ../Sources/ptReportOverlay.h \
    ../Sources/ptResizeFilters.h \
    ../Sources/ptRGBTemperature.h \
    ../Sources/ptRichRectInteraction.h \
    ../Sources/ptSettings.h \
    ../Sources/ptSimpleRectInteraction.h \
    ../Sources/ptSlider.h \
    ../Sources/ptTempFilterBase.h \
    ../Sources/ptTheme.h \
    ../Sources/ptToolBox.h \
    ../Sources/ptViewWindow.h \
    ../Sources/ptVisibleToolsView.h \
    ../Sources/ptWhiteBalances.h \
    ../Sources/ptWidget.h \
    ../Sources/ptWiener.h \
    ../Sources/qtsingleapplication/qtlocalpeer.h \
    ../Sources/qtsingleapplication/qtlockedfile.h \
    ../Sources/qtsingleapplication/qtsingleapplication.h \
    ../Sources/filters/imagespot/ptFilter_SpotTuning.h \
    ../Sources/filters/imagespot/ptImageSpot.h \
    ../Sources/filters/imagespot/ptImageSpotEditor.h \
    ../Sources/filters/imagespot/ptImageSpotItemDelegate.h \
    ../Sources/filters/imagespot/ptImageSpotModel.h \
    ../Sources/filters/imagespot/ptTuningSpot.h \
#    ../Sources/filters/imagespot/ptRepairInteraction.h \
#    ../Sources/filters/imagespot/ptRepairSpot.h \
    ../Sources/filters/imagespot/ptSpotInteraction.h \
    ../Sources/filters/imagespot/ptSpotListWidget.h \
    ../Sources/ptTempFile.h \
    ../Sources/filters/ptStorable.h \
    ../Sources/filters/ptFilter_ColorIntensity.h \
    ../Sources/filters/ptFilter_Brightness.h \
    ../Sources/filters/ptFilter_ReinhardBrighten.h \
    ../Sources/filters/ptFilter_Normalization.h \
    ../Sources/filters/ptFilter_ColorEnhancement.h \
    ../Sources/filters/ptFilter_Levels.h \
    ../Sources/filters/ptFilter_LMHRecovery.h \
    ../Sources/filters/ptFilter_Drc.h \
    ../Sources/filters/ptFilter_LabTransform.h \
    ../Sources/filters/ptFilter_Saturation.h \
    ../Sources/filters/ptFilter_ColorBoost.h \
    ../Sources/filters/ptFilter_Tone.h \
    ../Sources/filters/imagespot/ptImageSpotList.h \
    ../Sources/batch/ptJobListItem.h \
    ../Sources/filters/ptFilter_Vignette.h \
    ../Sources/filters/ptFilter_Exposure.h \
    ../Sources/batch/ptBatchWindow.h \
    ../Sources/batch/ptJobListModel.h


SOURCES += \
    ../Sources/dcb/dcb_demosaicing.c \
    ../Sources/filemgmt/ptColumnGridThumbnailLayouter.cpp \
    ../Sources/filemgmt/ptFileMgrDM.cpp \
    ../Sources/filemgmt/ptFileMgrWindow.cpp \
    ../Sources/filemgmt/ptGraphicsSceneEmitter.cpp \
    ../Sources/filemgmt/ptGraphicsThumbGroup.cpp \
    ../Sources/filemgmt/ptGridThumbnailLayouter.cpp \
    ../Sources/filemgmt/ptImageView.cpp \
    ../Sources/filemgmt/ptPathBar.cpp \
    ../Sources/filemgmt/ptRowGridThumbnailLayouter.cpp \
    ../Sources/filemgmt/ptSingleDirModel.cpp \
    ../Sources/filemgmt/ptTagList.cpp \
    ../Sources/filemgmt/ptTagModel.cpp \
    ../Sources/filemgmt/ptThumbnailCache.cpp \
    ../Sources/filemgmt/ptThumbnailer.cpp \
    ../Sources/filters/ptCfgItem.cpp \
    ../Sources/filters/ptFilter_ABCurves.cpp \
    ../Sources/filters/ptFilter_ColorContrast.cpp \
    ../Sources/filters/ptFilter_DetailCurve.cpp \
    ../Sources/filters/ptFilter_GammaTool.cpp \
    ../Sources/filters/ptFilter_Highlights.cpp \
    ../Sources/filters/ptFilter_LumaDenoiseCurve.cpp \
    ../Sources/filters/ptFilter_LumaSatAdjust.cpp \
    ../Sources/filters/ptFilter_Outline.cpp \
    ../Sources/filters/ptFilter_SatCurve.cpp \
    ../Sources/filters/ptFilter_ShadowsHighlights.cpp \
    ../Sources/filters/ptFilter_SigContrast.cpp \
    ../Sources/filters/ptFilter_StdCurve.cpp \
    ../Sources/filters/ptFilter_ToneAdjust.cpp \
    ../Sources/filters/ptFilter_Wiener.cpp \
    ../Sources/filters/ptFilterBase.cpp \
    ../Sources/filters/ptFilterConfig.cpp \
    ../Sources/filters/ptFilterDM.cpp \
    ../Sources/filters/ptFilterFactory.cpp \
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
    ../Sources/ptAbstractInteraction.cpp \
    ../Sources/ptImageHelper.cpp \
    ../Sources/ptInfo.cpp \
    ../Sources/ptInput.cpp \
    ../Sources/ptKernel.cpp \
    ../Sources/ptLensfun.cpp \
    ../Sources/ptLineInteraction.cpp \
    ../Sources/ptMain.cpp \
    ../Sources/ptMainWindow.cpp \
    ../Sources/ptMessageBox.cpp \
    ../Sources/ptParseCli.cpp \
    ../Sources/ptProcessor.cpp \
    ../Sources/ptReportOverlay.cpp \
    ../Sources/ptResizeFilters.cpp \
    ../Sources/ptRGBTemperature.cpp \
    ../Sources/ptRichRectInteraction.cpp \
    ../Sources/ptSettings.cpp \
    ../Sources/ptSimpleRectInteraction.cpp \
    ../Sources/ptSlider.cpp \
    ../Sources/ptTempFilterBase.cpp \
    ../Sources/ptTheme.cpp \
    ../Sources/ptToolBox.cpp \
    ../Sources/ptViewWindow.cpp \
    ../Sources/ptVisibleToolsView.cpp \
    ../Sources/ptWhiteBalances.cpp \
    ../Sources/ptWidget.cpp \
    ../Sources/ptWiener.cpp \
    ../Sources/qtsingleapplication/qtlocalpeer.cpp \
    ../Sources/qtsingleapplication/qtlockedfile.cpp \
    ../Sources/qtsingleapplication/qtlockedfile_unix.cpp \
    ../Sources/qtsingleapplication/qtlockedfile_win.cpp \
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
    ../Sources/vcd/vcd_interpolate.c \
    ../Sources/filters/imagespot/ptFilter_SpotTuning.cpp \
    ../Sources/filters/imagespot/ptImageSpot.cpp \
    ../Sources/filters/imagespot/ptImageSpotEditor.cpp \
    ../Sources/filters/imagespot/ptImageSpotItemDelegate.cpp \
    ../Sources/filters/imagespot/ptImageSpotModel.cpp \
    ../Sources/filters/imagespot/ptTuningSpot.cpp \
#    ../Sources/filters/imagespot/ptRepairInteraction.cpp \
#    ../Sources/filters/imagespot/ptRepairSpot.cpp \
    ../Sources/filters/imagespot/ptSpotInteraction.cpp \
    ../Sources/filters/imagespot/ptSpotListWidget.cpp \
    ../Sources/ptTempFile.cpp \
    ../Sources/filters/imagespot/ptImageSpotList.cpp \
    ../Sources/filters/ptFilter_ColorIntensity.cpp \
    ../Sources/filters/ptFilter_Brightness.cpp \
    ../Sources/filters/ptFilter_ReinhardBrighten.cpp \
    ../Sources/filters/ptFilter_Normalization.cpp \
    ../Sources/filters/ptFilter_ColorEnhancement.cpp \
    ../Sources/filters/ptFilter_Levels.cpp \
    ../Sources/filters/ptFilter_LMHRecovery.cpp \
    ../Sources/filters/ptFilter_Drc.cpp \
    ../Sources/filters/ptFilter_LabTransform.cpp \
    ../Sources/filters/ptFilter_Saturation.cpp \
    ../Sources/filters/ptFilter_ColorBoost.cpp \
    ../Sources/filters/ptFilter_Tone.cpp \
    ../Sources/batch/ptJobListItem.cpp \
    ../Sources/batch/ptBatchWindow.cpp \
    ../Sources/filters/ptFilter_Vignette.cpp \
    ../Sources/filters/ptFilter_Exposure.cpp \
    ../Sources/batch/ptJobListModel.cpp


FORMS += \
    ../Sources/filemgmt/ptFileMgrWindow.ui \
    ../Sources/ptMainWindow.ui \
    ../Sources/filters/ptFilter_Wiener.ui \
    ../Sources/filters/ptFilter_Outline.ui \
    ../Sources/filters/imagespot/ptFilter_SpotTuning.ui \
    ../Sources/filters/imagespot/ptSpotListWidget.ui \
    ../Sources/filters/ptFilter_ColorIntensity.ui \
    ../Sources/filters/ptFilter_LMHRecovery.ui \
    ../Sources/filters/ptFilter_Tone.ui \
    ../Sources/batch/ptBatchWindow.ui \
    ../Sources/filters/ptFilter_Vignette.ui \
    ../Sources/filters/ptFilter_Exposure.ui

RESOURCES += ../qrc/photivo.qrc

TRANSLATIONS += ../Translations/photivo_Dutch.ts
TRANSLATIONS += ../Translations/photivo_French.ts
TRANSLATIONS += ../Translations/photivo_German.ts
TRANSLATIONS += ../Translations/photivo_Italian.ts
TRANSLATIONS += ../Translations/photivo_Polish.ts
TRANSLATIONS += ../Translations/photivo_Russian.ts
TRANSLATIONS += ../Translations/photivo_Spanish.ts
TRANSLATIONS += ../Translations/photivo_Czech.ts

################################################################################

# Include PRO file for special local system specific settings, e.g.
# additional include paths for MinGW installations on Windows.
# These settings are only valid for one individual computer.
# Because of that local-system-specific.pro is not version controlled.
exists(../local-system-specific.pro) {
  include(../local-system-specific.pro)
}
