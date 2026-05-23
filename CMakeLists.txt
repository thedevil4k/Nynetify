cmake_minimum_required(VERSION 3.10)
project(Nynetify)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(FLTK REQUIRED)
include_directories(${FLTK_INCLUDE_DIR})

# Replace with actual libmpv path if not in standard location
include_directories(include)

add_executable(Nynetify 
    src/main.cpp
    src/AppSettings.cpp
    src/UIWidgets.cpp
    src/ArtistParser.cpp
    src/PlayerController.cpp
    src/ViewManager.cpp
    src/AppCallbacks.cpp
)

target_link_libraries(Nynetify ${FLTK_LIBRARIES} mpv psapi iphlpapi)
