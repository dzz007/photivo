# Copyright (C) 2009 by Leopold Palomo-Avellaneda                       #
# leo@alaxarxa.net                                                      #
#                                                                       #
# This program is free software; you can redistribute it and/or modify  #
# it under the terms of the GNU General Public License as published by  #
# the Free Software Foundation; either version 2 of the License, or     #
# (at your option) any later version.                                   #
#                                                                       #
# This program is distributed in the hope that it will be useful,       #
# but WITHOUT ANY WARRANTY; without even the implied warranty of        #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
# GNU General Public License for more details.                          #
#                                                                       #
# You should have received a copy of the GNU General Public License     #
# along with this program; if not, write to the                         #
# Free Software Foundation, Inc.,                                       #
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
#                                                                       #

###########################################
# photivo                                   #
###########################################

set(photivo_gui ptMainWindow.cpp
	   ptMain.cpp)

set(photivo_lib ptCurve.cpp 
           ptChannelMixer.cpp 
           ptError.cpp 
           ptGuiOptions.cpp 
           ptGuiSettings.cpp 
           ptImage.cpp 
           ptImage8.cpp 
           ptKernel.cpp 
           ptResizeFilters.cpp 
           ptRememberDialog.cpp 
           ptRGBTemperature.cpp 
           ptWhiteBalances.cpp 
           ptCurveWindow.cpp
           ptHistogramWindow.cpp 
           ptRefocusMatrix.cpp 
           ptViewWindow.cpp
           ptLensfun.cpp
           ptProcessor.cpp 
           ptGreycStoration.cpp)


set(photivo_dcraw ptDcRaw.cpp)

set(photivo_clapack clapack/xerbla.c 
           clapack/open.c 
           clapack/ieeeck.c 
           clapack/dtrsm.c 
           clapack/util.c 
           clapack/wsfe.c 
           clapack/s_cmp.c 
           clapack/dgetrs.c 
           clapack/ptaswp.c 
           clapack/ilaenv.c 
           clapack/s_stop.c 
           clapack/dgemm.c 
           clapack/s_copy.c 
           clapack/abort_.c 
           clapack/dgesv.c 
           clapack/dgetrf.c 
           clapack/fmtlib.c 
           clapack/sig_die.c 
           clapack/idamax.c 
           clapack/close.c 
           clapack/dger.c 
           clapack/lsame.c 
           clapack/dscal.c 
           clapack/fmt.c 
           clapack/dswap.c 
           clapack/endfile.c 
           clapack/wref.c 
           clapack/dgetf2.c 
           clapack/err.c 
           clapack/wrtfmt.c 
           clapack/sfe.c)


set(photivo_lensfun lensfun/auxfun.cpp
           lensfun/camera.cpp
           lensfun/database.cpp
           lensfun/lens.cpp
           lensfun/mod-color.cpp
           lensfun/mod-coord.cpp
           lensfun/mod-subpix.cpp
           lensfun/modifier.cpp
           lensfun/mount.cpp)

set(photivo_UIS ptMainWindow.ui
           ptRememberDialog.ui)

set (photivo_H ptConstants.h 
           ptCurve.h
           ptChannelMixer.h 
           ptDcRaw.h 
           ptDefines.h 
           ptError.h 
           ptGuiOptions.h 
           ptGuiSettings.h 
           ptImage.h 
           ptImage8.h 
           ptResizeFilters.h 
           ptKernel.h 
           ptMainWindow.h 
           ptRememberDialog.h 
           ptRGBTemperature.h 
           ptWhiteBalances.h 
           ptCurveWindow.h 
           ptChannelMixerSettings.h
           ptHistogramWindow.h 
           ptAdobeTable.h 
           ptRefocusMatrix.h 
           ptViewWindow.h
           ptLensfun.h
           ptProcessor.h
           ptGreycStoration.h
           greyc/CImg_128.h 
           greyc/CImg_130.h 
           clapack/clapack.h 
           clapack/fp.h 
           clapack/fmt.h 
           clapack/fio.h 
           clapack/blaswrap.h 
           lensfun/lensfun.h 
           lensfun/lensfunprv.h)


set(photivo_MOCS 
           ptMainWindow.h 
           ptCurveWindow.h 
           ptRememberDialog.h
           ptHistogramWindow.h 
           ptViewWindow.h)

set(photivo_RCCS ../photivo.qrc)

set(QT_USE_QTDESIGNER true)
set(QT_USE_QTMAIN true)


INCLUDE( ${QT_USE_FILE} )

include_directories(
	${QT_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}
	${EXIV2_INCLUDE_DIR}
	${LCMS_INCLUDE_DIR}
   ${GLIB2_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})

# generate rules for building source files that moc generates
QT4_WRAP_UI(photivo_UIS_H ${photivo_UIS} )
QT4_WRAP_CPP(photivo_MOC_SRCS ${photivo_MOCS})
QT4_ADD_RESOURCES(photivo_RCC_SRCS ${photivo_RCCS})

add_executable(photivo ${photivo_gui} ${photivo_RCCS}
         ${photivo_UIS_H} ${photivo_dcraw} 
         ${photivo_MOC_SRCS} ${ptraw_MOCS} ${photivo_lib}
         ${photivo_clapack} ${photivo_lensfun})


target_link_libraries( photivo ${QT_LIBRARIES} ${JPEG_LIBRARIES}
         ${EXIV2_LIBRARIES} ${LCMS_LIBRARIES} ${GLIB2_LIBRARIES})



