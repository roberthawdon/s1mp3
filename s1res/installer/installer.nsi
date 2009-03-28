;==================================================================================================
;
; s1res installer script v1.0
;
; @see http://nsis.sourceforge.net/Main_Page
;      http://nsis.sourceforge.net/Allow_only_one_installer_instance
;      http://nsis.sourceforge.net/Add_uninstall_information_to_Add/Remove_Programs
;      http://nsis.sourceforge.net/Shortcuts_removal_fails_on_Windows_Vista
;
;==================================================================================================


;--------------------------------------------------------------------------------------------------
; COMPRESSOR SETTINGS
;--------------------------------------------------------------------------------------------------
SetCompress Auto
SetCompressor /SOLID lzma
SetCompressorDictSize 32
SetDatablockOptimize On


;--------------------------------------------------------------------------------------------------
; INCLUDES
;--------------------------------------------------------------------------------------------------
!include "MUI.nsh"
!include "InstallDriver.nsh"


;--------------------------------------------------------------------------------------------------
; GLOBAL VARIABLES
;--------------------------------------------------------------------------------------------------
Var STARTMENU_FOLDER
Var STARTMENU_UNINST


;--------------------------------------------------------------------------------------------------
; GENERAL SETTINGS
;--------------------------------------------------------------------------------------------------
Name "s1res"
Caption "s1res v4.1 - Setup"
;ShowInstDetails nevershow

XPStyle on
RequestExecutionLevel admin

;Icon "resources\setup.ico"
OutFile "setup.exe"

;Default installation folder
InstallDir "$PROGRAMFILES\s1res"
;Get installation folder from registry if available
InstallDirRegKey HKLM "Software\s1res" ""

;Define colors
;BGGradient 000000 000080 FFFFFF
;InstallColors FF8080 000030

BrandingText " "

;Install types
;InstType "Typical"
;InstType "Full"



;--------------------------------------------------------------------------------------------------
; MUI SETTINGS
;--------------------------------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
;!define MUI_LICENSEPAGE_BGCOLOR /grey

!define MUI_FINISHPAGE_RUN "$INSTDIR\s1res.exe"

!define MUI_THEME "orange-vista-cd"

	!define MUI_ICON "themes\${MUI_THEME}\installer-nopng.ico"
	!define MUI_UNICON "themes\${MUI_THEME}\uninstaller-nopng.ico"

	!define MUI_HEADERIMAGE
	!define MUI_HEADERIMAGE_RIGHT
	!define MUI_HEADERIMAGE_BITMAP "themes\${MUI_THEME}\header-r.bmp"
	!define MUI_HEADERIMAGE_UNBITMAP "themes\${MUI_THEME}\header-r-un.bmp"

	!define MUI_WELCOMEFINISHPAGE_BITMAP "themes\${MUI_THEME}\wizard.bmp"
	!define MUI_UNWELCOMEFINISHPAGE_BITMAP "themes\${MUI_THEME}\wizard-un.bmp"


;--------------------------------------------------------------------------------------------------
; INSTALLER PAGES
;--------------------------------------------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.rtf"
;!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH


;--------------------------------------------------------------------------------------------------
; UNINSTALLER PAGES
;--------------------------------------------------------------------------------------------------
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


;--------------------------------------------------------------------------------------------------
; LANGUAGES
;--------------------------------------------------------------------------------------------------
!insertmacro MUI_LANGUAGE "English"


;--------------------------------------------------------------------------------------------------
; VERSION INFO
;--------------------------------------------------------------------------------------------------
VIProductVersion "4.1.0.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "application installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "1.0.0.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "s1res setup"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "-"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "s1res(T)"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "http://www.s1mp3.de/"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright © 2005-2007 wiRe"


;--------------------------------------------------------------------------------------------------
; INIT FUNCTION
;--------------------------------------------------------------------------------------------------
Function .onInit

  ;
  ; allow only one instance of the setup
  ;
  BringToFront
  System::Call "kernel32::CreateMutexA(i 0, i 0, t '$(^Name)') i .r0 ?e"
  Pop $0
  StrCmp $0 0 launch
    StrLen $0 "$(^Name)"
    IntOp $0 $0 + 1
   loop:
    FindWindow $1 '#32770' '' 0 $1
    IntCmp $1 0 +4
    System::Call "user32::GetWindowText(i r1, t .r2, i r0) i."
    StrCmp $2 "$(^Name)" 0 loop

    System::Call "user32::ShowWindow(i r1,i 9) i."         ; If minimized then maximize
    System::Call "user32::SetForegroundWindow(i r1) i."    ; Bring to front
   abort:
    Abort

 launch:

  ;
  ; ensure any previous version was uninstalled before  
  IfFileExists "$INSTDIR\uninstall.exe" +1 notInstalledYet
    MessageBox MB_YESNOCANCEL|MB_ICONSTOP \
      "This product is already installed!$\n\
      If you wish to install a new version,$\n\
      you have to uninstall any previous version before.$\n$\n\
      Run uninstaller now?" IDNO abort IDCANCEL abort

    ;Exec "$INSTDIR\uninstall.exe"
    ;Abort

    ;call uninstaller and, once completed without errors, start the installation, otherwise exit...
    ExecWait '"$INSTDIR\uninstall.exe" _?=$INSTDIR'
    IfErrors abort

    ;since we disabled the "shadow-copy" above, "uninstall.exe" wasn't able to delete itself
    Delete "$INSTDIR\uninstall.exe"

 notInstalledYet:

  ;
  ; only allow "Windows NT"
  ClearErrors
  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  IfErrors 0 is_winnt
    MessageBox MB_OK|MB_ICONSTOP "You need Windows NT or later to install this product!"
    Abort
 is_winnt:

FunctionEnd


;--------------------------------------------------------------------------------------------------
; INSTALLER SECTIONS
;--------------------------------------------------------------------------------------------------
SectionGroup /e "s1res" SEC_LC

  Section "Core" SEC_LCC
    ;SectionIn 1 2 RO
    ;AddSize 0

    SetShellVarContext all
    SetOverwrite on
    SetOutPath "$INSTDIR"

    ;Update registry values
    WriteRegStr HKLM "Software\s1res" "" $INSTDIR

    ;Install files
    File "..\Release\s1res.exe"
    File "files\driver\libusb\libusb0.dll"
    File "..\help\s1res.chm"
    File "files\s1mp3.ico"

    ;Install driver
    SetOutPath "$INSTDIR\driver"
    File /r "files\driver\*"

    Push "$INSTDIR\driver\libusb"              ;the directory of the .inf file
    Push "$INSTDIR\driver\libusb\s1mp3.inf"    ;the filepath of the .inf file (directory + filename)
    Call InstallDriver

    ;Create startmenu shortcuts (optional)
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
      DetailPrint "Create shortcuts..."
      CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\s1res.lnk" "$INSTDIR\s1res.exe"
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\s1res help.lnk" "$INSTDIR\s1res.chm"
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\uninstall.lnk" "$INSTDIR\uninstall.exe"
      CreateShortcut "$SMPROGRAMS\$STARTMENU_FOLDER\s1mp3.de.lnk" "http://www.s1mp3.de/" "" "$INSTDIR\s1mp3.ico"
    !insertmacro MUI_STARTMENU_WRITE_END
  SectionEnd

SectionGroupEnd


Section -finalize

  ;write uninstaller and add application to the windows uninstall list
  WriteUninstaller "$INSTDIR\uninstall.exe"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\s1res" "DisplayName" "s1res"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\s1res" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\s1res" "DisplayIcon" "$INSTDIR\s1res.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\s1res" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\s1res" "NoRepair" 1

SectionEnd


;--------------------------------------------------------------------------------------------------
; UNINSTALLER SECTIONS
;--------------------------------------------------------------------------------------------------
Section "Uninstall"

  SetShellVarContext all

  ;Remove all files
  RMDir /r "$INSTDIR"

  ;Remove startmenu
  !insertmacro MUI_STARTMENU_GETFOLDER Application $STARTMENU_UNINST
  Delete "$SMPROGRAMS\$STARTMENU_UNINST\s1res.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_UNINST\s1res help.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_UNINST\uninstall.lnk"
  Delete "$SMPROGRAMS\$STARTMENU_UNINST\s1mp3.de.lnk"
  RMDir "$SMPROGRAMS\$STARTMENU_UNINST"

  ;Remove registry key and all subkeys
  ;DeleteRegKey HKLM "Software\s1res"
  ;DeleteRegKey HKCU "Software\s1res"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\s1res"

SectionEnd
