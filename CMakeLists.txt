cmake_minimum_required(VERSION 3.30)
project(CF-Reg VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_SHARED_LIBS OFF)

include(FetchContent)

# xlnt uses its own option for static builds rather than BUILD_SHARED_LIBS.
set(STATIC ON CACHE BOOL "Build xlnt as a static library" FORCE)

FetchContent_Declare(
    xlnt
    GIT_REPOSITORY https://github.com/xlnt-community/xlnt.git
    GIT_TAG 551e772b58668b494207c7ff63979476b403421f
    GIT_SHALLOW TRUE
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/xlnt"
)
FetchContent_MakeAvailable(xlnt)

add_executable(cfreg main.cpp sheets.cpp u8.cpp passwords.cpp xlmodel.cpp)
target_link_libraries(cfreg PRIVATE xlnt)
target_compile_options(cfreg PRIVATE -Wall -Wextra)
target_compile_features(cfreg PRIVATE cxx_std_26)
