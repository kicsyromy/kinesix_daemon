cmake_minimum_required (VERSION 3.14)

project (kinesix-daemon C)

set (CMAKE_C_STANDARD 11)
set (CMAKE_C_STANDARD_REQUIRED ON)

find_package (PkgConfig QUIET REQUIRED)
pkg_search_module (libinput libinput REQUIRED IMPORTED_TARGET)
pkg_search_module (libudev libudev REQUIRED IMPORTED_TARGET)
pkg_search_module (libdbus dbus-1 REQUIRED IMPORTED_TARGET)

file (GLOB_RECURSE ${PROJECT_NAME}_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/*.h")
file (GLOB_RECURSE ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable (
    ${PROJECT_NAME} 
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
)

target_compile_definitions (
    ${PROJECT_NAME} PUBLIC
    -DPROJECT_NAME="${PROJECT_NAME}"
)

target_include_directories (
    ${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries (
    ${PROJECT_NAME} PUBLIC
    PkgConfig::libinput
    PkgConfig::libudev
    PkgConfig::libdbus
    pthread
)
