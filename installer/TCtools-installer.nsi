; ==========================================================================
; TCtools-installer.nsi  -  NSIS installer for TC-tools v0.1.0-rc2
; Build:  makensis TCtools-installer.nsi    (or run build-installer.bat)
; Requirements: NSIS 3.x (Unicode)  -  https://nsis.sourceforge.io/
; ==========================================================================

Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"

!define VERSION       "0.1.0-rc2"
!define APPNAME       "TC-tools"
!define EXENAME       "tctool.exe"
!define PUBLISHER     "TC-tools Project (Wu Qiaosheng)"
!define UNINSTKEY     "Software\Microsoft\Windows\CurrentVersion\Uninstall\TC-tools"
!define APPPATHKEY    "Software\Microsoft\Windows\CurrentVersion\App Paths\tctool.exe"
!define ENVKEY        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"

Name "${APPNAME} ${VERSION}"
OutFile "..\dist\TCtools-installer-${VERSION}.exe"
InstallDir "$PROGRAMFILES64\TC-tools"
InstallDirRegKey HKLM "${UNINSTKEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "${APPNAME} v${VERSION} | ${PUBLISHER}"

; ---------------------------------------------------------- pages ----------
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXENAME}"
!define MUI_FINISHPAGE_RUN_TEXT "$(STR_FINISH_RUN)"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------- strings --------
LangString STR_MAIN_DESC    ${LANG_ENGLISH}      "TC-tools main program (required). Adds the 'tctool' command to PATH for CMD."
LangString STR_MAIN_DESC    ${LANG_SIMPCHINESE}  "TC-tools 主程序（必选）。将 tctool 命令添加到 PATH，可在 CMD 中直接运行。"
LangString STR_DESKTOP_DESC ${LANG_ENGLISH}      "Create a shortcut on your desktop."
LangString STR_DESKTOP_DESC ${LANG_SIMPCHINESE}  "在桌面创建快捷方式。"
LangString STR_SM_DESC      ${LANG_ENGLISH}      "Add TC-tools to the Start Menu (application list)."
LangString STR_SM_DESC      ${LANG_SIMPCHINESE}  "添加到开始菜单（应用列表）。"
LangString STR_FINISH_RUN   ${LANG_ENGLISH}      "Run TC-tools now"
LangString STR_FINISH_RUN   ${LANG_SIMPCHINESE}  "立即运行 TC-tools"
LangString STR_MAIN_TITLE   ${LANG_ENGLISH}      "TC-tools (required)"
LangString STR_MAIN_TITLE   ${LANG_SIMPCHINESE}  "TC-tools 主程序（必选）"
LangString STR_DESKTOP_TITLE ${LANG_ENGLISH}     "Desktop shortcut"
LangString STR_DESKTOP_TITLE ${LANG_SIMPCHINESE} "桌面快捷方式"
LangString STR_SM_TITLE     ${LANG_ENGLISH}      "Start menu entry"
LangString STR_SM_TITLE     ${LANG_SIMPCHINESE}  "开始菜单项"

; ---------------------------------------------------------- sections -------
Section "$(STR_MAIN_TITLE)" SEC_MAIN
  SectionIn RO
  SetRegView 64
  SetOutPath "$INSTDIR"
  File "..\dist\${EXENAME}"
  File /oname=LICENSE.txt "..\LICENSE"
  File /oname=README.md "..\README.md"
  File /oname=README_EN.md "..\README_EN.md"
  !if /FileExists "..\packages\manifest.json"
    SetOutPath "$INSTDIR\packages"
    File /r "..\packages\*.*"
    SetOutPath "$INSTDIR"
  !endif
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; PATH (mandatory, machine-wide) + notify Explorer
  Push $INSTDIR
  Call AddToPathHklm
  System::Call 'user32::SendMessageTimeoutW(i0xFFFF, i0x001A, i0, w"Environment", i0x0002, i5000, *i.r0)'

  ; registry: uninstall entry + App Paths
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\${EXENAME}"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "${UNINSTKEY}" "QuietUninstallString" "$INSTDIR\uninstall.exe /S"
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://turtleweb.cc.cd"
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
  WriteRegStr HKLM "${APPPATHKEY}" "" "$INSTDIR\${EXENAME}"
  WriteRegStr HKLM "${APPPATHKEY}" "Path" "$INSTDIR"
SectionEnd

; desktop shortcut: optional, default OFF
Section /o "$(STR_DESKTOP_TITLE)" SEC_DESKTOP
  SetRegView 64
  CreateShortCut "$DESKTOP\TC-tools.lnk" "$INSTDIR\${EXENAME}"
SectionEnd

; start menu: optional, default ON
Section "$(STR_SM_TITLE)" SEC_SM
  SetRegView 64
  CreateDirectory "$SMPROGRAMS\TC-tools"
  CreateShortCut "$SMPROGRAMS\TC-tools\TC-tools.lnk" "$INSTDIR\${EXENAME}"
  CreateShortCut "$SMPROGRAMS\TC-tools\Uninstall TC-tools.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

; ---------------------------------------------------------- descriptions ---
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN}    "$(STR_MAIN_DESC)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "$(STR_DESKTOP_DESC)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_SM}      "$(STR_SM_DESC)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ==========================================================================
;  machine PATH helpers (case-insensitive membership check)
; ==========================================================================
; StrContains: $0 = needle, $1 = haystack -> returns $0 = "1" if found else "0" (case sensitive)
Function StrContains
  Exch $0
  Exch
  Exch $1
  Push $2
  Push $3
  Push $4
  Push $5
  StrLen $2 $0                  ; needle len
  StrLen $3 $1                  ; hay len
  StrCpy $4 0
  loop:
    IntOp $5 $3 - $2
    IntCmp $4 $5 0 0 notfound   ; pos > hay - needle -> not found
    StrCpy $5 $1 $2 $4          ; slice
    StrCmp $5 $0 found
    IntOp $4 $4 + 1
    Goto loop
  found:
    StrCpy $0 "1"
    Goto out
  notfound:
    StrCpy $0 "0"
  out:
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Exch $1
    Exch
    Exch $0
FunctionEnd

Function AddToPathHklm
  Exch $0
  Push $1
  Push $2
  Push $3
  SetRegView 64
  ReadRegStr $1 HKLM "${ENVKEY}" "Path"
  StrCpy $2 ";"$1";"
  StrCpy $3 ";"$0";"
 
  StrCpy $1 $2
  Call StrContains
  StrCmp $0 "1" skip
  ReadRegStr $1 HKLM "${ENVKEY}" "Path"
  ${If} $1 != ""
    StrCpy $2 $1 1
    ${If} $2 != ";"
      StrCpy $1 "$1;"
    ${EndIf}
    StrCpy $1 "$1$0"
  ${Else}
    StrCpy $1 "$0"
  ${EndIf}
  WriteRegExpandStr HKLM "${ENVKEY}" "Path" "$1"
  skip:
  Pop $3
  Pop $2
  Pop $1
  Exch $0
FunctionEnd

; uninstaller: remove $INSTDIR token from machine PATH
Function un.RemoveFromPathHklm
  Exch $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5
  SetRegView 64
  ReadRegStr $1 HKLM "${ENVKEY}" "Path"
  StrCpy $2 ""
  StrCpy $4 $1
  loop:
    StrCmp $4 "" done
    StrCpy $5 $4 1
    StrCmp $5 ";" skipEmpty
    StrCpy $3 ""
    tokloop:
      StrCpy $5 $4 1
      StrCmp $5 "" tokdone
      StrCmp $5 ";" tokdone
      StrCpy $3 "$3$5"
      StrCpy $4 $4 "" 1
      Goto tokloop
    tokdone:
    StrCmp $3 $0 drop
    StrCmp $2 "" first
    StrCpy $2 "$2;$3"
    Goto after
    first:
      StrCpy $2 "$3"
      Goto after
    drop:
    after:
    StrCpy $5 $4 1
    StrCmp $5 ";" skipSemi
    Goto loop
    skipEmpty:
      StrCpy $4 $4 "" 1
      Goto loop
    skipSemi:
      StrCpy $4 $4 "" 1
      Goto loop
  done:
  WriteRegExpandStr HKLM "${ENVKEY}" "Path" "$2"
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Exch $0
FunctionEnd
; ---------------------------------------------------------- uninstaller ----
Section "Uninstall"
  SetRegView 64
  Push $INSTDIR
  Call un.RemoveFromPathHklm
  System::Call 'user32::SendMessageTimeoutW(i0xFFFF, i0x001A, i0, w"Environment", i0x0002, i5000, *i.r0)'

  Delete "$DESKTOP\TC-tools.lnk"
  Delete "$SMPROGRAMS\TC-tools\TC-tools.lnk"
  Delete "$SMPROGRAMS\TC-tools\Uninstall TC-tools.lnk"
  RMDir "$SMPROGRAMS\TC-tools"

  RMDir /r "$INSTDIR"
  RMDir "$INSTDIR"

  DeleteRegKey HKLM "${UNINSTKEY}"
  DeleteRegKey HKLM "${APPPATHKEY}"
SectionEnd