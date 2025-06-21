cmake_minimum_required(VERSION 3.20)

project(rezn-cp LANGUAGES C CXX)

# ----------------------------------------------------------------------
# Toolchain / language settings
# ----------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------------------------------------------------
# Project layout helpers
# ----------------------------------------------------------------------
set(DEPS_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------
# Sources
# ----------------------------------------------------------------------
set(PROJECT_SOURCES
    ${SRC_DIR}/client.cpp
    ${SRC_DIR}/api_client.cpp
    ${SRC_DIR}/tui_backend.cpp
    ${SRC_DIR}/main.cpp

    # --- ImGui ---------------------------------------------------------
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/imgui.cpp
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/imgui_draw.cpp
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/imgui_widgets.cpp
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/imgui_tables.cpp
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/misc/cpp/imgui_stdlib.cpp
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/imgui_demo.cpp

    # --- ImTui ---------------------------------------------------------
    ${DEPS_DIR}/imtui/src/imtui-impl-ncurses.cpp
    ${DEPS_DIR}/imtui/src/imtui-impl-text.cpp
)

# ----------------------------------------------------------------------
# Third-party libraries
# ----------------------------------------------------------------------
find_package(Curses REQUIRED)
find_package(CURL   REQUIRED)
find_package(Threads REQUIRED)

set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_DATA OFF CACHE BOOL "" FORCE)
set(LIBPASSGEN_STATIC ON CACHE BOOL "" FORCE)
set(LIBPASSGEN_SHARED OFF CACHE BOOL "" FORCE)
set(PASSGEN_STATIC ON CACHE BOOL "" FORCE)

list(APPEND CMAKE_MODULE_PATH "${DEPS_DIR}/passgen/cmake")
add_subdirectory(${DEPS_DIR}/passgen)

configure_file(
    ${DEPS_DIR}/passgen/include/passgen/config.h.in
    ${DEPS_DIR}/passgen/include/passgen/config.h
)

set(REPROC++ ON)
set(REPROC_MULTITHREADED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
add_subdirectory(${DEPS_DIR}/reproc)

# ----------------------------------------------------------------------
# Target
# ----------------------------------------------------------------------
add_executable(rezn-cp ${PROJECT_SOURCES})

target_include_directories(rezn-cp PRIVATE
    ${INCLUDE_DIR}
    ${CURSES_INCLUDE_DIR}
    ${DEPS_DIR}
    ${DEPS_DIR}/json/single_include
    ${DEPS_DIR}/imtui/include
    ${DEPS_DIR}/imtui/third-party/imgui
    ${DEPS_DIR}/imtui/third-party/imgui/imgui
    ${DEPS_DIR}/imtui/third-party/imgui/imgui/misc/cpp
)

target_link_libraries(rezn-cp
    PRIVATE
    ${CURSES_LIBRARIES}
    Threads::Threads
    CURL::libcurl
    passgen-static
)

