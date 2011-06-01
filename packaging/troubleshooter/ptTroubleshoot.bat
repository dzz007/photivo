@echo off
cls

set ptLogFile="%HOMEDRIVE%%HOMEPATH%\Photivo-console.log"

echo Photivo Troubleshooter
echo ----------------------
echo Before you continue go to
echo http://code.google.com/p/photivo/downloads/list
echo and make sure you are using the latest version of Photivo and
echo this troubleshooter.
echo.
echo I will now attempt to start Photivo.
echo If you need to do anything in Photivo to make it crash, do it.
echo Then come back to this window for further instructions.
echo.
pause
echo.
echo Running Photivo ...

if exist pthreadGC2_64.dll (
  echo 64 bit Photivo detected. > %ptLogFile%
  ptConsole64.exe >> %ptLogFile% 2>&1
) else (
  echo 32 bit Photivo detected. > %ptLogFile%
  ptConsole32.exe >> %ptLogFile% 2>&1
)

explorer "%HOMEDRIVE%%HOMEPATH%"

echo.
echo Photivo exited or crashed.
echo I created the file "Photivo-console.log" in your user folder that contains
echo all of Photivo's console output. An Explorer windows should have opened
echo showing that folder. If not, find the logfile here:
echo %ptLogFile%
echo.
echo Now contact the Photivo developers via one of the ways mentioned on
echo http://photivo.org/photivo/feedback
echo and attach the logfile or post its full content. Please also include a
echo step by step description of what you did that crashed Photivo and any
echo error messages that maybe appears on screen.
echo.
echo Press a key to close this window.
pause>NUL

set ptLogFile=
