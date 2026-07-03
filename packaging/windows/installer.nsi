; Nynetify Windows Installer (NSIS)
; Usage: makensis /DARCH=x64 /DMSYS2_BIN=C:\msys64\mingw64\bin /DYT_DLP_PATH=C:\path\to\yt-dlp.exe installer.nsi

Unicode true

!define PRODUCT_NAME "Nynetify"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Nynetify"
!define PRODUCT_WEB_SITE "https://github.com/anomalyco/Nynetify"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Nynetify.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

; ── Architecture detection ──
!ifndef ARCH
    !define ARCH "x64"
!endif

!if "${ARCH}" == "arm64"
    !define ARCH_DIR "CLANGARM64"
    !define INSTDIR_REG_ROOT HKLM
    !define INSTDIR_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}_arm64"
!else
    !define ARCH_DIR "MINGW64"
    !define INSTDIR_REG_ROOT HKLM
    !define INSTDIR_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}_x64"
!endif

Name "${PRODUCT_NAME} ${PRODUCT_VERSION} (${ARCH})"
OutFile "Nynetify-${PRODUCT_VERSION}-${ARCH}-setup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" ""

RequestExecutionLevel admin

; ── Pages ──
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

; ── Files ──
Section "Nynetify" SEC_APP
    SectionIn RO
    SetOutPath "$INSTDIR"

    File "Nynetify.exe"

    ; MSYS2 runtime DLLs (dynamically resolved and copied by workflow)
    File "*.dll"

    ; yt-dlp
    File /nonfatal "yt-dlp.exe"

    ; App icon
    File /nonfatal "Nynetify.ico"

    ; Assets folder (logos, radio station data)
    CreateDirectory "$INSTDIR\assets"
    SetOutPath "$INSTDIR\assets"
    File /r "assets\*.*"
    SetOutPath "$INSTDIR"

    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\Nynetify.exe" "" "$INSTDIR\Nynetify.exe" 0
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\Nynetify.exe" "" "$INSTDIR\Nynetify.exe" 0

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Registry for Add/Remove Programs
    WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "DisplayIcon" "$INSTDIR\Nynetify.exe"
    WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegDWORD ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "NoModify" 1
    WriteRegDWORD ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "NoRepair" 1
SectionEnd

Section "Start Menu Shortcuts" SEC_SHORTCUTS
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\Nynetify.exe" "" "$INSTDIR\Nynetify.exe" 0
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

; ── Uninstaller ──
Section Uninstall
    Delete "$INSTDIR\Nynetify.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\yt-dlp.exe"
    Delete "$INSTDIR\Nynetify.ico"
    Delete "$INSTDIR\stations.json"

    ; Remove assets folder
    Delete "$INSTDIR\assets\*.*"
    RMDir "$INSTDIR\assets"

    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

    DeleteRegKey ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}"
SectionEnd
