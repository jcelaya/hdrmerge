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

!define MUI_ICON "@PROJ_SRC_DIR@\data\images\icon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "@PROJ_SRC_DIR@\data\images\logo.bmp"
!define MUI_HEADERIMAGE_RIGHT

;--------------------------------

; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "@PROJ_SRC_DIR@\LICENSE"
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
        "@PROJ_SRC_DIR@\LICENSE" \
        "@PROJ_SRC_DIR@\LICENSE_icons" \
        "@PROJ_SRC_DIR@\README.md"\
        "@MINGW_LIB_DIR@\libbz2-1.dll"\
        "@MINGW_LIB_DIR@\libexiv2.dll"\
        "@MINGW_LIB_DIR@\libexpat-1.dll"\
        "@MINGW_LIB_DIR@\libfreetype-6.dll"\
        "@MINGW_LIB_DIR@\libgcc_s_seh-1.dll"\
        "@MINGW_LIB_DIR@\libglib-2.0-0.dll"\
        "@MINGW_LIB_DIR@\libgomp-1.dll"\
        "@MINGW_LIB_DIR@\libgraphite2.dll"\
        "@MINGW_LIB_DIR@\libharfbuzz-0.dll"\
        "@MINGW_LIB_DIR@\libiconv-2.dll"\
        "@MINGW_LIB_DIR@\libicudt58.dll"\
        "@MINGW_LIB_DIR@\libicuin58.dll"\
        "@MINGW_LIB_DIR@\libicuuc58.dll"\
        "@MINGW_LIB_DIR@\libintl-8.dll"\
        "@MINGW_LIB_DIR@\libjasper-4.dll"\
        "@MINGW_LIB_DIR@\libjpeg-8.dll"\
        "@MINGW_LIB_DIR@\liblcms2-2.dll"\
        "@MINGW_LIB_DIR@\libpcre-1.dll"\
        "@MINGW_LIB_DIR@\libpcre2-16-0.dll"\
        "@MINGW_LIB_DIR@\libpng16-16.dll"\
        "@MINGW_LIB_DIR@\libraw_r-16.dll"\
        "@MINGW_LIB_DIR@\libstdc++-6.dll"\
        "@MINGW_LIB_DIR@\libwinpthread-1.dll"\
        "@MINGW_LIB_DIR@\Qt5Core.dll"\
        "@MINGW_LIB_DIR@\Qt5Gui.dll"\
        "@MINGW_LIB_DIR@\Qt5Widgets.dll"\
        "@MINGW_LIB_DIR@\zlib1.dll"
    File /oname=hdrmerge.com "hdrmerge-nogui.exe"
    
    SetOutPath $INSTDIR\platforms

    File "@QT5_PLUGINS_DIR@\platforms\qwindows.dll"

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
    Delete "$INSTDIR\platforms\qwindows.dll"
    RMDir  "$INSTDIR\platforms"
    Delete "$INSTDIR\libbz2-1.dll"
    Delete "$INSTDIR\libexiv2.dll"
    Delete "$INSTDIR\libexpat-1.dll"
    Delete "$INSTDIR\libfreetype-6.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libglib-2.0-0.dll"
    Delete "$INSTDIR\libgomp-1.dll"
    Delete "$INSTDIR\libgraphite2.dll"
    Delete "$INSTDIR\libharfbuzz-0.dll"
    Delete "$INSTDIR\libiconv-2.dll"
    Delete "$INSTDIR\libicudt58.dll"
    Delete "$INSTDIR\libicuin58.dll"
    Delete "$INSTDIR\libicuuc58.dll"
    Delete "$INSTDIR\libintl-8.dll"
    Delete "$INSTDIR\libjasper-4.dll"
    Delete "$INSTDIR\libjpeg-8.dll"
    Delete "$INSTDIR\liblcms2-2.dll"
    Delete "$INSTDIR\libpcre-1.dll"
    Delete "$INSTDIR\libpcre2-16-0.dll"
    Delete "$INSTDIR\libpng16-16.dll"
    Delete "$INSTDIR\libraw_r-16.dll"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\zlib1.dll"
    RMDir /REBOOTOK $INSTDIR

SectionEnd
