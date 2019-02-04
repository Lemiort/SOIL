cmake_minimum_required(VERSION 3.13.3)
set(PROJECT_NAME "SOIL")

project(${PROJECT_NAME} C)
set(CMAKE_BUILD_TYPE Release)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(build/conanbuildinfo.cmake)
conan_basic_setup()


find_package("GLEW")
if(GLEW_FOUND)
    message("GLEW was found")
else()
    message(FATAL_ERROR "GLEW not found")
endif()

include_directories(${GLEW_INCLUDE_DIR})
include_directories(src)
file(GLOB_RECURSE SOURCES_LIST "src/*.c")

add_library(${PROJECT_NAME} ${SOURCES_LIST})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})