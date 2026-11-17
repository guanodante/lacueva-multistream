; Script de instalador NSIS para La Cueva MultiStream Plugin
!include "MUI2.nsh"

Name "La Cueva MultiStream - OBS Plugin"
OutFile "LaCuevaMultiStream_Installer_x64.exe"
InstallDir "$PROGRAMFILES64\obs-studio"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "Spanish"

Section "Plugin Principal" SecPlugin
    SetOutPath "$INSTDIR\obs-plugins\64bit"
    File "lacueva-multistream.dll"

    SetOutPath "$INSTDIR\data\obs-plugins\lacueva-multistream\locale"
    File /r "data\locale\*.*"
SectionEnd
