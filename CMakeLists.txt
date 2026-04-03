cmake_minimum_required(VERSION 3.21)
project(hexes VERSION 0.1.0 LANGUAGES CXX)

# yaml-cpp's CMakeLists uses cmake_minimum_required < 3.5 which newer CMake
# rejects without this. Set it before any CPMAddPackage calls.
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)

# Export compile_commands.json for clangd (IDE/LSP support).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── CPM source cache (avoids re-downloading across clean builds) ───────────────
# Set the CPM_SOURCE_CACHE env var, or uncomment the line below to hard-code it:
# set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm-cache")
if(NOT DEFINED CPM_SOURCE_CACHE AND DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "$ENV{CPM_SOURCE_CACHE}")
endif()

# ── Bootstrap CPM.cmake ────────────────────────────────────────────────────────
set(CPM_DOWNLOAD_VERSION 0.42.1)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}...")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
    )
endif()
include("${CPM_DOWNLOAD_LOCATION}")

# ── FTXUI ─────────────────────────────────────────────────────────────────────
# Exports: ftxui::screen, ftxui::dom, ftxui::component
CPMAddPackage(
    NAME ftxui
    GITHUB_REPOSITORY ArthurSonzogni/FTXUI
    VERSION 6.1.9
    OPTIONS
        "FTXUI_BUILD_EXAMPLES OFF"
        "FTXUI_BUILD_DOCS OFF"
)

# ── sol2 + Lua 5.4 ────────────────────────────────────────────────────────────
# Exports: sol2::sol2
# Lua 5.5 is installed alongside 5.4 on this machine; pin 5.4 explicitly so
# find_package(Lua) inside sol2 doesn't pick up the wrong version.
# Pin Lua 5.4 before sol2's find_package(Lua) runs.
# LUA_LIBRARY (singular) is the key that FindLua's find_library checks;
# LUA_LIBRARIES (plural) is derived after the search.
set(LUA_INCLUDE_DIR "/usr/include/lua5.4"   CACHE PATH     "Lua 5.4 include dir" FORCE)
set(LUA_LIBRARY     "/usr/lib/liblua5.4.so" CACHE FILEPATH "Lua 5.4 library"     FORCE)
find_package(Lua 5.4 EXACT REQUIRED)

# sol2's CMakeLists creates a lualib imported target but never links it to
# sol2::sol2, so the Lua include dir and library don't propagate automatically.
# Create our own canonical Lua 5.4 imported target and link it through
# hexes_lib PUBLIC so every consumer gets the right headers and library.
add_library(lua54 SHARED IMPORTED GLOBAL)
set_target_properties(lua54 PROPERTIES
    IMPORTED_LOCATION     "/usr/lib/liblua5.4.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include/lua5.4"
)

CPMAddPackage(
    NAME sol2
    GITHUB_REPOSITORY ThePhD/sol2
    VERSION 3.5.0
    OPTIONS
        "SOL2_TESTS OFF"
        "SOL2_EXAMPLES OFF"
        "SOL2_INTEROP_EXAMPLES OFF"
        "SOL2_DYNAMIC_LOADING_EXAMPLES OFF"
        "SOL2_BUILD_LUA OFF"
)

# ── glaze ─────────────────────────────────────────────────────────────────────
# Exports: glaze::glaze (header-only)
CPMAddPackage(
    NAME glaze
    GITHUB_REPOSITORY stephenberry/glaze
    VERSION 7.2.3
    OPTIONS
        "glaze_DEVELOPER_MODE OFF"
)

# ── yaml-cpp ───────────────────────────────────────────────────────────────────
# Used for YAML serialization. glaze v2 has no YAML format; yaml-cpp fills the gap.
# Exports: yaml-cpp::yaml-cpp
CPMAddPackage(
    NAME yaml-cpp
    GITHUB_REPOSITORY jbeder/yaml-cpp
    GIT_TAG 0.8.0
    OPTIONS
        "YAML_CPP_BUILD_TESTS OFF"
        "YAML_CPP_BUILD_TOOLS OFF"
        "YAML_CPP_BUILD_CONTRIB OFF"
        "YAML_CPP_INSTALL OFF"
)

# ── spdlog ─────────────────────────────────────────────────────────────────────
# Fast C++ logging library.
# Exports: spdlog::spdlog
CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.14.1
    OPTIONS
        "SPDLOG_BUILD_SHARED OFF"
        "SPDLOG_BUILD_EXAMPLE OFF"
        "SPDLOG_BUILD_TESTS OFF"
)

# ── Core library ───────────────────────────────────────────────────────────────
file(GLOB_RECURSE HEXES_SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
)
message(STATUS "HEXES_SOURCES: ${HEXES_SOURCES}")
add_library(hexes_lib ${HEXES_SOURCES})
add_library(hexes::hexes ALIAS hexes_lib)

target_include_directories(hexes_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(hexes_lib PUBLIC
    sol2::sol2
    lua54
    glaze::glaze
    yaml-cpp::yaml-cpp
    spdlog::spdlog
)

# ── TUI application ────────────────────────────────────────────────────────────
add_executable(hexes_app app/main.cpp)

target_link_libraries(hexes_app PRIVATE
    hexes::hexes
    ftxui::screen
    ftxui::dom
    ftxui::component
)

add_custom_command(TARGET hexes_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/scripts"
        "$<TARGET_FILE_DIR:hexes_app>/scripts"
    COMMENT "Copying Lua scripts to build directory"
)

# ── Examples ───────────────────────────────────────────────────────────────────
add_executable(example_basic examples/basic/main.cpp)
target_link_libraries(example_basic PRIVATE hexes::hexes)

add_custom_command(TARGET example_basic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/scripts"
        "$<TARGET_FILE_DIR:example_basic>/scripts"
    COMMENT "Copying Lua scripts alongside example"
)
