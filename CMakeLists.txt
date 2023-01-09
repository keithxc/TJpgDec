cmake_minimum_required (VERSION 3.2)

project ("jpeg_demo")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# include
include_directories(${CMAKE_CURRENT_LIST_DIR})

# src
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src SRC_CPP)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/test SRC_CPP)

add_executable (${PROJECT_NAME} ${SRC_CPP})
