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
    target_sources(Nynetify PRIVATE src/app.rc src/SystemTray.cpp)
    set_target_properties(Nynetify PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(Nynetify PRIVATE psapi iphlpapi)
endif()

target_link_libraries(Nynetify PRIVATE ${FLTK_LIBRARIES} mpv)

# ── CPack (Linux packages) ──────────────────────────
if(UNIX)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_PACKAGE_NAME "nynetify")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "YouTube Audio Player")
    set(CPACK_PACKAGE_CONTACT "https://github.com/anomalyco/Nynetify")
    set(CPACK_PACKAGE_VENDOR "Nynetify")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "mpv, yt-dlp")
    set(CPACK_RPM_PACKAGE_REQUIRES "mpv, yt-dlp")

    # Detect architecture dynamically based on compiler flags
    if(CMAKE_C_FLAGS MATCHES "-m32" OR CMAKE_CXX_FLAGS MATCHES "-m32")
        set(ARCH "i386")
    else()
        set(ARCH "amd64")
    endif()

    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${ARCH}")
    set(CPACK_DEBIAN_FILE_NAME "nynetify-${PROJECT_VERSION}-${ARCH}.deb")

    set(CPACK_RPM_PACKAGE_ARCHITECTURE "${ARCH}")
    set(CPACK_RPM_FILE_NAME "nynetify-${PROJECT_VERSION}-${ARCH}.rpm")

    include(CPack)
endif()
