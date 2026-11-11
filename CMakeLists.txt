cmake_minimum_required(VERSION 3.30)
project(CF-Reg VERSION 1.0.0 LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)

# Add https://github.com/xlnt-community/xlnt at tag v1.6.1

add_executable(cfreg main.cpp)
target_include_directories(cfreg ${CMAKE_PROJECT_BINARY_DIR}/#[[CFReg repo top-level dir]]/include)
target_link_libraries(cfreg STATIC #[[xlnt]])
