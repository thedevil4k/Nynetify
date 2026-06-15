cmake_minimum_required(VERSION 3.10)
project(Nynetify VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# ── Dependencies ──────────────────────────────
find_package(FLTK REQUIRED)
include_directories(${FLTK_INCLUDE_DIR})
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

target_link_libraries(Nynetify PRIVATE ${FLTK_LIBRARIES} mpv)

if(UNIX)
    set(_fltk_compat "${CMAKE_BINARY_DIR}/fltk_compat.h")
    file(WRITE "${_fltk_compat}"
        "#pragma once\n"
        "#include <FL/Fl.H>\n"
        "#if FL_MINOR_VERSION < 4\n"
        "inline void fl_antialias(int) {}\n"
        "#endif\n")
    target_compile_options(Nynetify PRIVATE -include "${_fltk_compat}")
endif()

# ── Install ──────────────────────────────────
install(TARGETS Nynetify DESTINATION bin)
