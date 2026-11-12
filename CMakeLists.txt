cmake_minimum_required(VERSION 3.30)

set(CACHE{CMAKE_CXX_FLAGS_LLDEBUG} TYPE STRING VALUE "-O0 -glldb -fstandalone-debug")
mark_as_advanced(CMAKE_CXX_FLAGS_LLDEBUG)

project(CF-Reg VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_SHARED_LIBS OFF)

include(FetchContent)

set(OPENXLSX_CREATE_DOCS OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
# Fetch pinned dependencies consistently, avoiding incompatible system packages.
set(FORCE_FETCH_ALL ON CACHE BOOL "Fetch OpenXLSX dependencies" FORCE)

FetchContent_Declare(
    OpenXLSX
    GIT_REPOSITORY https://codeberg.org/lars_uffmann/OpenXLSX.git
    GIT_TAG v0.5.2
    GIT_SHALLOW TRUE
)
# OpenXLSX v0.5.2 fetches miniz 3.0.2, whose policy baseline predates CMake 4.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM OR CMAKE_POLICY_VERSION_MINIMUM VERSION_LESS 3.5)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()
FetchContent_MakeAvailable(OpenXLSX)

add_library(cfreg_core STATIC sheets.cpp u8.cpp passwords.cpp xlmodel.cpp)
target_include_directories(cfreg_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cfreg_core PUBLIC OpenXLSX::OpenXLSX)
target_compile_options(cfreg_core PRIVATE -Wall -Wextra)
target_compile_features(cfreg_core PUBLIC cxx_std_26)

add_executable(cfreg main.cpp)
target_link_libraries(cfreg PRIVATE cfreg_core)

option(CFREG_BUILD_GUI "Build the native Windows application" ${WIN32})
if(CFREG_BUILD_GUI)
    if(NOT WIN32)
        message(FATAL_ERROR "The native GUI requires a Windows target. Use CFREG_BUILD_GUI=OFF for the CLI.")
    endif()
    add_library(cfreg_widgets STATIC mainwindow.cpp)
    target_compile_definitions(cfreg_widgets PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(cfreg_widgets PUBLIC cfreg_core user32 gdi32)
    target_compile_options(cfreg_widgets PRIVATE -Wall -Wextra)
    add_executable(cfreg-gui WIN32 gui_main.cpp)
    target_link_libraries(cfreg-gui PRIVATE cfreg_widgets shell32)
    if(MINGW)
        target_link_options(cfreg-gui PRIVATE -municode -static)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(xlmodel_test tests/xlmodel_test.cpp)
    target_link_libraries(xlmodel_test PRIVATE cfreg_core)
    add_test(NAME xlmodel COMMAND xlmodel_test)
    if(CFREG_BUILD_GUI)
        add_executable(gui_test tests/gui_test.cpp)
        target_link_libraries(gui_test PRIVATE cfreg_widgets)
        add_test(NAME gui COMMAND gui_test)
    endif()
endif()
