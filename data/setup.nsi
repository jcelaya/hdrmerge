!include MUI2.nsh
!include FileFunc.nsh

!define APPNAME "HDRMerge@WIN_ARCH@"

; The name of the installer
Name "${APPNAME} v@HDRMERGE_VERSION@"

; The file to write
OutFile "@SETUP_PROG@"

; The default installation directory
InstallDir $PROGRAMFILES@WIN_ARCH@\HDRMerge

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\${APPNAME}" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Version Information

VIProductVersion "@HDRMERGE_VERSION@.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "CompanyName" "Javier Celaya"
VIAddVersionKey "LegalCopyright" "Copyright Javier Celaya"
VIAddVersionKey "FileDescription" "${APPNAME}"
VIAddVersionKey "FileVersion" "@HDRMERGE_VERSION@"
VIAddVersionKey "ProductVersion" "@HDRMERGE_VERSION@"

!define MUI_ICON "@PROJECT_SOURCE_DIR@/images/icon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "@PROJECT_SOURCE_DIR@/images/logo.bmp"
!define MUI_HEADERIMAGE_RIGHT

;--------------------------------

; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "@PROJECT_SOURCE_DIR@/LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

; The stuff to install
Section "HDRMerge (required)"

    SectionIn RO

    ; Set output path to the installation directory.
    SetOutPath $INSTDIR

    ; Put file there
    File "hdrmerge.exe" \
        "@PROJECT_SOURCE_DIR@/LICENSE" \
        "@PROJECT_SOURCE_DIR@/LICENSE_icons" \
        "@PROJECT_SOURCE_DIR@/README.md"
    File /oname=hdrmerge.com "hdrmerge-nogui.exe"

    ; Write the installation path into the registry
    WriteRegStr HKLM SOFTWARE\${APPNAME} "Install_Dir" "$INSTDIR"

    ; Write the uninstall keys for Windows
    !define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    WriteRegStr HKLM "${ARP}"   "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "${ARP}"   "DisplayIcon" "$INSTDIR\hdrmerge.exe"
    WriteRegStr HKLM "${ARP}"   "DisplayVersion" "@HDRMERGE_VERSION@"
    WriteRegStr HKLM "${ARP}"   "InstallLocation" "$INSTDIR"
    ; Compute installed size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${ARP}" "EstimatedSize" "$0"
    WriteRegStr HKLM "${ARP}"   "Publisher" "Javier Celaya"
    WriteRegStr HKLM "${ARP}"   "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${ARP}" "NoModify" 1
    WriteRegDWORD HKLM "${ARP}" "NoRepair" 1
    WriteUninstaller "uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
    CreateShortCut "$SMPROGRAMS\${APPNAME}\HDRMerge.lnk" "$INSTDIR\hdrmerge.exe" "" "$INSTDIR\hdrmerge.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

    ; Remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    DeleteRegKey HKLM SOFTWARE\${APPNAME}

    ; Remove shortcuts, if any
    Delete "$SMPROGRAMS\${APPNAME}\*.*"

    ; Remove directories used
    RMDir "$SMPROGRAMS\${APPNAME}"
    Delete "$INSTDIR\hdrmerge.exe"
    Delete "$INSTDIR\hdrmerge.com"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\LICENSE_icons"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\uninstall.exe"
    RMDir /REBOOTOK $INSTDIR

SectionEnd
