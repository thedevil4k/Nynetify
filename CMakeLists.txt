cmake_minimum_required(VERSION 3.10)
project(YTfy)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(FLTK REQUIRED)
include_directories(${FLTK_INCLUDE_DIR})

# Replace with actual libmpv path if not in standard location
include_directories(include)

add_executable(YTfy 
    src/main.cpp
)

target_link_libraries(YTfy ${FLTK_LIBRARIES} mpv psapi iphlpapi)
