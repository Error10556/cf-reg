cmake_minimum_required(VERSION 3.30)
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

add_executable(cfreg main.cpp sheets.cpp u8.cpp passwords.cpp xlmodel.cpp)
target_link_libraries(cfreg PRIVATE OpenXLSX::OpenXLSX)
target_compile_options(cfreg PRIVATE -Wall -Wextra)
target_compile_features(cfreg PRIVATE cxx_std_26)

include(CTest)
if(BUILD_TESTING)
    add_executable(xlmodel_test tests/xlmodel_test.cpp xlmodel.cpp passwords.cpp u8.cpp)
    target_include_directories(xlmodel_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(xlmodel_test PRIVATE OpenXLSX::OpenXLSX)
    target_compile_features(xlmodel_test PRIVATE cxx_std_26)
    add_test(NAME xlmodel COMMAND xlmodel_test)
endif()
