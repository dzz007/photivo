################################################################################
##
## Photivo
##
## Copyright (C) 2008-2009 Jos De Laender
## Copyright (C) 2009-2010 Michael Munzert <mail@mm-log.com>
## Copyright (C) 2011-2013 Bernd Schoeler <brother.john@photivo.org>
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

# Get version info from hg and make it available to the application.
win32 {
  DEVNULL="1>nul 2>nul"
} else {
  DEVNULL="1>/dev/null 2>/dev/null"
}

!system("hg $$DEVNULL") {
  !build_pass:warning("Could not call Mercurial to determine Photivo's version info.")
  !build_pass:warning("Using current date/time instead.")
  APPVERSION = "compiled at $$_DATE_"

} else {
  HGBRANCH = $$system(hg branch)
  APPVERSION = $$system('hg log --limit 1 --branch $$HGBRANCH --template "{date|shortdate} (rev {node|short})"')

  isEmpty(APPVERSION) {
    !build_pass:warning("Mercurial returned empty version info.")
    !build_pass:warning("Using current date/time instead.")
    APPVERSION = "compiled at $$_DATE_"
  } else {
    # Find out if working dir is clean. If not add a "+" to the version string.
    WDSTATUS = $$system(hg identify)
    contains(WDSTATUS, ^[^\\+]*\\+): APPVERSION = "$$APPVERSION+"
  }
}
!debug:!build_pass:message("Building Photivo $$APPVERSION")

#------------------------------------------------------------------------------
#--- Qt configuration ---
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets 

TEMPLATE     = app
TARGET       = photivo
CONFIG      += silent

DESTDIR = ..

!release {
  CONFIG += console
}

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

win32 {
  QT       += network
  CONFIG   += link_pkgconfig
  RC_FILE   = photivo.rc
}

unix {
  QT         += network
  CONFIG     += link_pkgconfig
}

#------------------------------------------------------------------------------
#--- Compiler and linker configuration ---

# * Add path to sources folder to the include search paths.
#   Necessary for GCC to find the .h files of (in Designer) promoted widgets.
#   When you promote widgets you must specify the .h relative to the "Sources" folder.
# * Pull in additional include paths from the custom INCLUDEPATHS environment variable.
INCLUDEPATH += $${_PRO_FILE_PWD_}/../Sources $$(INCLUDEPATHS)

# The APPVERSION string has a space in it, i.e. it cannot go into DEFINES
COMPILERFLAGS_ALL = -ffast-math -DAPPVERSION=\"$${APPVERSION}\"

# Flags from the environment must be pulled in explicitely like this appended them
# at the end of the flags. Needed to ensure that user-settings from the environment
# have the highest priority and are not overwritten by default from QMake.
QMAKE_CXXFLAGS_RELEASE += -funroll-loops -ftree-vectorize -fopenmp $$(CXXFLAGS)
QMAKE_CFLAGS_RELEASE   += $$(CFLAGS)
QMAKE_LFLAGS_RELEASE   += $$(LDFLAGS)
QMAKE_CXXFLAGS_DEBUG   += $$(CXXFLAGS)
QMAKE_CFLAGS_DEBUG     += $$(CFLAGS)
QMAKE_LFLAGS_DEBUG     += $$(LDFLAGS)

QMAKE_CXXFLAGS += $${COMPILERFLAGS_ALL} -std=gnu++0x
QMAKE_CFLAGS   += $${COMPILERFLAGS_ALL}
QMAKE_LFLAGS   += $${COMPILERFLAGS_ALL}

LIBS += -lgomp -lpthread \
        -ljpeg -llcms2 -lexiv2 -lfftw3 -llensfun

win32 {
  PKGCONFIG += GraphicsMagick++ GraphicsMagickWand lqr-1
  LIBS      += libole32 -lwsock32 -lgdi32 -lexpat -liconv
  # see http://www.libssh2.org/mail/libssh2-devel-archive-2011-10/0069.shtml
  DEFINES   += WIN32_LEAN_AND_MEAN
}

unix {
  DEFINES            += PREFIX=$${PREFIX}
  PKGCONFIG          += GraphicsMagick++ GraphicsMagickWand lqr-1
  LIBS               += $$system(GraphicsMagick++-config --libs)
  QMAKE_LFLAGS_DEBUG += -rdynamic
  
  # use a CImg include provided by the system instead of the local copy
  CONFIG(WithSystemCImg) {
    PKGCONFIG += CImg
    DEFINES   += SYSTEM_CIMG
    system(echo "Using system supplied CImg library")
  }
}

macx {
  PKGCONFIG += lcms2 lqr-1

  # prevent qmake from adding -arch flags
  QMAKE_CFLAGS_X86_64           = -m64
  QMAKE_CXXFLAGS_X86_64         = -m64 -std=gnu++0x
  QMAKE_OBJECTIVE_CFLAGS_X86_64 = -m64
  QMAKE_LFLAGS_X86_64           = -headerpad_max_install_names

  LIBS += -framework QtCore -framework QtGui -framework QtNetwork
}

#------------------------------------------------------------------------------
#--- Workarounds ---

!contains(QMAKE_HOST.arch, x86_64) {
  QMAKE_CXXFLAGS += -march=i686
  QMAKE_CFLAGS   += -march=i686
}

contains(LIBS, -llcms) {
  LIBS -= -llcms
}

#------------------------------------------------------------------------------
#--- Sources ---

HEADERS += \
    ../Sources/batch/ptBatchWindow.h \
    ../Sources/batch/ptJobListItem.h \
    ../Sources/batch/ptJobListModel.h \
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
    ../Sources/filemgmt/ptThumbCache.h \
    ../Sources/filters/imagespot/ptFilter_SpotTuning.h \
    ../Sources/filters/imagespot/ptImageSpot.h \
    ../Sources/filters/imagespot/ptImageSpotEditor.h \
    ../Sources/filters/imagespot/ptImageSpotItemDelegate.h \
    ../Sources/filters/imagespot/ptImageSpotList.h \
    ../Sources/filters/imagespot/ptImageSpotModel.h \
    ../Sources/filters/imagespot/ptSpotInteraction.h \
    ../Sources/filters/imagespot/ptSpotListWidget.h \
    ../Sources/filters/imagespot/ptTuningSpot.h \
    ../Sources/filters/ptCfgItem.h \
    ../Sources/filters/ptFilter_ABCurves.h \
    ../Sources/filters/ptFilter_Brightness.h \
    ../Sources/filters/ptFilter_ColorBoost.h \
    ../Sources/filters/ptFilter_ColorContrast.h \
    ../Sources/filters/ptFilter_ColorEnhancement.h \
    ../Sources/filters/ptFilter_ColorIntensity.h \
    ../Sources/filters/ptFilter_DetailCurve.h \
    ../Sources/filters/ptFilter_Drc.h \
    ../Sources/filters/ptFilter_GammaTool.h \
    ../Sources/filters/ptFilter_Highlights.h \
    ../Sources/filters/ptFilter_LabTransform.h \
    ../Sources/filters/ptFilter_Levels.h \
    ../Sources/filters/ptFilter_LMHRecovery.h \
    ../Sources/filters/ptFilter_LumaDenoiseCurve.h \
    ../Sources/filters/ptFilter_LumaSatAdjust.h \
    ../Sources/filters/ptFilter_Normalization.h \
    ../Sources/filters/ptFilter_Outline.h \
    ../Sources/filters/ptFilter_ReinhardBrighten.h \
    ../Sources/filters/ptFilter_SatCurve.h \
    ../Sources/filters/ptFilter_Saturation.h \
    ../Sources/filters/ptFilter_ShadowsHighlights.h \
    ../Sources/filters/ptFilter_SigContrast.h \
    ../Sources/filters/ptFilter_StdCurve.h \
    ../Sources/filters/ptFilter_Tone.h \
    ../Sources/filters/ptFilter_ToneAdjust.h \
    ../Sources/filters/ptFilter_Wiener.h \
    ../Sources/filters/ptFilterBase.h \
    ../Sources/filters/ptFilterConfig.h \
    ../Sources/filters/ptFilterDM.h \
    ../Sources/filters/ptFilterFactory.h \
    ../Sources/filters/ptFilterUids.h \
    ../Sources/ptStorable.h \
    ../Sources/greyc/CImg.h \
    ../Sources/ptAbstractInteraction.h \
    ../Sources/ptAdobeTable.h \
    ../Sources/ptCalloc.h \
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
    ../Sources/ptGridInteraction.h \
    ../Sources/ptGroupBox.h \
    ../Sources/ptGuiOptions.h \
    ../Sources/ptHistogramWindow.h \
    ../Sources/ptImage.h \
    ../Sources/ptImage8.h \
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
    ../Sources/ptTempFile.h \
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
    ../Sources/ptUtils_Storage.h \
    ../Sources/ptUtils.h \
    ../Sources/filemgmt/ptThumbDefines.h \
    ../Sources/ptMutexLocker.h \
    ../Sources/filemgmt/ptThumbGenMgr.h \
    ../Sources/filemgmt/ptThumbGenWorker.h \
    ../Sources/filemgmt/ptThumbGenHelpers.h \
    ../Sources/filters/ptFilter_TextureContrast.h \
    ../Sources/filters/ptFilter_LocalContrastStretch.h \
    ../Sources/filters/ptFilter_LocalContrast.h \
    ../Sources/filters/ptFilter_ImpulseNR.h \
    ../Sources/filters/ptFilter_ChannelMixer.h \
    ../Sources/filters/ptFilter_LumaDenoise.h \
    ../Sources/filters/ptFilter_ColorDenoise.h \
    ../Sources/filters/ptFilter_GradientSharpen.h \
    ../Sources/filters/ptFilter_PyramidDenoise.h \
    ../Sources/filters/ptFilter_EAWavelets.h \
    ../Sources/filters/ptFilter_GreyCStoration.h \
    ../Sources/filters/ptFilter_Defringe.h \
    ../Sources/filters/ptFilter_WaveletDenoise.h

SOURCES += \
    ../Sources/batch/ptBatchWindow.cpp \
    ../Sources/batch/ptJobListItem.cpp \
    ../Sources/batch/ptJobListModel.cpp \
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
    ../Sources/filemgmt/ptThumbCache.cpp \
    ../Sources/filters/imagespot/ptFilter_SpotTuning.cpp \
    ../Sources/filters/imagespot/ptImageSpot.cpp \
    ../Sources/filters/imagespot/ptImageSpotEditor.cpp \
    ../Sources/filters/imagespot/ptImageSpotItemDelegate.cpp \
    ../Sources/filters/imagespot/ptImageSpotList.cpp \
    ../Sources/filters/imagespot/ptImageSpotModel.cpp \
    ../Sources/filters/imagespot/ptSpotInteraction.cpp \
    ../Sources/filters/imagespot/ptSpotListWidget.cpp \
    ../Sources/filters/imagespot/ptTuningSpot.cpp \
    ../Sources/filters/ptCfgItem.cpp \
    ../Sources/filters/ptFilter_ABCurves.cpp \
    ../Sources/filters/ptFilter_Brightness.cpp \
    ../Sources/filters/ptFilter_ColorBoost.cpp \
    ../Sources/filters/ptFilter_ColorContrast.cpp \
    ../Sources/filters/ptFilter_ColorEnhancement.cpp \
    ../Sources/filters/ptFilter_ColorIntensity.cpp \
    ../Sources/filters/ptFilter_DetailCurve.cpp \
    ../Sources/filters/ptFilter_Drc.cpp \
    ../Sources/filters/ptFilter_GammaTool.cpp \
    ../Sources/filters/ptFilter_Highlights.cpp \
    ../Sources/filters/ptFilter_LabTransform.cpp \
    ../Sources/filters/ptFilter_Levels.cpp \
    ../Sources/filters/ptFilter_LMHRecovery.cpp \
    ../Sources/filters/ptFilter_LumaDenoiseCurve.cpp \
    ../Sources/filters/ptFilter_LumaSatAdjust.cpp \
    ../Sources/filters/ptFilter_Normalization.cpp \
    ../Sources/filters/ptFilter_Outline.cpp \
    ../Sources/filters/ptFilter_ReinhardBrighten.cpp \
    ../Sources/filters/ptFilter_SatCurve.cpp \
    ../Sources/filters/ptFilter_Saturation.cpp \
    ../Sources/filters/ptFilter_ShadowsHighlights.cpp \
    ../Sources/filters/ptFilter_SigContrast.cpp \
    ../Sources/filters/ptFilter_StdCurve.cpp \
    ../Sources/filters/ptFilter_Tone.cpp \
    ../Sources/filters/ptFilter_ToneAdjust.cpp \
    ../Sources/filters/ptFilter_Wiener.cpp \
    ../Sources/filters/ptFilterBase.cpp \
    ../Sources/filters/ptFilterConfig.cpp \
    ../Sources/filters/ptFilterDM.cpp \
    ../Sources/filters/ptFilterFactory.cpp \
    ../Sources/perfectraw/lmmse_interpolate.c \
    ../Sources/ptAbstractInteraction.cpp \
    ../Sources/ptCalloc.cpp \
    ../Sources/ptCheck.cpp \
    ../Sources/ptChoice.cpp \
    ../Sources/ptCimg.cpp \
    ../Sources/ptConfirmRequest.cpp \
    ../Sources/ptCurve.cpp \
    ../Sources/ptCurveWindow.cpp \
    ../Sources/ptDcRaw.cpp \
    ../Sources/ptError.cpp \
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
    ../Sources/ptTempFile.cpp \
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
    ../Sources/ptUtils_Storage.cpp \
    ../Sources/ptStorable.cpp \
    ../Sources/ptUtils.cpp \
    ../Sources/filemgmt/ptThumbDefines.cpp \
    ../Sources/ptMutexLocker.cpp \
    ../Sources/filemgmt/ptThumbGenMgr.cpp \
    ../Sources/filemgmt/ptThumbGenWorker.cpp \
    ../Sources/filemgmt/ptThumbGenHelpers.cpp \
    ../Sources/filters/ptFilter_TextureContrast.cpp \
    ../Sources/filters/ptFilter_LocalContrastStretch.cpp \
    ../Sources/filters/ptFilter_LocalContrast.cpp \
    ../Sources/filters/ptFilter_ImpulseNR.cpp \
    ../Sources/filters/ptFilter_ChannelMixer.cpp \
    ../Sources/filters/ptFilter_LumaDenoise.cpp \
    ../Sources/filters/ptFilter_ColorDenoise.cpp \
    ../Sources/filters/ptFilter_GradientSharpen.cpp \
    ../Sources/filters/ptFilter_PyramidDenoise.cpp \
    ../Sources/filters/ptFilter_EAWavelets.cpp \
    ../Sources/filters/ptFilter_GreyCStoration.cpp \
    ../Sources/filters/ptFilter_Defringe.cpp \
    ../Sources/filters/ptFilter_WaveletDenoise.cpp

FORMS += \
    ../Sources/batch/ptBatchWindow.ui \
    ../Sources/filemgmt/ptFileMgrWindow.ui \
    ../Sources/filters/imagespot/ptFilter_SpotTuning.ui \
    ../Sources/filters/imagespot/ptSpotListWidget.ui \
    ../Sources/filters/ptFilter_ColorIntensity.ui \
    ../Sources/filters/ptFilter_LMHRecovery.ui \
    ../Sources/filters/ptFilter_Outline.ui \
    ../Sources/filters/ptFilter_Tone.ui \
    ../Sources/filters/ptFilter_Wiener.ui \
    ../Sources/filters/ptFilter_ChannelMixer.ui \
    ../Sources/filters/ptFilter_GradientSharpen.ui \
    ../Sources/filters/ptFilter_EAWavelets.ui \
    ../Sources/filters/ptFilter_GreyCStoration.ui \
    ../Sources/filters/ptFilter_Defringe.ui \
    ../Sources/filters/ptFilter_WaveletDenoise.ui \
    ../Sources/ptMainWindow.ui

win32 {
  HEADERS += ../Sources/ptEcWin7.h \
             ../Sources/ptWinApi.h
  SOURCES += ../Sources/ptEcWin7.cpp \
             ../Sources/ptWinApi.cpp
}

RESOURCES += ../qrc/photivo.qrc

TRANSLATIONS += ../Translations/photivo_Danish.ts
TRANSLATIONS += ../Translations/photivo_Dutch.ts
TRANSLATIONS += ../Translations/photivo_French.ts
TRANSLATIONS += ../Translations/photivo_German.ts
TRANSLATIONS += ../Translations/photivo_Italian.ts
TRANSLATIONS += ../Translations/photivo_Polish.ts
TRANSLATIONS += ../Translations/photivo_Russian.ts
TRANSLATIONS += ../Translations/photivo_Spanish.ts
TRANSLATIONS += ../Translations/photivo_Czech.ts
