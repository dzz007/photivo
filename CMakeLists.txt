################################################################################
##
## Photivo
##
## Copyright (C) 2013 Sergey Salnikov <salsergey@gmail.com>
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
# The file CMakeLists.txt is generated automatically and you shouldn't
# edit it directly. Instead of this you should edit CMakeLists.txt.in
# and then run a script CMakeGenerate.py.
# The script extracts all sources, headers and UIs from the
# photivoProject/photivoProject.pro file and includes them into
# CMakeLists.txt. So these variables should be lived empty in CMakeLists.txt.in.
#
################################################################################

CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

PROJECT( photivo )

if( NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.6 )
  message( WARNING "\nAt least GCC 4.6 is required to build Photivo." )
endif( NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.6 )

if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Release )
endif( NOT CMAKE_BUILD_TYPE )

find_package( PkgConfig REQUIRED )
find_package( JPEG REQUIRED )
find_package( Qt4 4.7 REQUIRED QtCore QtGui QtNetwork )
include( ${QT_USE_FILE} )

pkg_check_modules( GLIB2   REQUIRED glib-2.0>=2.18 )
pkg_check_modules( LCMS2   REQUIRED lcms2 )
pkg_check_modules( EXIV2   REQUIRED exiv2>=0.19 )
pkg_check_modules( LQR     REQUIRED lqr-1>=0.4.1 )
pkg_check_modules( LENSFUN REQUIRED lensfun>=0.2.5 )
pkg_check_modules( GM      REQUIRED GraphicsMagick++>=1.3.12 )
pkg_check_modules( GMW     REQUIRED GraphicsMagickWand>=1.3.12 )
pkg_check_modules( FFTW3   REQUIRED fftw3>=3.2.2 )

if( ${WITH_GIMP} )
  pkg_check_modules( GIMP REQUIRED gimp-2.0>=2.6.10 )
  pkg_check_modules( GTK2 REQUIRED gtk+-2.0 )
endif( ${WITH_GIMP} )

# Determine Photivo version with mercurial. Use current date if mercurial is unavailable.
find_program( MERCURIAL hg )
if( MERCURIAL )
  execute_process( COMMAND hg branch OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE BRANCH )
  execute_process( COMMAND hg log --limit 1 --branch ${BRANCH} --template {date|shortdate}
                   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE REVDATE )
  execute_process( COMMAND hg identify --id OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE REVISION )
  set( APPVERSION "${REVDATE} (rev ${REVISION})" )
else( MERCURIAL )
  message( WARNING "Could not call Mercurial to determine Photivo's version info, using current date/time instead." )
  execute_process( COMMAND date +%c OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE DATE )
  set( APPVERSION "compiled at ${DATE}" )
endif( MERCURIAL )

message( "\nBuilding Photivo ${APPVERSION}\n" )

# Choose Photivo parts to be built.
option( WITH_CLEAR          "whether to build ptClear" ON )
option( WITH_GIMP           "whether to build Gimp plugin" )
option( WITH_ADOBE_PROFILES "whether to build Adobe profiles creator" )

message( "Build Photivo                : ON" )
message( "Build ptClear                : ${WITH_CLEAR}" )
message( "Build Gimp plugin            : ${WITH_GIMP}" )
message( "Build curves creator         : OFF - obsolete" )
message( "Build Adobe profiles creator : ${WITH_ADOBE_PROFILES}\n" )

set( CMAKE_CXX_FLAGS "" )
set( CMAKE_C_FLAGS "" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops -fopenmp -std=gnu++0x" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -std=gnu++0x" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=gnu++0x" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/Sources
                     ${CMAKE_CURRENT_BINARY_DIR}
                     ${QT4_INCLUDES}
                     ${GLIB2_INCLUDE_DIRS}
                     ${JPEG_INCLUDE_DIR}
                     ${EXIV2_INCLUDE_DIRS}
                     ${LCMS2_INCLUDE_DIRS}
                     ${LQR_INCLUDE_DIRS}
                     ${LENSFUN_INCLUDE_DIRS}
                     ${GM_INCLUDE_DIRS}
                     ${GMW_INCLUDE_DIRS}
                     ${FFTW3_INCLUDE_DIRS}
)

link_directories( ${GLIB2_LIBRARY_DIRS}
                  ${JPEG_LIBRARY_DIRS}
                  ${EXIV2_LIBRARY_DIRS}
                  ${LCMS2_LIBRARY_DIRS}
                  ${LQR_LIBRARY_DIRS}
                  ${LENSFUN_LIBRARY_DIRS}
                  ${GM_LIBRARY_DIRS}
                  ${GMW_LIBRARY_DIRS}
                  ${FFTW3_LIBRARY_DIRS}
)

set( photivo_MOC_HDRS
     Sources/batch/ptBatchWindow.h
     Sources/batch/ptJobListItem.h
     Sources/batch/ptJobListModel.h
     Sources/filemgmt/ptFileMgrDM.h
     Sources/filemgmt/ptFileMgrWindow.h
     Sources/filemgmt/ptGraphicsSceneEmitter.h
     Sources/filemgmt/ptImageView.h
     Sources/filemgmt/ptPathBar.h
     Sources/filemgmt/ptSingleDirModel.h
     Sources/filemgmt/ptTagList.h
     Sources/filemgmt/ptTagModel.h
     Sources/filters/imagespot/ptFilter_SpotTuning.h
     Sources/filters/imagespot/ptImageSpotEditor.h
     Sources/filters/imagespot/ptImageSpotItemDelegate.h
     Sources/filters/imagespot/ptImageSpotModel.h
     Sources/filters/imagespot/ptSpotInteraction.h
     Sources/filters/imagespot/ptSpotListWidget.h
     Sources/filters/ptFilter_ABCurves.h
     Sources/filters/ptFilter_Brightness.h
     Sources/filters/ptFilter_ColorBoost.h
     Sources/filters/ptFilter_ColorContrast.h
     Sources/filters/ptFilter_ColorEnhancement.h
     Sources/filters/ptFilter_ColorIntensity.h
     Sources/filters/ptFilter_DetailCurve.h
     Sources/filters/ptFilter_Drc.h
     Sources/filters/ptFilter_GammaTool.h
     Sources/filters/ptFilter_Highlights.h
     Sources/filters/ptFilter_LabTransform.h
     Sources/filters/ptFilter_Levels.h
     Sources/filters/ptFilter_LMHRecovery.h
     Sources/filters/ptFilter_LumaDenoiseCurve.h
     Sources/filters/ptFilter_LumaSatAdjust.h
     Sources/filters/ptFilter_Normalization.h
     Sources/filters/ptFilter_Outline.h
     Sources/filters/ptFilter_ReinhardBrighten.h
     Sources/filters/ptFilter_SatCurve.h
     Sources/filters/ptFilter_Saturation.h
     Sources/filters/ptFilter_ShadowsHighlights.h
     Sources/filters/ptFilter_SigContrast.h
     Sources/filters/ptFilter_StdCurve.h
     Sources/filters/ptFilter_Tone.h
     Sources/filters/ptFilter_ToneAdjust.h
     Sources/filters/ptFilter_Wiener.h
     Sources/filters/ptFilterBase.h
     Sources/filters/ptFilterFactory.h
     Sources/ptAbstractInteraction.h
     Sources/ptCheck.h
     Sources/ptChoice.h
     Sources/ptCurveWindow.h
     Sources/ptGridInteraction.h
     Sources/ptGroupBox.h
     Sources/ptHistogramWindow.h
     Sources/ptInput.h
     Sources/ptLineInteraction.h
     Sources/ptMainWindow.h
     Sources/ptMessageBox.h
     Sources/ptReportOverlay.h
     Sources/ptRichRectInteraction.h
     Sources/ptSimpleRectInteraction.h
     Sources/ptSlider.h
     Sources/ptTempFilterBase.h
     Sources/ptToolBox.h
     Sources/ptViewWindow.h
     Sources/ptVisibleToolsView.h
     Sources/ptWidget.h
     Sources/qtsingleapplication/qtlocalpeer.h
     Sources/qtsingleapplication/qtsingleapplication.h
     Sources/filemgmt/ptThumbGenWorker.h
)

set( photivo_SRCS
     Sources/batch/ptBatchWindow.cpp
     Sources/batch/ptJobListItem.cpp
     Sources/batch/ptJobListModel.cpp
     Sources/filemgmt/ptColumnGridThumbnailLayouter.cpp
     Sources/filemgmt/ptFileMgrDM.cpp
     Sources/filemgmt/ptFileMgrWindow.cpp
     Sources/filemgmt/ptGraphicsSceneEmitter.cpp
     Sources/filemgmt/ptGraphicsThumbGroup.cpp
     Sources/filemgmt/ptGridThumbnailLayouter.cpp
     Sources/filemgmt/ptImageView.cpp
     Sources/filemgmt/ptPathBar.cpp
     Sources/filemgmt/ptRowGridThumbnailLayouter.cpp
     Sources/filemgmt/ptSingleDirModel.cpp
     Sources/filemgmt/ptTagList.cpp
     Sources/filemgmt/ptTagModel.cpp
     Sources/filemgmt/ptThumbCache.cpp
     Sources/filters/imagespot/ptFilter_SpotTuning.cpp
     Sources/filters/imagespot/ptImageSpot.cpp
     Sources/filters/imagespot/ptImageSpotEditor.cpp
     Sources/filters/imagespot/ptImageSpotItemDelegate.cpp
     Sources/filters/imagespot/ptImageSpotList.cpp
     Sources/filters/imagespot/ptImageSpotModel.cpp
     Sources/filters/imagespot/ptSpotInteraction.cpp
     Sources/filters/imagespot/ptSpotListWidget.cpp
     Sources/filters/imagespot/ptTuningSpot.cpp
     Sources/filters/ptCfgItem.cpp
     Sources/filters/ptFilter_ABCurves.cpp
     Sources/filters/ptFilter_Brightness.cpp
     Sources/filters/ptFilter_ColorBoost.cpp
     Sources/filters/ptFilter_ColorContrast.cpp
     Sources/filters/ptFilter_ColorEnhancement.cpp
     Sources/filters/ptFilter_ColorIntensity.cpp
     Sources/filters/ptFilter_DetailCurve.cpp
     Sources/filters/ptFilter_Drc.cpp
     Sources/filters/ptFilter_GammaTool.cpp
     Sources/filters/ptFilter_Highlights.cpp
     Sources/filters/ptFilter_LabTransform.cpp
     Sources/filters/ptFilter_Levels.cpp
     Sources/filters/ptFilter_LMHRecovery.cpp
     Sources/filters/ptFilter_LumaDenoiseCurve.cpp
     Sources/filters/ptFilter_LumaSatAdjust.cpp
     Sources/filters/ptFilter_Normalization.cpp
     Sources/filters/ptFilter_Outline.cpp
     Sources/filters/ptFilter_ReinhardBrighten.cpp
     Sources/filters/ptFilter_SatCurve.cpp
     Sources/filters/ptFilter_Saturation.cpp
     Sources/filters/ptFilter_ShadowsHighlights.cpp
     Sources/filters/ptFilter_SigContrast.cpp
     Sources/filters/ptFilter_StdCurve.cpp
     Sources/filters/ptFilter_Tone.cpp
     Sources/filters/ptFilter_ToneAdjust.cpp
     Sources/filters/ptFilter_Wiener.cpp
     Sources/filters/ptFilterBase.cpp
     Sources/filters/ptFilterConfig.cpp
     Sources/filters/ptFilterDM.cpp
     Sources/filters/ptFilterFactory.cpp
     Sources/ptAbstractInteraction.cpp
     Sources/ptCalloc.cpp
     Sources/ptChannelMixer.cpp
     Sources/ptCheck.cpp
     Sources/ptChoice.cpp
     Sources/ptCimg.cpp
     Sources/ptConfirmRequest.cpp
     Sources/ptCurve.cpp
     Sources/ptCurveWindow.cpp
     Sources/ptDcRaw.cpp
     Sources/ptError.cpp
     Sources/ptFastBilateral.cpp
     Sources/ptGridInteraction.cpp
     Sources/ptGroupBox.cpp
     Sources/ptGuiOptions.cpp
     Sources/ptHistogramWindow.cpp
     Sources/ptImage.cpp
     Sources/ptImage_Cimg.cpp
     Sources/ptImage_DRC.cpp
     Sources/ptImage_EAW.cpp
     Sources/ptImage_GM.cpp
     Sources/ptImage_GMC.cpp
     Sources/ptImage_Lensfun.cpp
     Sources/ptImage_Lqr.cpp
     Sources/ptImage_Pyramid.cpp
     Sources/ptImage8.cpp
     Sources/ptImageHelper.cpp
     Sources/ptInfo.cpp
     Sources/ptInput.cpp
     Sources/ptKernel.cpp
     Sources/ptLensfun.cpp
     Sources/ptLineInteraction.cpp
     Sources/ptMain.cpp
     Sources/ptMainWindow.cpp
     Sources/ptMessageBox.cpp
     Sources/ptParseCli.cpp
     Sources/ptProcessor.cpp
     Sources/ptReportOverlay.cpp
     Sources/ptResizeFilters.cpp
     Sources/ptRGBTemperature.cpp
     Sources/ptRichRectInteraction.cpp
     Sources/ptSettings.cpp
     Sources/ptSimpleRectInteraction.cpp
     Sources/ptSlider.cpp
     Sources/ptTempFile.cpp
     Sources/ptTempFilterBase.cpp
     Sources/ptTheme.cpp
     Sources/ptToolBox.cpp
     Sources/ptViewWindow.cpp
     Sources/ptVisibleToolsView.cpp
     Sources/ptWhiteBalances.cpp
     Sources/ptWidget.cpp
     Sources/ptWiener.cpp
     Sources/qtsingleapplication/qtlocalpeer.cpp
     Sources/qtsingleapplication/qtsingleapplication.cpp
     Sources/ptUtils_Storage.cpp
     Sources/ptStorable.cpp
     Sources/ptUtils.cpp
     Sources/filemgmt/ptThumbDefines.cpp
     Sources/ptMutexLocker.cpp
     Sources/filemgmt/ptThumbGenMgr.cpp
     Sources/filemgmt/ptThumbGenWorker.cpp
     Sources/filemgmt/ptThumbGenHelpers.cpp
)
 
if( WIN32 )
  set( photivo_SRCS
       ${photivo_SRCS}
       Sources/ptEcWin7.cpp
       Sources/ptWinApi.cpp
  )
endif( WIN32 )

set( photivo_UI_HDRS
     Sources/batch/ptBatchWindow.ui
     Sources/filemgmt/ptFileMgrWindow.ui
     Sources/filters/imagespot/ptFilter_SpotTuning.ui
     Sources/filters/imagespot/ptSpotListWidget.ui
     Sources/filters/ptFilter_ColorIntensity.ui
     Sources/filters/ptFilter_LMHRecovery.ui
     Sources/filters/ptFilter_Outline.ui
     Sources/filters/ptFilter_Tone.ui
     Sources/filters/ptFilter_Wiener.ui
     Sources/ptMainWindow.ui
)

set( photivo_RCCS
     qrc/photivo.qrc
)

QT4_WRAP_UI( photivo_UI_SRCS ${photivo_UI_HDRS} )
QT4_WRAP_CPP( photivo_MOC_SRCS ${photivo_MOC_HDRS} )
QT4_ADD_RESOURCES( photivo_RCC_SRCS ${photivo_RCCS} )

add_executable( photivo
                ${photivo_SRCS}
                ${photivo_RCC_SRCS}
                ${photivo_MOC_SRCS}
                ${photivo_UI_SRCS}
)

if( NOT ${CMAKE_SYSTEM_PROCESSOR} OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
  set_target_properties( photivo PROPERTIES COMPILE_FLAGS
                         "-ffast-math -DAPPVERSION='${APPVERSION}' -DPREFIX=${CMAKE_INSTALL_PREFIX}" )
else( NOT ${CMAKE_SYSTEM_PROCESSOR} OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
  set_target_properties( photivo PROPERTIES COMPILE_FLAGS
                         "-march=i686 -ffast-math -DAPPVERSION='${APPVERSION}' -DPREFIX=${CMAKE_INSTALL_PREFIX}" )
endif( NOT ${CMAKE_SYSTEM_PROCESSOR} OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )

set_target_properties( photivo PROPERTIES LINK_FLAGS "-lgomp" )

target_link_libraries( photivo
                       ${GLIB2_LIBRARIES}
                       ${QT_LIBRARIES}
                       ${JPEG_LIBRARIES}
                       ${EXIV2_LIBRARIES}
                       ${LCMS2_LIBRARIES}
                       ${LQR_LIBRARIES}
                       ${LENSFUN_LIBRARIES}
                       ${GM_LIBRARIES}
                       ${GMW_LIBRARIES}
                       ${FFTW3_LIBRARIES}
)

install( TARGETS   photivo         DESTINATION bin )
install( FILES     qrc/photivo-appicon.png DESTINATION share/pixmaps )
install( FILES     ReferenceMaterial/photivo.desktop DESTINATION share/applications )
install( DIRECTORY ChannelMixers   DESTINATION share/photivo )
install( DIRECTORY Curves          DESTINATION share/photivo )
install( DIRECTORY LensfunDatabase DESTINATION share/photivo )
install( DIRECTORY Presets         DESTINATION share/photivo )
install( DIRECTORY Profiles        DESTINATION share/photivo )
install( DIRECTORY Themes          DESTINATION share/photivo )
install( DIRECTORY Translations    DESTINATION share/photivo )
install( DIRECTORY UISettings      DESTINATION share/photivo )


if( ${WITH_CLEAR} )
  add_executable( ptClear Sources/ptClear.cpp )
  target_link_libraries( ptClear ${QT_LIBRARIES} )
  install( TARGETS ptClear DESTINATION bin )
endif( ${WITH_CLEAR} )


if( ${WITH_GIMP} )
  include_directories( ${GIMP_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS} )
  link_directories(    ${GIMP_LIBRARY_DIRS} ${GTK2_LIBRARY_DIRS} )
  add_executable( ptGimp Sources/ptCalloc.cpp
                         Sources/ptError.cpp
                         Sources/ptGimp.cpp )
  set_target_properties( ptGimp PROPERTIES COMPILE_FLAGS "-ffast-math" )
  target_link_libraries( ptGimp ${QT_LIBRARIES} ${GTK2_LIBRARIES} ${GIMP_LIBRARIES} )
  find_library( LIBGIMP gimp-2.0 )
  string( REGEX MATCH "lib[0-9]*" LIBPATH ${LIBGIMP} )
  install( TARGETS   ptGimp                DESTINATION ${LIBPATH}/gimp/2.0/plug-ins )
  install( PROGRAMS "mm extern photivo.py" DESTINATION ${LIBPATH}/gimp/2.0/plug-ins )
endif( ${WITH_GIMP} )


if( ${WITH_ADOBE_PROFILES} )
  add_executable( ptCreateAdobeProfiles Sources/ptCreateAdobeProfiles.cpp )
  target_link_libraries( ptCreateAdobeProfiles ${QT_LIBRARIES} ${LCMS2_LIBRARIES} )
  install( TARGETS ptCreateAdobeProfiles DESTINATION bin )
endif( ${WITH_ADOBE_PROFILES} )


# uninstall target
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                IMMEDIATE @ONLY )

add_custom_target( uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake )
