Steps to create the Windows troubleshooter archive:

1. Compile Photivo 32bit with console output, i.e. uncomment the line
     CONFIG += console
   in "photivoProject\photivoProject.pro"

2. Same for Photivo 64bit.

3. Rename the two executables ptConsole32.exe and ptConsole64.exe

4. Create a Zip archive containing:
   - ptConsole32.exe
   - ptConsole64.exe
   - ptTroubleshoot.bat  (troubleshooting script from this folder)
   - How to use.txt  (user instructions file from this folder)
