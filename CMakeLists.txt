cmake_minimum_required(VERSION 3.10)
project(Nynetify VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(FLTK REQUIRED)
include_directories(${FLTK_INCLUDE_DIR})
include_directories(include)

# Copy icon to build directory for RC compiler
if(WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/assets/Nynetify.ico" "Nynetify.ico" COPYONLY)
endif()

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
    target_sources(Nynetify PRIVATE src/app.rc)
    set_target_properties(Nynetify PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(Nynetify ${FLTK_LIBRARIES} mpv psapi iphlpapi)

# ── CPack (Linux packages) ──────────────────────────
if(UNIX)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_PACKAGE_NAME "nynetify")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "YouTube Audio Player")
    set(CPACK_PACKAGE_CONTACT "https://github.com/anomalyco/Nynetify")
    set(CPACK_PACKAGE_VENDOR "Nynetify")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "mpv, yt-dlp")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_FILE_NAME "nynetify-${PROJECT_VERSION}-amd64.deb")

    set(CPACK_RPM_PACKAGE_REQUIRES "mpv, yt-dlp")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_RPM_FILE_NAME "nynetify-${PROJECT_VERSION}-x86_64.rpm")

    include(CPack)
endif()
