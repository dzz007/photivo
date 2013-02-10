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

# Constants.

ptNoAttrs    = '\033[0m'
ptBold       = '\033[1m'
ptNegative   = '\033[7m'
ptBlack      = '\033[30m'
ptRed        = '\033[31m'
ptGreen      = '\033[32m'
ptYellow     = '\033[33m'
ptBlue       = '\033[34m'
ptMagenta    = '\033[35m'
ptCyan       = '\033[36m'
ptWhite      = '\033[37m'
ptBoldRed    = '\033[1;31m'
ptBoldGreen  = '\033[1;32m'
ptBoldYellow = '\033[1;33m'
ptBoldBlue   = '\033[1;34m'
ptBoldMagenta= '\033[1;35m'
ptBoldCyan   = '\033[1;36m'
ptBoldWhite  = '\033[1;37m'

if not sys.stdout.isatty():
  ptNoAttrs    = ''
  ptBold       = ''
  ptNegative   = ''
  ptBlack      = ''
  ptRed        = ''
  ptGreen      = ''
  ptYellow     = ''
  ptBlue       = ''
  ptMagenta    = ''
  ptCyan       = ''
  ptWhite      = ''
  ptBoldRed    = ''
  ptBoldGreen  = ''
  ptBoldYellow = ''
  ptBoldBlue   = ''
  ptBoldMagenta= ''
  ptBoldCyan   = ''
  ptBoldWhite  = ''

################################################################################

# Prints to screen and to log. With color to screen.

def ptPrintLog(DoPrint,LogFile,Color,Message):
  if DoPrint :
    MyMessage = Color + Message + ptNoAttrs
    print MyMessage
  LogFile.write(Message + '\n')
  return None

################################################################################

# Check for sufficient GCC. Simply returns true if OK.

def ptCheckGCCVersion(BuildEnv,MinVersion):
  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in BuildEnv['ENV'].keys():
    os.environ[Key] = BuildEnv['ENV'][Key]
  ptCC  = BuildEnv['CC']
  ptCXX = BuildEnv['CXX']
  ptCCVersion  = os.popen(ptCC  + ' -dumpversion').read().rstrip().split('.')
  ptCXXVersion = os.popen(ptCXX + ' -dumpversion').read().rstrip().split('.')
  os.environ.clear()
  os.environ.update(ptSavedEnviron)
  #print 'FooCC : "'  + str(ptCCVersion)  + '"'
  #print 'FooCXX : "' + str(ptCXXVersion) + '"'
  ptMinVersion = MinVersion.split('.')
  if (ptCCVersion[0] < ptMinVersion[0] or ptCXXVersion[0] < ptMinVersion[0]):
    return False;
  if (ptCCVersion[1] < ptMinVersion[1] or ptCXXVersion[1] < ptMinVersion[1]):
    return False;
  # Some report 4.7 for 4.7.2
  try:
    if (ptCCVersion[2] < ptMinVersion[2] or ptCXXVersion[2] < ptMinVersion[2]):
      return False;
  except IndexError:
    pass
  return True;

################################################################################

# Get GCC/CXX version

def ptGetGCCVersion(BuildEnv):
  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in BuildEnv['ENV'].keys():
    os.environ[Key] = BuildEnv['ENV'][Key]
  ptCC  = BuildEnv['CC']
  ptCXX = BuildEnv['CXX']
  ptCCVersion  = os.popen(ptCC  + ' -dumpversion').read().rstrip()
  ptCXXVersion = os.popen(ptCXX + ' -dumpversion').read().rstrip()
  os.environ.clear()
  os.environ.update(ptSavedEnviron)
  return[ptCCVersion,ptCXXVersion]

################################################################################

# Check hg

def ptCheckHg(Context):
  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for hg ... ')
  Ret=Context.TryAction('hg')[0]
  Context.Result(Ret)
  return Ret

################################################################################

# Get AppVersion

def ptGetAppVersion():
  ptHgRev   = os.popen('hg identify').read()[:11]
  ptChanged = os.popen('hg identify').read()[12]
  ptAppVer  = os.popen(
      'hg log --rev ' + ptHgRev + \
      ' --template "{date|shortdate} (rev {node|short})"').read()
  return ptAppVer + ptChanged

################################################################################

# Get the package version and flags for packages handled by pkg-config

def ptGetPKGOutput(Context,Name):
  ptPkgConfig = Context.env['PT_CROSS'] + 'pkg-config'
  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in Context.env['ENV'].keys():
    os.environ[Key] = Context.env['ENV'][Key]
  ptVersion = os.popen(ptPkgConfig + ' --modversion ' + Name).read().rstrip()
  ptFlags   = os.popen(ptPkgConfig + ' --cflags --libs ' + Name).read().rstrip()
  os.environ.clear()
  os.environ.update(ptSavedEnviron)
  return [ptVersion,ptFlags]

################################################################################

def ptCheckPKGConfig(Context,MinVersion):
  ptPkgConfig = Context.env['PT_CROSS'] + 'pkg-config'
  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for ' + ptPkgConfig + ' ... ')
  Ret=Context.TryAction(
      ptPkgConfig + ' --atleast-pkgconfig-version=' + MinVersion)[0]
  Context.Result(Ret)
  return Ret

################################################################################

def ptCheckPKG(Context,Name):
  ptPkgConfig = Context.env['PT_CROSS'] + 'pkg-config'
  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for ' + Name + '... ')
  Ret = Context.TryAction(ptPkgConfig + ' --exists \'%s\'' % Name)[0]
  Context.Result(Ret)
  return Ret

################################################################################

def ptCheckLibWithHeader(Context,Lib,Header,Language):
  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for ' + Lib + ' (' + Header + ')... ')
  Ret = Context.sconf.CheckLibWithHeader(Lib,Header,Language)
  Context.Result(Ret)
  return Ret

################################################################################

# custom check on libjpeg version

def ptCheckLibJpeg(Context,MinVersion,MaxVersion):
  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for libjpeg between ' + str(MinVersion) +
             ' and ' + str(MaxVersion) + '... ')
  ptProgram = """
#include <stdlib.h>
#include <stdio.h>
#define JPEG_LIB_VERSION 0 
#include <jpeglib.h>
int main() {
    printf("%d",JPEG_LIB_VERSION);
    return 0;
}
"""
  Ret = Context.TryCompile(ptProgram, '.c')
  if Ret == 0:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'Failing test. Cannot compile test program:')
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,ptProgram)
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,'Giving up.')
    Exit(1)
  Ret = Context.TryRun(ptProgram, '.c')
  if Ret[0] == 0:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'Failing test. Cannot run test program:')
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,ptProgram)
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,'Giving up.')
    Exit(1)
  ptVersion = int(Ret[1])
  OK = not (ptVersion < MinVersion or ptVersion > MaxVersion)
  if not OK:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'libjpeg version : ' + str(Ret[1]) + ' should be between ' +
               str(MinVersion) + ' and ' + str(MaxVersion))
  Context.Result(OK)
  return OK

################################################################################

# Boilerplate to log commands nicely to screen and completely to log file.

def ptPrintCmdLine(s, target, src, env):

  # Always to a log file. (and with an extra linefeed to 'see' commands)
  LogFile = env['PT_LOGFILE']
  LogFile.write('\n' + s + '\n')

  ShortText = 'Building object'
  # 'Recognized' commands ?
  if 'DSCONS_CXX' in s:
    ShortText = ptGreen + 'Building CXX object'
  elif 'DSCONS_CC' in s:
    ShortText = ptGreen + 'Building C object'
  elif 'DSCONS_LINK' in s:
    ShortText = ptBoldMagenta + 'Linking'
  elif 'DSCONS_UIC' in s:
    ShortText = ptBoldBlue + 'Generating UIC object'
  elif 'DSCONS_MOC' in s:
    ShortText = ptBoldBlue + 'Generating MOC object'
  elif 'DSCONS_RCC' in s:
    ShortText = ptBoldBlue + 'Generating RCC object'
  elif 'DSCONS_WINDRES' in s:
    ShortText = ptBoldBlue + 'Generating Windows resource'
  elif s.startswith('Creating'):
    ShortText = ptBoldBlue + 'Creating'
  else:
    # Install is a kind of exception. Also it points to a func.
    # We *assume* fallthrough is install. But that's a very shaky one.
    # XXX TODO
    #print 'DEBUG FOO : ' + s
    ShortText = ptBoldMagenta + 'Creating'
  MyMessage = ''
  if not env['PT_VERBOSE']:
    MyMessage = ShortText + ' ' + ' and '.join([str(x) for x in target])
  else:
    MyMessage = s
  MyMessage += ptNoAttrs
  print MyMessage
  return None

################################################################################

# Exit function ensures color reset.

def ptLastCalledAtExit():
  print ptBoldYellow                                 + \
        'Bye from the scons build program for Photivo' + \
        ptNoAttrs
  return None

################################################################################

# AtExit that joins the stderr collected in stderr.log into the logfile.

def ptAtExit(LogFile):
  LogFile.write('\nThe stderr output is :\n')
  sys.stderr.flush() # Make sure the stderr is complete.
  StdErrFile = open('stderr.log','r')
  LogFile.write(StdErrFile.read())
  StdErrFile.close()
  return None

################################################################################

# Basically from Scons wiki : Spawn which echos stdout/stderr from the child.
# ptFoo is mine.

import subprocess
def ptEchoSpawn( sh, escape, cmd, args, env ):

  ptFoo = ' '.join(args)
    
  # convert env from unicode strings
  asciienv = {}
  for key, value in env.iteritems():
    asciienv[key] = str(value)    
      
  p = subprocess.Popen(
    #args, 
    ptFoo,
    env=asciienv, 
    stderr=subprocess.PIPE,
    stdout=subprocess.PIPE,
    shell=True,
    universal_newlines=True)
  (stdout, stderr) = p.communicate()

  # Does this screw up the relative order of the two?
  sys.stdout.write(stdout)
  sys.stderr.write(stderr)
  return p.returncode

################################################################################
