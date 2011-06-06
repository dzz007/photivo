CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

EXECUTE_PROCESS(COMMAND /bin/sh -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/photivoProject; sh ./get_appversion" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE APPVERSION )

PROJECT(photivo)
find_package(PkgConfig REQUIRED)
find_package(JPEG REQUIRED) 
find_package(Qt4 REQUIRED) 
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDESIGNER 1)
include(${QT_USE_FILE})
pkg_check_modules(GLIB2 glib-2.0)
pkg_check_modules(LCMS2 lcms2)
pkg_check_modules(EXIV2 exiv2)
pkg_check_modules(LQR lqr-1)
pkg_check_modules(LENSFUN lensfun)
pkg_check_modules(GM GraphicsMagick++)
pkg_check_modules(GMW GraphicsMagickWand)
pkg_check_modules(GTK2 gtk+-2.0)
pkg_check_modules(GIMP gimp-2.0)
pkg_check_modules(FFTW3 fftw3)

ADD_DEFINITIONS("-DAPPVERSION=\"${APPVERSION}\"")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Wall -O2 -g -ffast-math -ftree-vectorize -fopenmp")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O4 -DNDEBUG -ffast-math -ftree-vectorize -fopenmp") 
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Wall -O2 -g -ffast-math -ftree-vectorize -fopenmp")
set(CMAKE_C_FLAGS_RELEASE "-Wall -O4 -DNDEBUG -ffast-math -ftree-vectorize -fopenmp")
set(CMAKE_L_FLAGS_RELWITHDEBINFO "-rdynamic")
set(CMAKE_L_FLAGS_DEBUG "-rdynamic")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${QT4_INCLUDES} ${GLIB2_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ${EXIV2_INCLUDE_DIRS}
                                 ${LCMS2_INCLUDE_DIRS} ${LQR_INCLUDE_DIRS} ${LENSFUN_INCLUDE_DIRS} ${GM_INCLUDE_DIRS} ${GMW_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS}
                                 ${GIMP_INCLUDE_DIRS} ${FFTW3_INCLUDE_DIRS} )
link_directories( ${GLIB2_LIBRARY_DIRS} ${JPEG_LIBRARY_DIRS} ${EXIV2_LIBRARY_DIRS} ${LCMS2_LIBRARY_DIRS} ${LQR_LIBRARY_DIRS} ${LENSFUN_LIBRARY_DIRS} ${GM_LIBRARY_DIRS}
                           ${GMW_LIBRARY_DIRS} ${GTK2_LIBRARY_DIRS} ${GIMP_LIBRARY_DIRS} ${FFTW3_LIBRARY_DIRS} )

set(photivo_MOC_HDRS  Sources/ptVisibleToolsView.h Sources/ptMainWindow.h Sources/ptMessageBox.h Sources/ptCurveWindow.h Sources/ptHistogramWindow.h Sources/ptViewWindow.h Sources/ptInput.h
  Sources/ptChoice.h Sources/ptCheck.h Sources/ptGroupBox.h Sources/ptSlider.h Sources/qtsingleapplication/qtsingleapplication.h Sources/qtsingleapplication/qtlocalpeer.h)

set(photivo_SRCS  Sources/ptCurve.cpp Sources/ptVisibleToolsView.cpp Sources/ptChannelMixer.cpp Sources/ptDcRaw.cpp Sources/ptError.cpp Sources/ptGuiOptions.cpp
  Sources/ptSettings.cpp Sources/ptImage.cpp Sources/ptImage_DRC.cpp Sources/ptImage_EAW.cpp Sources/ptImage_GM.cpp Sources/ptImage_GMC.cpp Sources/ptImage_Pyramid.cpp
  Sources/ptImage_Cimg.cpp Sources/ptImage_Lensfun.cpp Sources/ptImage_Lqr.cpp Sources/ptImage8.cpp Sources/ptResizeFilters.cpp Sources/ptKernel.cpp Sources/ptMain.cpp
  Sources/ptMainWindow.cpp Sources/ptMessageBox.cpp Sources/ptRGBTemperature.cpp Sources/ptWhiteBalances.cpp Sources/ptCurveWindow.cpp Sources/ptHistogramWindow.cpp Sources/ptRefocusMatrix.cpp
  Sources/ptViewWindow.cpp Sources/ptProcessor.cpp Sources/ptCimg.cpp Sources/ptFastBilateral.cpp Sources/ptWiener.cpp Sources/ptInput.cpp Sources/ptChoice.cpp Sources/ptCheck.cpp
  Sources/ptCalloc.cpp Sources/ptGroupBox.cpp Sources/ptSlider.cpp Sources/ptTheme.cpp Sources/qtsingleapplication/qtsingleapplication.cpp Sources/qtsingleapplication/qtlocalpeer.cpp
  Sources/clapack/xerbla.c Sources/clapack/open.c Sources/clapack/ieeeck.c Sources/clapack/dtrsm.c Sources/clapack/util.c Sources/clapack/wsfe.c
  Sources/clapack/s_cmp.c Sources/clapack/dgetrs.c Sources/clapack/ptaswp.c Sources/clapack/ilaenv.c Sources/clapack/s_stop.c Sources/clapack/dgemm.c Sources/clapack/s_copy.c
  Sources/clapack/abort_.c Sources/clapack/dgesv.c Sources/clapack/dgetrf.c Sources/clapack/fmtlib.c Sources/clapack/sig_die.c Sources/clapack/idamax.c Sources/clapack/close.c
  Sources/clapack/dger.c Sources/clapack/lsame.c Sources/clapack/dscal.c Sources/clapack/fmt.c Sources/clapack/dswap.c Sources/clapack/endfile.c Sources/clapack/wref.c
  Sources/clapack/dgetf2.c Sources/clapack/err.c Sources/clapack/wrtfmt.c Sources/clapack/sfe.c
#Sources/qtsingleapplication/qtlockedfile.cpp
#Sources/dcb/dcb_demosaicing.c Sources/dcb/dcb_demosaicing_old.c  Sources/vcd/ahd_interpolate_mod.c Sources/vcd/ahd_partial_interpolate.c
#Sources/vcd/es_median_filter.c Sources/vcd/median_filter_new.c Sources/vcd/refinement.c
#Sources/vcd/vcd_interpolate.c Sources/perfectraw/lmmse_interpolate.c Sources/rawtherapee/amaze_interpolate.c Sources/rawtherapee/cfa_line_dn.c Sources/rawtherapee/ca_correct.c
#Sources/rawtherapee/green_equil.c
)

set(photivo_UI_HDRS Sources/ptMainWindow.ui)

set(photivo_RCCS qrc/photivo.qrc)

QT4_WRAP_UI(photivo_UI_SRCS ${photivo_UI_HDRS} )
QT4_WRAP_CPP(photivo_MOC_SRCS ${photivo_MOC_HDRS} )
QT4_ADD_RESOURCES(photivo_RCC_SRCS ${photivo_RCCS} )

add_executable(photivo ${photivo_SRCS} ${photivo_RCC_SRCS} ${photivo_MOC_SRCS} ${photivo_UI_SRCS} )
add_executable(ptClear Sources/ptClear.cpp )
add_executable(ptCreateAdobeProfiles Sources/ptCreateAdobeProfiles.cpp )
add_executable(ptCreateCurves Sources/ptCreateSomeCurves.cpp Sources/ptCurve.cpp Sources/ptError.cpp Sources/ptCalloc.cpp )
add_executable(ptGimp Sources/ptCalloc.cpp Sources/ptError.cpp Sources/ptGimp.cpp )

target_link_libraries(photivo ${GLIB2_LIBRARIES} ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${EXIV2_LIBRARIES} ${LCMS2_LIBRARIES} ${LQR_LIBRARIES} ${LENSFUN_LIBRARIES} ${GM_LIBRARIES} ${GMW_LIBRARIES} ${FFTW3_LIBRARIES} )
target_link_libraries(ptClear ${QT_LIBRARIES} )
target_link_libraries(ptCreateAdobeProfiles ${QT_LIBRARIES} ${LCMS2_LIBRARIES} )
target_link_libraries(ptCreateCurves ${QT_LIBRARIES} )
target_link_libraries(ptGimp ${QT_LIBRARIES} ${GTK2_LIBRARIES} ${GIMP_LIBRARIES} )

file(GLOB_RECURSE ChannelMixer "ChannelMixers/*")
file(GLOB_RECURSE Curves "Curves/*")
file(GLOB_RECURSE LensFun "LensfunDatabase/*")
file(GLOB_RECURSE Presets "Presets/*")
file(GLOB_RECURSE Profiles "Profiles/*")
file(GLOB_RECURSE Translations "Translations/*")

install(TARGETS photivo DESTINATION bin)
install(TARGETS ptClear DESTINATION bin)
install(FILES qrc/photivo.png DESTINATION share/pixmaps)
install(FILES qrc/photivo.png photivoLogo.png DESTINATION share/photivo)
install(FILES ReferenceMaterial/photivo.desktop DESTINATION share/applications)
install(FILES ${ChannelMixer} DESTINATION share/photivo/ChannelMixers)
install(FILES ${Curves} DESTINATION share/photivo/Curves)
install(FILES ${LensFun} DESTINATION share/photivo/LensfunDatabase)
install(FILES ${Presets} DESTINATION share/photivo/Presets)
install(FILES ${Profiles} DESTINATION share/photivo/Profiles)
install(FILES ${Translations} DESTINATION share/photivo/Translations)
install(FILES UISettings/default.ptu DESTINATION share/photivo/UISettings)
