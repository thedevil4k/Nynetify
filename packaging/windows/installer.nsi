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
UnPage instfiles

; ── Files ──
Section "Nynetify" SEC_APP
    SectionIn RO
    SetOutPath "$INSTDIR"

    File "Nynetify.exe"

    ; MSYS2 runtime DLLs
    File "${MSYS2_BIN}\libmpv-2.dll"
    File "${MSYS2_BIN}\libfltk-1.4.dll"
    File "${MSYS2_BIN}\libfltk_images-1.4.dll"
    File "${MSYS2_BIN}\libstdc++-6.dll"
    File "${MSYS2_BIN}\libgcc_s_seh-1.dll"
    File "${MSYS2_BIN}\libwinpthread-1.dll"
    File "${MSYS2_BIN}\libpng16-16.dll"
    File "${MSYS2_BIN}\libjpeg-8.dll"
    File "${MSYS2_BIN}\libzlib1.dll"
    File "${MSYS2_BIN}\libbrotlicommon.dll"
    File "${MSYS2_BIN}\libbrotlidec.dll"
    File "${MSYS2_BIN}\libfreetype-6.dll"
    File "${MSYS2_BIN}\libharfbuzz-0.dll"
    File "${MSYS2_BIN}\libglib-2.0-0.dll"
    File "${MSYS2_BIN}\libintl-8.dll"
    File "${MSYS2_BIN}\libpcre2-8-0.dll"
    File "${MSYS2_BIN}\libiconv-2.dll"

    ; yt-dlp
    !if "${YT_DLP_PATH}" != ""
        File "${YT_DLP_PATH}"
    !endif

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
    Delete "$INSTDIR\libmpv-2.dll"
    Delete "$INSTDIR\libfltk-1.4.dll"
    Delete "$INSTDIR\libfltk_images-1.4.dll"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\libpng16-16.dll"
    Delete "$INSTDIR\libjpeg-8.dll"
    Delete "$INSTDIR\libzlib1.dll"
    Delete "$INSTDIR\libbrotlicommon.dll"
    Delete "$INSTDIR\libbrotlidec.dll"
    Delete "$INSTDIR\libfreetype-6.dll"
    Delete "$INSTDIR\libharfbuzz-0.dll"
    Delete "$INSTDIR\libglib-2.0-0.dll"
    Delete "$INSTDIR\libintl-8.dll"
    Delete "$INSTDIR\libpcre2-8-0.dll"
    Delete "$INSTDIR\libiconv-2.dll"
    Delete "$INSTDIR\yt-dlp.exe"

    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

    DeleteRegKey ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}"
SectionEnd
