cmake_minimum_required(VERSION 3.20)

project(rezn-ledger LANGUAGES C CXX)

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
    ${SRC_DIR}/ledger.cpp

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

# ----------------------------------------------------------------------
# Target
# ----------------------------------------------------------------------
add_executable(rezn-ledger ${PROJECT_SOURCES})

target_include_directories(rezn-ledger PRIVATE
    ${INCLUDE_DIR}
    ${CURSES_INCLUDE_DIR}
    ${DEPS_DIR}
    ${DEPS_DIR}/json/single_include
    ${DEPS_DIR}/imtui/include
    ${DEPS_DIR}/imtui/third-party/imgui
    ${DEPS_DIR}/imtui/third-party/imgui/imgui
)

target_link_libraries(rezn-ledger
    PRIVATE
    ${CURSES_LIBRARIES}
    Threads::Threads
    CURL::libcurl
)
