################################################################################
##
## Photivo
##
## Copyright (C) 2008-2013 Jos De Laender <jos@de-laender.be>
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

import os
import fnmatch
import sys
import glob
import atexit

################################################################################

# Minimum requirements;

ptMinGCCVersion     = '4.6.0'
ptMinQtVersion      = '4.7.0'
ptMinGMVersion      = '1.3.12'
ptMinGMWandVersion  = ptMinGMVersion
ptMinExiv2Version   = '0.19'
ptMinLcms2Version   = '2.1'
ptMinGlib2Version   = '2.18'
ptMinLensfunVersion = '0.2.5'
ptMinFftw3Version   = '3.2.2'
ptMinLqr1Version    = '0.4.1'
ptMinGimp20Version  = '2.6.10' # only when gimp plugin

# Custom libjpeg checks. Has no pkg-config equivalent.
ptMinLibJpegVersion = 62
ptMaxLibJpegVersion = 79

################################################################################

# Clean exit and exit logging.
atexit.register(ptLastCalledAtExit)

################################################################################

# Announce ourselves as the build program.

print ''
print ptBoldYellow                                            + \
      'This is the scons build program for Photivo.\n'      + \
      'Copyright (C) 2013 Jos De Laender <jos@de-laender.be>' + \
      ptNoAttrs;
print ''

# Help, options and variables boiler plate.

HelpText = '''
Usage : scons [-Q] [--ptVerbose] [--ptVerboseConfig] [--ptBuildConfFile=FILE] [install]

  -Q                : Quiet about reading/building progress messages.
                      (default : not quiet)
  --ptVerbose       : Verbose output of progress during compile.
                      (default : not verbose)
  --ptVerboseConfig : Verbose output of progress during config.
                      (default : not verbose)
  --ptBuildConfFile : File that describes the build parameters.
                      (default = DefaultBuild.py) 
  install           : Install in directory (defined by PT_INSTALL_PATH)
'''

Help(HelpText)

AddOption('--ptBuildConfFile',
          dest    = 'ptBuildConfFile',
          type    = 'string',
          nargs   = 1,
          action  = 'store',
          metavar = 'FILE',
          default = 'BuildConfs/DefaultBuild.py')

AddOption('--ptVerbose',
          dest    = 'ptVerbose',
          action  = 'store_true',
          default = False)
   
AddOption('--ptVerboseConfig',
          dest    = 'ptVerboseConfig',
          action  = 'store_true',
          default = False)
   
ptBuildConfFile = GetOption('ptBuildConfFile')
ptVerbose       = GetOption('ptVerbose')
ptVerboseConfig = GetOption('ptVerboseConfig')

print ptBoldCyan                                                      + \
      'Reading build configuration from \'' + ptBuildConfFile + '\''  + \
      ptNoAttrs

# Use of simple file input (without 'Variables()' and command line input)
# enables a simpler and more correct guessing of values in more
# complex cases of local qt, gcc, etc ..

if not os.path.exists(ptBuildConfFile): 
  print ptBoldRed                                      + \
        'No such ptBuildConfFile : ' , ptBuildConfFile , \
        ptNoAttrs
  print ptNoAttrs + HelpText
  Exit(1)

ptValidOptions = ['CC',
                  'CXX',
                  'PT_BUILD_CONF_NAME',
                  'PT_CROSS',
                  'PT_HOST_PLATFORM',
                  'PT_INSTALL_MODE',
                  'PT_INSTALL_PATH',
                  'PT_LOGFILE_NAME',
                  'PT_OMP',
                  'PT_RELEASE',
                  'PT_TARGET_PLATFORM',
                  'PT_TOOLS_DIR',
                  'PT_WITH_CONSOLE',
                  'PT_WITH_FILEMGR',
                  'PT_WITH_GIMPPLUGIN',
                  'PT_WITH_SYSTEMCIMG',
                  'PKG_CONFIG_PATH',
                  'QT4DIR']

# Defaults.
ptBuildValues = {'PT_BUILD_CONF_NAME' : 'Build',
                 'PT_CROSS'           : '',
                 'PT_INSTALL_MODE'    : 'Original',
                 'PT_INSTALL_PATH'    : '/opt/Photivo',
                 'PT_OMP'             : True,
                 'PT_RELEASE'         : True,
                 'PT_WITH_FILEMGR'    : False,
                 'PT_WITH_GIMPPLUGIN' : False,
                 'PT_WITH_SYSTEMCIMG' : False,
                 'PT_WITH_CONSOLE'    : False}

# Read them from file
exec open(ptBuildConfFile, 'rU').read() in {}, ptBuildValues

#for key,value in ptBuildValues.items():
#  print key + ' => ' + str(value)

# A default environment to start from.
ptDefaultEnv  = Environment()

# Do we have CC and CXX ?
if (ptDefaultEnv['CC'] == None) :
  print ptBoldRed + 'CC not defined' + ptNoAttrs
  print ptBoldRed + 'Giving up' + ptNoAttrs
  Exit(1)
if (ptDefaultEnv['CXX'] == None) :
  print ptBoldRed + 'CXX not defined' + ptNoAttrs
  print ptBoldRed + 'Giving up' + ptNoAttrs
  Exit(1)

# For later reference. The unaltered one.
ptDefaultEnv['PT_DEFAULT_PATH'] = ptDefaultEnv['ENV']['PATH']

# Throw everything that we recognize in the environment, overwriting.
for ptBuildKey,ptBuildValue in ptBuildValues.items():

  if ptBuildKey in ptValidOptions:
    ptDefaultEnv[ptBuildKey] = ptBuildValues[ptBuildKey]
  else:
    print ptBoldRed                           + \
          'No such option : ' + ptBuildKey    + \
          ' while reading ' + ptBuildConfFile ,\
          ptNoAttrs
    print ptNoAttrs + HelpText
    Exit(1)

# QT4DIR (name compatible with qt4 tool) via qmake if not yet in environment.
if not 'QT4DIR' in ptDefaultEnv: 
  ptEnv = Environment(ENV = os.environ)
  qmake = ptEnv.WhereIs('qmake') or ptEnv.WhereIs('qmake-qt4')
  if qmake:
    ptDefaultEnv['QT4DIR'] = os.path.dirname(os.path.dirname(qmake))
  else :
    print ptBoldRed          + \
          'No QT4DIR found.' , \
          ptNoAttrs
    Exit(1)

# Check QT4DIR (user can have given wrong one)
if not os.path.isdir(ptDefaultEnv['QT4DIR']):
  print ptBoldRed                                                 + \
        'QT4DIR (' + ptDefaultEnv['QT4DIR'] + ') does not exist.' , \
        ptNoAttrs
  Exit(1)

# PT_TOOLS_DIR detection. If not yet in environment.
if not 'PT_TOOLS_DIR' in ptDefaultEnv:
  cc = ptDefaultEnv.WhereIs(ptDefaultEnv['CC'])
  if cc:
    ptDefaultEnv['PT_TOOLS_DIR'] = os.path.dirname(cc)
  else :
    print ptBoldRed                + \
          'No PT_TOOLS_DIR found.' , \
          ptNoAttrs
    Exit(1)

# Check PT_TOOLS_DIR (user can have given wrong one)
if not os.path.isdir(ptDefaultEnv['PT_TOOLS_DIR']):
  print ptBoldRed                                       + \
        'PT_TOOLS_DIR (' + ptDefaultEnv['PT_TOOLS_DIR'] + \
        ') does not exist.'                             , \
        ptNoAttrs
  Exit(1)

# PT_LOGFILE_NAME
if not 'PT_LOGFILE_NAME' in ptDefaultEnv:
  ptDefaultEnv['PT_LOGFILE_NAME'] = ptDefaultEnv['PT_BUILD_CONF_NAME'] + '.log'

# Check PT_INSTALL_PATH
if not os.path.isdir(ptDefaultEnv['PT_INSTALL_PATH']):
  print ptBoldRed                                             + \
        'PT_INSTALL_PATH (' + ptDefaultEnv['PT_INSTALL_PATH'] + \
        ') does not exist.' ,                                   \
        ptNoAttrs
  Exit(1)

# Target and host platform. Normally platform.
if not 'PT_TARGET_PLATFORM' in ptDefaultEnv:
  ptDefaultEnv['PT_TARGET_PLATFORM'] = ptDefaultEnv['PLATFORM']

if not ptDefaultEnv['PT_TARGET_PLATFORM'] in ['posix','win32']:
  print ptBoldRed                                                   + \
        'PT_TARGET_PLATFORM (' + ptDefaultEnv['PT_TARGET_PLATFORM'] + \
        ') should be \'posix\' or \'win32\'.'                       , \
        ptNoAttrs
  Exit(1)

if not 'PT_HOST_PLATFORM' in ptDefaultEnv:
  ptDefaultEnv['PT_HOST_PLATFORM'] = ptDefaultEnv['PLATFORM']

if not ptDefaultEnv['PT_HOST_PLATFORM'] in ['posix','win32']:
  print ptBoldRed                                               + \
        'PT_HOST_PLATFORM (' + ptDefaultEnv['PT_HOST_PLATFORM'] + \
        ') should be \'posix\' or \'win32\'.'                   , \
        ptNoAttrs
  Exit(1)

################################################################################

# Opening of LogFile

if not ptDefaultEnv['PT_LOGFILE_NAME']:
  ptDefaultEnv['PT_LOGFILE_NAME'] = ptDefaultEnv['PT_BUILD_CONF_NAME'] + '.log'

ptLogFile = open(ptDefaultEnv['PT_LOGFILE_NAME'],'w',1) # Line buffered
ptDefaultEnv['PT_LOGFILE'] = ptLogFile

# I hope to duplicate compile errors (via stderr) into the log this way.
sys.stderr = os.popen('tee stderr.log','w')

atexit.register(ptAtExit,ptLogFile)

################################################################################

# Some extra derived environment.

# Spawn with stdout/stderr echoing from the child.
ptDefaultEnv['SPAWN'] = ptEchoSpawn

# Needed for above.
ptDefaultEnv['PT_VERBOSE']       = ptVerbose
ptDefaultEnv['PT_VERBOSECONFIG'] = ptVerboseConfig

# Extend the CC/CXX names for cross. XXX JDLA More might be needed. TODO
ptDefaultEnv['CC']   = ptDefaultEnv['PT_CROSS'] + ptDefaultEnv['CC']
ptDefaultEnv['CXX']  = ptDefaultEnv['PT_CROSS'] + ptDefaultEnv['CXX']

# Extend PATH with the found PT_TOOLS_DIR
ptDefaultEnv['ENV']['PATH'] = \
  ptDefaultEnv['PT_TOOLS_DIR'] + ':' + ptDefaultEnv['ENV']['PATH']

# Add or extend PKG_CONFIG_PATH 
# Assuming that it is only needed if QT4DIR is 'non standard'
ptQtBin =  os.path.join(str(ptDefaultEnv['QT4DIR']),'bin')
if not ptQtBin in ptDefaultEnv['PT_DEFAULT_PATH']:
  # print "NON DEFAULT QT4DIR"
  ptPkgConfigPath = \
    os.path.join(os.path.join(str(ptDefaultEnv['QT4DIR']),'lib'),'pkgconfig')
  if not 'PKG_CONFIG_PATH' in ptDefaultEnv['ENV']:
    ptDefaultEnv['ENV']['PKG_CONFIG_PATH'] = ptPkgConfigPath
  else :
    ptDefaultEnv['ENV']['PKG_CONFIG_PATH'] = \
        ptDefaultEnv['ENV']['PKG_CONFIG_PATH'] + ':' + ptPkgConfigPath

################################################################################

# Options summary so far.

ptDoPrint = False
if ptVerboseConfig:
  ptDoPrint = True

ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'Configuration file   : ' + ptBuildConfFile)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'CC                   : ' + str(ptDefaultEnv['CC']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'CXX                  : ' + str(ptDefaultEnv['CXX']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_BUILD_CONF_NAME   : ' + str(ptDefaultEnv['PT_BUILD_CONF_NAME']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_CROSS             : ' + str(ptDefaultEnv['PT_CROSS']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_HOST_PLATFORM     : ' + str(ptDefaultEnv['PT_HOST_PLATFORM']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_INSTALL_PATH      : ' + str(ptDefaultEnv['PT_INSTALL_PATH']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_LOGFILE_NAME      : ' + str(ptDefaultEnv['PT_LOGFILE_NAME']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_OMP               : ' + str(ptDefaultEnv['PT_OMP']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_RELEASE           : ' + str(ptDefaultEnv['PT_RELEASE']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_TARGET_PLATFORM   : ' + str(ptDefaultEnv['PT_TARGET_PLATFORM']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_TOOLS_DIR         : ' + str(ptDefaultEnv['PT_TOOLS_DIR']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_WITH_CONSOLE      : ' + str(ptDefaultEnv['PT_WITH_CONSOLE']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_WITH_FILEMGR      : ' + str(ptDefaultEnv['PT_WITH_FILEMGR']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_WITH_GIMPPLUGIN   : ' + str(ptDefaultEnv['PT_WITH_GIMPPLUGIN']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'PT_WITH_SYTSTEMCIMG  : ' + str(ptDefaultEnv['PT_WITH_SYSTEMCIMG']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'ENV[PATH]            : ' + str(ptDefaultEnv['ENV']['PATH']))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
    'ENV[PKG_CONFIG_PATH] : ' + str(ptDefaultEnv['ENV'].get('PKG_CONFIG_PATH')))
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'QT4DIR               : ' + str(ptDefaultEnv['QT4DIR']))

################################################################################

if ptDefaultEnv['PT_TARGET_PLATFORM'] == 'win32' :
  ptDefaultEnv['PROGSUFFIX'] = '.exe'

################################################################################

# Minimum compiler version check.

if not ptCheckGCCVersion(ptDefaultEnv,ptMinGCCVersion) : 
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'GCC >= ' + ptMinGCCVersion + ' not found.')
  ptVersion = ptGetGCCVersion(ptDefaultEnv)
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found GCC :  ' +  ptVersion[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found G++ :  ' +  ptVersion[1])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

################################################################################

# Check for libraries pkg-config and Qt version.

ptConf = Configure(ptDefaultEnv, 
                   custom_tests = 
                       {'ptCheckPKGConfig'     : ptCheckPKGConfig,
                        'ptCheckPKG'           : ptCheckPKG ,
                        'ptCheckLibWithHeader' : ptCheckLibWithHeader,
                        'ptCheckHg'            : ptCheckHg,
                        'ptCheckLibJpeg'       : ptCheckLibJpeg,
                        'ptGetPKGOutput'       : ptGetPKGOutput})

# hg check
if not ptConf.ptCheckHg():
  ptPrintLog(True,ptLogFile,ptBoldRed,'Mercurial (hg) not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

# Version we are building
ptAppVersion = ptGetAppVersion()

# jpeg check. Note header file might be tricky and need tweak !
if not ptConf.ptCheckLibWithHeader('jpeg','jpeglib.h','cxx'):
  ptPrintLog(True,ptLogFile,ptBoldRed,'Library jpeg (or jpeglib.h) not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'It is not unusual you have to add \n'
             '"#include <stdlib.h>" and "#include <stdio.h>" \n'
             'to your "jpeglib.h".')
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

# Additional custom test on jpeg lib version.
# TODO Check doesn't work for CROSS (can't execute it on host ..)
if ptDefaultEnv['PT_TARGET_PLATFORM'] == ptDefaultEnv['PT_HOST_PLATFORM'] :
  if not ptConf.ptCheckLibJpeg(ptMinLibJpegVersion,ptMaxLibJpegVersion):
    ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
    Exit(1)

# jasper (jpeg2000) check.
if not ptConf.ptCheckLibWithHeader('jasper','jasper/jasper.h','cxx'):
  ptPrintLog(True,ptLogFile,ptBoldRed,'Library jasper not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

# png check.
if not ptConf.ptCheckLibWithHeader('png','png.h','cxx'):
  ptPrintLog(True,ptLogFile,ptBoldRed,'Library png not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

# tiff check.
if not ptConf.ptCheckLibWithHeader('tiff','tiff.h','cxx'):
  ptPrintLog(True,ptLogFile,ptBoldRed,'Library tiff not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

# pkg-config check. (does 'cross' behind the scenes).
if not ptConf.ptCheckPKGConfig('0.25'):
  ptPrintLog(True,ptLogFile,ptBoldRed,'pkg-config >= 0.25 not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)

# lensfun check.
if not ptConf.ptCheckPKG('lensfun >= ' + ptMinLensfunVersion):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'lensfun >= ' + ptMinLensfunVersion + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('lensfun')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptLensfunVersionString,ptLensfunFlags] = ptConf.ptGetPKGOutput('lensfun')

# fftw3 check.
if not ptConf.ptCheckPKG('fftw3 >= ' + ptMinFftw3Version):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'fftw3 >= ' + ptMinFftw3Version + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('fftw3')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptFftw3VersionString,ptFftw3Flags] = ptConf.ptGetPKGOutput('fftw3')

# lqr-1 check.
if not ptConf.ptCheckPKG('lqr-1 >= ' + ptMinLqr1Version):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'lqr-1 >= ' + ptMinLqr1Version + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('lqr-1')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptLqr1VersionString,ptLqr1Flags] = ptConf.ptGetPKGOutput('lqr-1')

# glib-2.0 check.
if not ptConf.ptCheckPKG('glib-2.0 >= ' + ptMinGlib2Version):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'glib-2.0 >= ' + ptMinGlib2Version + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('glib-2.0')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptGlib2VersionString,ptGlib2Flags] = ptConf.ptGetPKGOutput('glib-2.0')

# exiv2 check.
if not ptConf.ptCheckPKG('exiv2 >= ' + ptMinExiv2Version):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'exiv2 >= ' + ptMinExiv2Version + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('exiv2')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptExiv2VersionString,ptExiv2Flags] = ptConf.ptGetPKGOutput('exiv2')

# lcms2 check.
if not ptConf.ptCheckPKG('lcms2 >= ' + ptMinLcms2Version):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'lcms2 >= ' + ptMinLcms2Version + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('lcms2')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptLcms2VersionString,ptLcms2Flags] = ptConf.ptGetPKGOutput('lcms2')

# GraphicsMagick check.
if not ptConf.ptCheckPKG('GraphicsMagick++ >= ' + ptMinGMVersion):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'Magick++ >= ' + ptMinGMVersion + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('GraphicsMagick++')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptGMVersionString,ptGMFlags] = ptConf.ptGetPKGOutput('GraphicsMagick++')

# GraphicsMagickWand check.
if not ptConf.ptCheckPKG('GraphicsMagickWand >= ' + ptMinGMWandVersion):
  ptPrintLog(True,ptLogFile, ptBoldRed,
             'MagickWand >= ' + ptMinGMWandVersion + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('GraphicsMagickWand')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptGMWandVersionString,ptGMWandFlags] = ptConf.ptGetPKGOutput('GraphicsMagickWand')

# QT check.
if not ptConf.ptCheckPKG('QtCore >= ' + ptMinQtVersion):
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'QtCore >= ' + ptMinQtVersion + ' not found.')
  ptPrintLog(True,ptLogFile,ptBoldRed,
             'Found :  ' +  ptConf.ptGetPKGOutput('QtCore')[0])
  ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
  Exit(1)
else :
  [ptQtVersionString,ptQtFlags] = ptConf.ptGetPKGOutput('QtCore')

# libgimp check in case we are working with GIMPPLUGIN
if ptDefaultEnv['PT_WITH_GIMPPLUGIN']:
  if not ptConf.ptCheckPKG('gimp-2.0 >= ' + ptMinGimp20Version):
    ptPrintLog(True,ptLogFile, ptBoldRed,
               'gimp-2.0 >= ' + ptMinGimp20Version + ' not found.')
    ptPrintLog(True,ptLogFile,ptBoldRed,
               'Found :  ' +  ptConf.ptGetPKGOutput('gimp-2.0')[0])
    ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
    Exit(1)
  else :
    [ptGimp20VersionString,ptGimp20Flags] = ptConf.ptGetPKGOutput('gimp-2.0')

# Some functions check.
if ptConf.CheckFunc('getc_unlocked'):
  ptConf.env.Append(CPPDEFINES = ['-DHAVE_GETC_UNLOCKED'])
if ptConf.CheckFunc('ftello'):
  ptConf.env.Append(CPPDEFINES = ['-DHAVE_FTELLO'])

# Version defines.
ptConf.env.Append(CPPDEFINES = ['-DAPPVERSION=\'' + ptAppVersion + '\''])

# Prefix defines.
ptConf.env.Append(CPPDEFINES = \
    ['-DPREFIX=\'' + ptDefaultEnv['PT_INSTALL_PATH'] + '\''])

# System CIMG
if ptDefaultEnv['PT_WITH_SYSTEMCIMG']:
  ptConf.env.Append(CPPDEFINES = ['-DSYSTEM_CIMG'])

# FileMgr
if not ptDefaultEnv['PT_WITH_FILEMGR']:
  ptConf.env.Append(CPPDEFINES = ['-DPT_WITHOUT_FILEMGR'])

# XXX CHECKME.
if ptDefaultEnv['PT_TARGET_PLATFORM'] == 'win32':
  if not ptConf.CheckLib('gnurx'):
    ptPrintLog(True,ptLogFile,ptBoldRed,'Library gnurx not found.')
    ptPrintLog(True,ptLogFile,ptBoldRed,'Giving up.')
    Exit(1)
  ptConf.env.Append(LIBS = ['gnurx'])
  
# Finalize configuration
ptConf.Finish()

# Show summary results.
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'lensfun version      : ' + ptLensfunVersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'fftw3 version        : ' + ptFftw3VersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'lqr-1 version        : ' + ptLqr1VersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'glib-2.0 version     : ' + ptGlib2VersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'exiv2 version        : ' + ptExiv2VersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'lcms2 version        : ' + ptLcms2VersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'GM version           : ' + ptGMVersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'GM Wand version      : ' + ptGMWandVersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'Qt version           : ' + ptQtVersionString)
if ptDefaultEnv['PT_WITH_GIMPPLUGIN']:
  ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
             'Gimp20 version       : ' + ptGimp20VersionString)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'lensfun flags        : ' + ptLensfunFlags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'fftw3 flags          : ' + ptFftw3Flags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'lqr-1 flags          : ' + ptLqr1Flags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'glib-2.0 flags       : ' + ptGlib2Flags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'exiv2 flags          : ' + ptExiv2Flags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'lcms2 flags          : ' + ptLcms2Flags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'GM flags             : ' + ptGMFlags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'GM Wand flags        : ' + ptGMWandFlags)
ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
           'Qt flags             : ' + ptQtFlags)
if ptDefaultEnv['PT_WITH_GIMPPLUGIN']:
  ptPrintLog(ptDoPrint,ptLogFile,ptBoldMagenta,
             'Gimp20 flags         : ' + ptGimp20Flags)

################################################################################

# Parse all the flags collected up to now.

ptParsedLensfunFlags = ptDefaultEnv.ParseFlags(ptLensfunFlags)
ptDefaultEnv.MergeFlags(ptParsedLensfunFlags)

ptParsedFftw3Flags = ptDefaultEnv.ParseFlags(ptFftw3Flags)
ptDefaultEnv.MergeFlags(ptParsedFftw3Flags)

ptParsedLqr1Flags = ptDefaultEnv.ParseFlags(ptLqr1Flags)
ptDefaultEnv.MergeFlags(ptParsedLqr1Flags)

ptParsedGlib2Flags = ptDefaultEnv.ParseFlags(ptGlib2Flags)
ptDefaultEnv.MergeFlags(ptParsedGlib2Flags)

ptParsedExiv2Flags = ptDefaultEnv.ParseFlags(ptExiv2Flags)
ptDefaultEnv.MergeFlags(ptParsedExiv2Flags)

ptParsedLcms2Flags = ptDefaultEnv.ParseFlags(ptLcms2Flags)
ptDefaultEnv.MergeFlags(ptParsedLcms2Flags)

ptParsedGMFlags = ptDefaultEnv.ParseFlags(ptGMFlags)
ptDefaultEnv.MergeFlags(ptParsedGMFlags)

ptParsedGMWandFlags = ptDefaultEnv.ParseFlags(ptGMWandFlags)
ptDefaultEnv.MergeFlags(ptParsedGMWandFlags)

# qt4 tool (later) will do also, but I guess compatible.
ptParsedQtFlags = ptDefaultEnv.ParseFlags(ptQtFlags)
ptDefaultEnv.MergeFlags(ptParsedQtFlags)

if ptDefaultEnv['PT_WITH_GIMPPLUGIN']:
  ptParsedGimp20Flags = ptDefaultEnv.ParseFlags(ptGimp20Flags)
  ptDefaultEnv.MergeFlags(ptParsedGimp20Flags)

################################################################################

# Command printing via a wrapper function for decorating and logging.
# After the configure checks, in order not to pollute the log.

ptDefaultEnv['PRINT_CMD_LINE_FUNC'] = ptPrintCmdLine

################################################################################

# Pure for scons printing recognition.

ptDefaultEnv.Append(CXXFLAGS  = ['-DSCONS_CXX'])
ptDefaultEnv.Append(CCFLAGS   = ['-DSCONS_CC'])
ptDefaultEnv.Append(LINKFLAGS = ['-DSCONS_LINK'])

################################################################################

ptDefaultEnv.Append(CXXFLAGS = ['-std=gnu++0x'])
ptDefaultEnv.Append(CCFLAGS  = ['-ffast-math'])
ptDefaultEnv.Append(CCFLAGS  = ['-Wall'])
ptDefaultEnv.Append(CCFLAGS  = ['-Werror'])
ptDefaultEnv.Append(CCFLAGS  = ['-Wextra'])

if ptDefaultEnv['PT_OMP']:
  ptDefaultEnv.Append(CCFLAGS  = ['-fopenmp'])
  ptDefaultEnv.Append(LIBS     = ['gomp','pthread'])

if ptDefaultEnv['PT_RELEASE'] == True:
  ptDefaultEnv.Append(CCFLAGS  = ['-O3'])
  ptDefaultEnv.Append(CCFLAGS  = ['-funroll-loops', '-ftree-vectorize'])
  ptDefaultEnv.Append(CCFLAGS  = ['-DQT_NO_DEBUG'])
  ptDefaultEnv.Append(LINKFLAGS = ['-Wl,-O1'])
else:
  ptDefaultEnv.Append(CCFLAGS  = ['-O1'])
  ptDefaultEnv.Append(CCFLAGS  = ['-g'])
  ptDefaultEnv.Append(CCFLAGS  = ['-DQT_DEBUG'])

if ptDefaultEnv['PT_TARGET_PLATFORM'] == 'win32':
  if ptDefaultEnv['PT_WITH_CONSOLE'] == True:
    ptDefaultEnv.Append(LINKFLAGS = ['-Wl,-subsystem,console'])
  else:
    ptDefaultEnv.Append(LINKFLAGS = ['-Wl,-subsystem,windows'])

################################################################################

# Make a qt4 env. 
# XXX JDLA TODO Not fully understood why needed : in any
# case when not doing so, .qrc (rcc) fails to be recognized ...

ptQtEnv = ptDefaultEnv.Clone();
ptQtEnv.Tool('qt4')

################################################################################

# Subsidiary scripts in a variant build.

SConscript(os.path.join('Sources','SConscript'),   
           variant_dir = os.path.join('Build',
                         os.path.join(ptDefaultEnv['PT_BUILD_CONF_NAME'],
                         'Build_Photivo')), 
           exports     = 'ptQtEnv')

################################################################################

# Install

if ptDefaultEnv['PT_INSTALL_MODE'] == 'Original' :

  ptOrgList = []
  ptTgtList = []

  # binaries.
  if ptDefaultEnv['PT_TARGET_PLATFORM'] == 'posix':
    ptOrgList += ['photivo']
    ptOrgList += ['ptClear']
    ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + '/bin/photivo']
    ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + '/bin/ptclear']
  if ptDefaultEnv['PT_TARGET_PLATFORM'] == 'win32':
    ptBinOrgList += ['photivo.exe']
    ptBinOrgList += ['ptClear.exe']
    ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + '/bin/photivo.exe']
    ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + '/bin/ptclear.exe']

  # desktop. (twice : also in .local
  ptOrgList += ['ReferenceMaterial/photivo.desktop']
  ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + \
               '/share/applications/photivo.desktop']
  ptOrgList += ['ReferenceMaterial/photivo.desktop']
  ptTgtList += ['~/.local/share/applications/photivo.desktop']

  # icon.
  ptOrgList += ['qrc/photivo-appicon.png']
  ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + \
               '/share/pixmap/photivo-appicon.png']
  # Curves etc ..
  for Dir in ['Curves','ChannelMixers','Presets','Profiles','Translations',
              'LensfunDatabase','UISettings','Themes']:
    for Root,DirNames,FileNames in os.walk(Dir):
      for FileName in FileNames:
        ptOrgList += [os.path.join(Root,FileName)]
        ptTgtList += [ptDefaultEnv['PT_INSTALL_PATH'] + \
                     '/share/photivo/' + os.path.join(Root,FileName)]

  if ptDefaultEnv['PT_HOST_PLATFORM']   == 'posix' and \
     ptDefaultEnv['PT_TARGET_PLATFORM'] == 'posix' :
    ptDefaultEnv.Alias('install',ptDefaultEnv['PT_INSTALL_PATH'])
    ptDefaultEnv.InstallAs(ptTgtList,ptOrgList)

################################################################################
