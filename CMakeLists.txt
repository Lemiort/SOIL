cmake_minimum_required(VERSION 3.13.3)
set(PROJECT_NAME "soilcpp")

project(${PROJECT_NAME} CXX)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
else()
  include(conanbuildinfo.cmake)
endif()
conan_basic_setup()
endif()


include_directories(${CONAN_INCLUDE_DIRS})
# include_directories(src)

file(GLOB_RECURSE SOURCES_LIST "src/*.cpp")

add_library(${PROJECT_NAME} ${SOURCES_LIST})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})