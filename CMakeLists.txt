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


# ########## Project setup ##########



PROJECT(photivo)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.8)
set(DLRAW_VERSION_MAJOR 0)
set(DLRAW_VERSION_MINOR 03)

# Differences between CMake 2.4 and 2.6
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.4)
   message(STATUS "Using cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
   # some developers may be using an cmake cvs version which didn't have set_property() yet
   # Tell them that a more recent version is required.
   if(NOT COMMAND SET_PROPERTY)
      message(FATAL_ERROR "You are using an old version of CMake from cvs, please update to CMake >= 2.6.0 or cvs at least from Feb 20th, 2008")
   endif(NOT COMMAND SET_PROPERTY)


   # CMake 2.6 gives errors if there are multiple targets with the same name
   # we use this for the target "buildtests", which is created for the unit tests
   # and which depends on the tests, so building "buildtests" builds all the tests
   # enabling this property disables this check in CMake
   set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
    
   # CMP0002: in KDE4 we have multiple targets with the same name for the unit tests
   cmake_policy(SET CMP0002 OLD)
   # CMP0003: add the link paths to the link command as with cmake 2.4
   cmake_policy(SET CMP0003 OLD)
   # CMake 2.6 uses the full file names of the libraries when linking and so 
   # doesn't use -L anymore to list the link dirs. Now the dependencies created
   # export_library_dependencies() lists the in-project libraries without 
   # path, i.e. with only the logical name ("kdecore"), so they don't link
   # Setting this variable to true has the effect that the link dirs are
   # listed nevertheless also with CMake 2.6.
   set(CMAKE_LINK_OLD_PATHS TRUE)

endif("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.4)


# ######### General setup ##########

find_package(Qt4 REQUIRED) 
if(NOT QT4_FOUND)
  message(FATAL_ERROR "photivo needs QT4 to compile")
endif(NOT QT4_FOUND)
# and now the version string given by qmake
STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" QT_MAJOR_VERSION "${QTVERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+.*" "\\1" QT_MINOR_VERSION "${QTVERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" QT_PATCH_VERSION "${QTVERSION}")

if (${QT_MAJOR_VERSION}.${QT_MINOR_VERSION} LESS 4.4)
    message(FATAL_ERROR " photivo needs >= QT4.4 to compile")
endif (${QT_MAJOR_VERSION}.${QT_MINOR_VERSION} LESS 4.4)

#Searching libjpeg stuff
find_package(JPEG REQUIRED) 
if(NOT JPEG_FOUND)
  message(FATAL_ERROR "photivo needs libjpeg to compile")
endif(NOT JPEG_FOUND)
#  JPEG_INCLUDE_DIR, where to find jpeglib.h, etc.
#  JPEG_LIBRARIES, the libraries needed to use JPEG.
#  JPEG_FOUND, If false, do not try to use JPEG.
# also defined, but not for general use are
#  JPEG_LIBRARY, where to find the JPEG library.

#Searching Exiv2 stuff
include (${CMAKE_CURRENT_SOURCE_DIR}/ModulesCMake/FindExiv2.cmake)
if(NOT EXIV2_FOUND)
  message(FATAL_ERROR "photivo needs exiv2 to compile")
else(NOT EXIV2_FOUND)
  add_definitions (${EXIV2_DEFINITIONS})
endif(NOT EXIV2_FOUND)
#  EXIV2_FOUND - system has libexiv2
#  EXIV2_INCLUDE_DIR - the libexiv2 include directory
#  EXIV2_LIBRARIES - Link these to use libexiv2
#  EXIV2_DEFINITIONS

#Searching lcms stuff
include (${CMAKE_CURRENT_SOURCE_DIR}/ModulesCMake/FindLCMS.cmake)
if(NOT LCMS_FOUND)
  message(FATAL_ERROR "photivo needs lcms to compile")
endif(NOT LCMS_FOUND)

#  LCMS_INCLUDE_DIR, where to find lcms.h
#  LCMS_LIBRARIES, the libraries needed to use LCMS.
#  LCMS_VERSION, The value of LCMS_VERSION defined in lcms.h
#  LCMS_FOUND, If false, do not try to use LCMS.


#Searching glib2 stuff
include (${CMAKE_CURRENT_SOURCE_DIR}/ModulesCMake/FindGLIB2.cmake)
if(NOT GLIB2_FOUND)
  message(FATAL_ERROR "photivo needs glib2 to compile .. lensfun need it")
endif(NOT GLIB2_FOUND)

#  GLIB2_FOUND - system has glib2
#  GLIB2_INCLUDE_DIR - the glib2 include directory
#  GLIB2_LIBRARIES - glib2 library


###############################################

#For the future ...
#INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/ModulesCMake/FindGettext.cmake)

#Some stuff
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O4 -DNDEBUG -ffast-math") 
set(CMAKE_C_FLAGS_RELEASE "-Wall -O4 -DNDEBUG -ffast-math")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Wall -O2 -g -ffast-math")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Wall -O2 -g -ffast-math")

add_subdirectory(Sources)

#Taking care of the OSX port
if(APPLE)
    message(STATUS "Compiling for OSX")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif(APPLE)

