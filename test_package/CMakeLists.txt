cmake_minimum_required(VERSION 3.1.2)
set(PROJECT_NAME "soilcppTest")
project("${PROJECT_NAME}" CXX)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


find_package("GLEW")

if(GLEW_FOUND)
    message("GLEW was found")
else()
    message(FATAL_ERROR "GLEW not found")
endif()

find_package(glfw3)

if(glfw3_FOUND)
    message("GLFW was found")
else()
    message(FATAL_ERROR "GLFW not found")
endif()

find_package(SOILCPP)
if(SOILCPP_FOUND)
    message("SOILCPP was found")
    # include_directories(${SOIL_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "SOILCPP not found")
endif()

add_executable(${PROJECT_NAME} test.cpp)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# CTest is a testing tool that can be used to test your project.
enable_testing()
add_test(NAME ${PROJECT_NAME}
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
         COMMAND ${PROJECT_NAME})
