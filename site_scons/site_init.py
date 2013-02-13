################################################################################
##
## Photivo
##
## Copyright (C) 2013 Jos De Laender <jos@de-laender.be>
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

import tempfile
import subprocess
import shutil

################################################################################

# Constants.

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

# Do we have colors in win32 ?
ptHaveColors = True
if sys.platform in ['win32'] :
  ptHaveColors = False
  try:
    from colorama import init
    init()
    ptHaveColors = True
  except :
    print '\nTIP : Installing colorama would give you coloured output.\n'
    pass

if ptHaveColors and sys.stdout.isatty() :

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

  ptPrintLog(True,BuildEnv['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for GCC >= ' + MinVersion + ' ... ')

  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in BuildEnv['ENV'].keys():
    os.environ[Key] = BuildEnv['ENV'][Key]

  ptCC  = BuildEnv['CC']
  ptCXX = BuildEnv['CXX']
  ptCCVersion  = os.popen(ptCC  + ' -dumpversion').read().rstrip().split('.')
  ptCXXVersion = os.popen(ptCXX + ' -dumpversion').read().rstrip().split('.')

  # Restpre env
  os.environ.clear()
  os.environ.update(ptSavedEnviron)

  ptMinVersion = MinVersion.split('.')
  if (ptCCVersion[0] > ptMinVersion[0] and ptCXXVersion[0] > ptMinVersion[0]):
    return True;
  if (ptCCVersion[0] < ptMinVersion[0] or ptCXXVersion[0] < ptMinVersion[0]):
    return False;
  if (ptCCVersion[1] > ptMinVersion[1] and ptCXXVersion[1] > ptMinVersion[1]):
    return True;
  if (ptCCVersion[1] < ptMinVersion[1] or ptCXXVersion[1] < ptMinVersion[1]):
    return False;
  # Some report 4.7 for 4.7.2
  try:
    if (ptCCVersion[2] > ptMinVersion[2] and ptCXXVersion[2] > ptMinVersion[2]):
      return True;
    if (ptCCVersion[2] < ptMinVersion[2] or ptCXXVersion[2] < ptMinVersion[2]):
      return False;
  except IndexError:
    pass
  return True;

################################################################################

# Get GCC/CXX version

def ptGetGCCVersion(BuildEnv) :

  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in BuildEnv['ENV'].keys():
    os.environ[Key] = BuildEnv['ENV'][Key]

  ptCC  = BuildEnv.WhereIs(BuildEnv['CC'])
  ptCXX = BuildEnv.WhereIs(BuildEnv['CXX'])
  ptCCVersion  = os.popen(ptCC  + ' -dumpversion').read().rstrip()
  ptCXXVersion = os.popen(ptCXX + ' -dumpversion').read().rstrip()

  # Restore env
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

  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in Context.env['ENV'].keys():
    os.environ[Key] = Context.env['ENV'][Key]

  ptPkgConfig = Context.env['PT_CROSS'] + 'pkg-config'
  ptVersion = os.popen(ptPkgConfig + ' --modversion ' + Name).read().rstrip()
  ptFlags   = os.popen(ptPkgConfig + ' --cflags --libs ' + Name).read().rstrip()

  # Restore env
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

  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for ' + Name + '... ')

  ptPkgConfig = Context.env['PT_CROSS'] + 'pkg-config'
  if sys.platform in ['win32'] :
    ptCommand = ptPkgConfig + ' --exists %s' % Name
    # WIN32 shell escape of >
    ptCommand = ptCommand.replace(">","^>")
  else :
    ptCommand = ptPkgConfig + ' --exists \'%s\'' % Name
  
  Ret = Context.TryAction(ptCommand)[0]

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

def ptCheckQt(Context,MinVersion):

  ptPrintLog(True,Context.env['PT_LOGFILE'],
             ptBoldBlue,
             'Checking for Qt >= ' + MinVersion + '... ')

  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in Context.env['ENV'].keys():
    os.environ[Key] = Context.env['ENV'][Key]

  # Locate qmake. Taking QT4DIR into account.

  qmake_1 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake'
  qmake_2 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake.exe'
  qmake_3 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake-qt4'
  qmake_4 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake-qt4.exe'

  qmakes = [qmake_1,qmake_2,qmake_3,qmake_4]

  qmake = ''
  for qm in qmakes : 
    if os.path.exists(qm) :
      qmake = qm
    break
 
  if not qmake:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'Cannot locate qmake.')
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,'Giving up.')
    Exit(1)

  # Locate make

  make = Context.env.WhereIs('make') 
  if not make:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'Cannot locate make.')
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,'Giving up.')
    Exit(1)

  # Check version

  ptQtVersion = \
      os.popen(qmake + ' -query QT_VERSION').read().rstrip().split('.')
  ptQtMinVersion = MinVersion.split('.')

  if ptQtVersion[0] < ptQtMinVersion[0] :
    Context.Result(False)
    return False
  if ptQtVersion[0] > ptQtMinVersion[0] :
    Context.Result(True)
    return True

  if ptQtVersion[1] < ptQtMinVersion[1] :
    Context.Result(False)
    return False
  if ptQtVersion[1] > ptQtMinVersion[1] :
    Context.Result(True)
    return True

  if ptQtVersion[2] < ptQtMinVersion[2] :
    Context.Result(False)
    return False
  if ptQtVersion[2] > ptQtMinVersion[2] :
    Context.Result(True)
    return True

  Context.Result(True)
  return True
    
################################################################################

# Determine Qt Compile and Link parameters via a qmake run on test program.

def ptGetQtOutput(Context):

  # Make sure we work with correct and minimum os.environ. Save previous.
  ptSavedEnviron = dict(os.environ)
  os.environ.clear()
  for Key in Context.env['ENV'].keys():
    os.environ[Key] = Context.env['ENV'][Key]
 
  # Locate qmake. Taking QT4DIR into account.

  qmake_1 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake'
  qmake_2 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake.exe'
  qmake_3 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake-qt4'
  qmake_4 = Context.env['QT4DIR'] + os.sep + 'bin' + os.sep + 'qmake-qt4.exe'

  qmakes = [qmake_1,qmake_2,qmake_3,qmake_4]

  qmake = ''
  for qm in qmakes : 
    if os.path.exists(qm) :
      qmake = qm
    break
 
  if not qmake:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'Cannot locate qmake.')
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,'Giving up.')
    Exit(1)

  # Locate make

  make = Context.env.WhereIs('make') 
  if not make:
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
               'Cannot locate make.')
    ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,'Giving up.')
    Exit(1)

  # Version
  ptQtVersion = os.popen(qmake + ' -query QT_VERSION').read().rstrip()

  # Analyze output of qmake/make combo
  ptCurDir = os.getcwd()
  ptTmpDir = tempfile.mkdtemp()

  ptProgram = """
      int main() {
        return 0;
      }
      """

  with open(ptTmpDir + os.sep + 'FooTest.cpp','w') as f :
    f.write(ptProgram)

  with open(ptTmpDir + os.sep + 'FooTest.pro','w') as f :
    f.write('CONFIG -= DEBUG\n')
    f.write('CONFIG -= RELEASE\n')
    if Context.env['PT_RELEASE'] :
      f.write('CONFIG += RELEASE\n')
    else :
      f.write('CONFIG += DEBUG\n')
    f.write('QT += core\n')
    f.write('QT += gui\n')
    f.write('QT += network\n')
    f.write('SOURCES = FooTest.cpp\n')

  os.chdir(ptTmpDir)
  os.popen(qmake)
  ptMakeOutput = os.popen(make).read().rstrip().split('\n')

  # Analyze output to determine flags.
  ptCompileFlags = ''
  ptLinkFlags    = ''

  for Line in ptMakeOutput:

    if 'FooTest.cpp' in Line :

      # Assuming compile step.
      #ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
      #          '\nFoo compile line : ' + Line + '\n')

      ptLocalEnv = Environment()
      ptParsedFlags = ptLocalEnv.ParseFlags(Line)
      for ptFlag in ptParsedFlags['CPPDEFINES'] :
        ptCompileFlags += ' ' + '-D' + ptFlag
      for ptFlag in ptParsedFlags['CCFLAGS'] :
        if ptFlag.startswith('-f') or ptFlag.startswith('-m'):
          ptCompileFlags += ' ' + ptFlag
      for ptPath in ptParsedFlags['CPPPATH'] :
        if ptPath in ['.','debug','release'] :
          continue
        ptCompileFlags += ' -I' + os.path.abspath(ptPath).replace("\\","/")

      #ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
      #          '\nptParsedFlags : ' + str(ptParsedFlags) + '\n')
      #ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
      #          '\nptCompileFlags : ' + ptCompileFlags + '\n')

    elif 'FooTest' in Line :

      # Assuming link step.
      #ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
      #         '\nFoo link line : ' + Line + '\n')

      ptLocalEnv = Environment()
      ptParsedFlags = ptLocalEnv.ParseFlags(Line)
      for ptFlag in ptParsedFlags['LINKFLAGS'] :
        ptLinkFlags += ' ' + ptFlag
      for ptFlag in ptParsedFlags['CCFLAGS'] :
        if ptFlag.startswith('-f') or ptFlag.startswith('-m'):
          ptLinkFlags += ' ' + ptFlag
      for ptPath in ptParsedFlags['LIBPATH'] :
        ptLinkFlags += ' -L' + os.path.abspath(ptPath).replace("\\","/")
      for ptLib in ptParsedFlags['LIBS'] :
        try:
          ptLinkFlags += ' -l' + ptLib
        except TypeError:
          # foo.exe,foo.o references.
          pass
  
      #ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
      #          '\nptParsedFlags : ' + str(ptParsedFlags) + '\n')
      #ptPrintLog(True,Context.env['PT_LOGFILE'],ptBoldRed,
      #         '\nptLinkFlags : ' + ptLinkFlags + '\n')


  # Back to dir we were.
  os.chdir(ptCurDir)

  # Remove our temp dir.
  shutil.rmtree(ptTmpDir)

  # Restore env
  os.environ.clear()
  os.environ.update(ptSavedEnviron)

  return [ptQtVersion,ptCompileFlags + ' ' + ptLinkFlags]

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
  elif s.endswith('.lnk') :
    ShortText = ptBoldMagenta + 'Linking'
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
  try:
    LogFile.write('\nThe stderr output is :\n')
    sys.stderr.flush() # Make sure the stderr is complete.
    StdErrFile = open('stderr.log','r')
    LogFile.write(StdErrFile.read())
    StdErrFile.close()
  except:
    pass
  return None

################################################################################

# Basically from Scons wiki : Spawn which echos stdout/stderr from the child.
# ptFoo is mine.

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

# Local (changed) copy of TempFileMunge

class ptTempFileMunge(object):

  """A callable class.  You can set an Environment variable to this,
  then call it with a string argument, then it will perform temporary
  file substitution on it.  This is used to circumvent the long command
  line limitation.

  Example usage:
  env["TEMPFILE"] = TempFileMunge
  env["LINKCOM"] = "${TEMPFILE('$LINK $TARGET $SOURCES')}"

  By default, the name of the temporary file used begins with a
  prefix of '@'.  This may be configred for other tool chains by
  setting '$TEMPFILEPREFIX'.

  env["TEMPFILEPREFIX"] = '-@'        # diab compiler
  env["TEMPFILEPREFIX"] = '-via'      # arm tool chain
  """

  def __init__(self, cmd):
    self.cmd = cmd

  def __call__(self, target, source, env, for_signature):
    if for_signature:
      # If we're being called for signature calculation, it's
      # because we're being called by the string expansion in
      # Subst.py, which has the logic to strip any $( $) that
      # may be in the command line we squirreled away.  So we
      # just return the raw command line and let the upper
      # string substitution layers do their thing.
      return self.cmd

    # Now we're actually being called because someone is actually
    # going to try to execute the command, so we have to do our
    # own expansion.

    cmd = env.subst_list(self.cmd, SCons.Subst.SUBST_CMD, target, source)[0]
    try:
      maxline = int(env.subst('$MAXLINELENGTH'))
    except ValueError:
      maxline = 2048

    length = 0
    for c in cmd:
      length += len(c)
    if length <= maxline:
      return self.cmd

    # We do a normpath because mktemp() has what appears to be
    # a bug in Windows that will use a forward slash as a path
    # delimiter.  Windows's link mistakes that for a command line
    # switch and barfs.
    #
    # We use the .lnk suffix for the benefit of the Phar Lap
    # linkloc linker, which likes to append an .lnk suffix if
    # none is given.
    (fd, tmp) = tempfile.mkstemp('.lnk', text=True)
    native_tmp = SCons.Util.get_native_path(os.path.normpath(tmp))
 
    if env['SHELL'] and env['SHELL'] == 'sh':
      # The sh shell will try to escape the backslashes in the
      # path, so unescape them.
      native_tmp = native_tmp.replace('\\', r'\\\\')
      # In Cygwin, we want to use rm to delete the temporary
      # file, because del does not exist in the sh shell.
      rm = env.Detect('rm') or 'del'
    else:
      # Don't use 'rm' if the shell is not sh, because rm won't
      # work with the Windows shells (cmd.exe or command.com) or
      # Windows path names.
      rm = 'del'

    prefix = env.subst('$TEMPFILEPREFIX')
    if not prefix:
      prefix = '@'

    # JDLA , Another round of escapes for win32, which is in msys in our case.
    if sys.platform in ['win32'] :
      for i,ptCmd in enumerate(cmd) :
        cmd[i] = ptCmd.replace('\\','\\\\')

    args = list(map(SCons.Subst.quote_spaces, cmd[1:]))
    os.write(fd, " ".join(args) + "\n")
    os.close(fd)
    # XXX Using the SCons.Action.print_actions value directly
    # like this is bogus, but expedient.  This class should
    # really be rewritten as an Action that defines the
    # __call__() and strfunction() methods and lets the
    # normal action-execution logic handle whether or not to
    # print/execute the action.  The problem, though, is all
    # of that is decided before we execute this method as
    # part of expanding the $TEMPFILE construction variable.
    # Consequently, refactoring this will have to wait until
    # we get more flexible with allowing Actions to exist
    # independently and get strung together arbitrarily like
    # Ant tasks.  In the meantime, it's going to be more
    # user-friendly to not let obsession with architectural
    # purity get in the way of just being helpful, so we'll
    # reach into SCons.Action directly.

    #if SCons.Action.print_actions:
    if False :
      print("Using tempfile "+native_tmp+" for command line:\n"+
            str(cmd[0]) + " " + " ".join(args))

    return [ cmd[0], prefix + native_tmp + '\n' + rm, native_tmp ]

################################################################################
