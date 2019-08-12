cmake_minimum_required(VERSION 3.13.3)
set(PROJECT_NAME "soilcpp")

project(${PROJECT_NAME} CXX)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  if (CONAN_PROFILE)
    set(CONANPROFILE PROFILE ${CONAN_PROFILE})
  endif()
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.14/conan.cmake"
                   "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)
  conan_cmake_run(
    CONANFILE conanfile.py
    BASIC_SETUP
    ${CONANPROFILE}
  )
endif()

find_package("GLEW")
if(GLEW_FOUND)
    message("GLEW was found")
else()
    message(FATAL_ERROR "GLEW not found")
endif()

include_directories(${GLEW_INCLUDE_DIR})
include_directories(src)
file(GLOB_RECURSE SOURCES_LIST "src/*.cpp")

add_library(${PROJECT_NAME} ${SOURCES_LIST})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})