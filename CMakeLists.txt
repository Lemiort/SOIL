cmake_minimum_required(VERSION 3.13.3)
set(PROJECT_NAME "SOIL")

project(${PROJECT_NAME} C)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
    message("OpenGL was found")
else()
    message(FATAL_ERROR "OpenGL not found")
endif()

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(src)
file(GLOB_RECURSE SOURCES_LIST "src/*.c")

add_library(${PROJECT_NAME} ${SOURCES_LIST})

# Reserved if someone wants to build it statically
# add_library(${PROJECT_NAME} SHARED ${SOURCES_LIST})

# target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})