;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; photivo
;;
;; Copyright (C) 2010 Bernd Schoeler <brother.john@photivo.org>
;;
;; This file is part of photivo.
;;
;; photivo is free software: you can redistribute it and;or modify
;; it under the terms of the GNU General Public License version 3
;; as published by the Free Software Foundation.
;;
;; photivo is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with photivo.  If not, see <http:;;www.gnu.org;licenses;>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppID={{9B7CEA17-E1CC-43E1-A2F6-F36A34051539}
AppName=Photivo
AppVersion=DD MMM 20YY (???)
AppPublisherURL=http://photivo.org/
AppSupportURL=http://photivo.org/
AppUpdatesURL=http://code.google.com/p/photivo/downloads/list
DefaultDirName={pf}\Photivo
DefaultGroupName=Photivo
AllowNoIcons=yes
InfoBeforeFile=..\..\_bin-win64\Changelog.txt
OutputBaseFilename=photivo-setup--win64
Compression=lzma/Max
SolidCompression=false
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
ChangesAssociations=false
ShowLanguageDialog=no
LanguageDetectionMethod=none
DisableWelcomePage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "..\..\_bin-win64\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Photivo"; Filename: "{app}\photivo.exe"
Name: "{group}\{cm:ProgramOnTheWeb,Photivo}"; Filename: "http://photivo.org/"
Name: "{group}\{cm:UninstallProgram,Photivo}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Photivo"; Filename: "{app}\photivo.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Photivo"; Filename: "{app}\photivo.exe"; Tasks: quicklaunchicon
