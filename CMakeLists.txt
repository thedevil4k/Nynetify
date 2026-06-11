cmake_minimum_required(VERSION 3.10)
project(Nynetify VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# ── Dependencies ──────────────────────────────
if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FLTK fltk)
    if(NOT FLTK_FOUND)
        pkg_check_modules(FLTK fltk1.3)
    endif()
    if(NOT FLTK_FOUND)
        message(FATAL_ERROR "FLTK not found via pkg-config. Install libfltk1.3-dev (Debian/Ubuntu) or fltk-devel (Fedora).")
    endif()
    include_directories(${FLTK_INCLUDE_DIRS})
else()
    find_package(FLTK REQUIRED)
    include_directories(${FLTK_INCLUDE_DIR})
endif()
include_directories(include)

if(WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/assets/Nynetify.ico" "Nynetify.ico" COPYONLY)
endif()

# ── Sources ────────────────────────────────────
add_executable(Nynetify
    src/main.cpp
    src/AppSettings.cpp
    src/UIWidgets.cpp
    src/ArtistParser.cpp
    src/PlayerController.cpp
    src/ViewManager.cpp
    src/AppCallbacks.cpp
)

if(WIN32)
    target_sources(Nynetify PRIVATE src/app.rc src/SystemTray.cpp)
    set_target_properties(Nynetify PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(Nynetify PRIVATE psapi iphlpapi)
endif()

if(UNIX)
    target_link_libraries(Nynetify PRIVATE ${FLTK_LINK_LIBRARIES} mpv)
    set(_fltk_compat "${CMAKE_BINARY_DIR}/fltk_compat.h")
    file(WRITE "${_fltk_compat}"
        "#pragma once\n"
        "#include <FL/Fl.H>\n"
        "#if FL_MINOR_VERSION < 4\n"
        "inline void fl_antialias(int) {}\n"
        "#endif\n")
    target_compile_options(Nynetify PRIVATE -include "${_fltk_compat}")
else()
    target_link_libraries(Nynetify PRIVATE ${FLTK_LIBRARIES} mpv)
endif()

# ── Install ──────────────────────────────────
if(WIN32)
    install(TARGETS Nynetify DESTINATION bin)
else()
    install(TARGETS Nynetify DESTINATION "/opt/nynetify/bin")
endif()

# ── CPack (Linux packages) ────────────────────
if(UNIX)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_PACKAGE_NAME "nynetify")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "YouTube Audio Player")
    set(CPACK_PACKAGE_CONTACT "https://github.com/anomalyco/Nynetify")
    set(CPACK_PACKAGE_VENDOR "Nynetify")

    # Depend on the system mpv package + core libs; yt-dlp is bundled
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "mpv")
    set(CPACK_RPM_PACKAGE_REQUIRES "mpv")

    if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|aarch64|arm64)$")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
        else()
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()
    set(CPACK_DEBIAN_FILE_NAME "nynetify-${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")

    if(NOT CPACK_RPM_PACKAGE_ARCHITECTURE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|aarch64|arm64)$")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE "i686")
        else()
            set(CPACK_RPM_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()
    set(CPACK_RPM_FILE_NAME "nynetify-${PROJECT_VERSION}-${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")

    set(CPACK_PACKAGE_INSTALL_DIR "/opt/nynetify")
    include(CPack)
endif()
