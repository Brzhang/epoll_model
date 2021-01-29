# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.0)

project(client)

# Import required CMake files
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.


set (EXECUTABLE_OUTPUT_PATH ./release)
#set (CMAKE_BUILD_TYPE RELEASE)

add_library(libglog STATIC IMPORTED)
set_target_properties(libglog PROPERTIES IMPORTED_LOCATION
  ${CMAKE_SOURCE_DIR}/../utility/lib/glog/lib/libglog.a)

include_directories(${PROJECT_SOURCE_DIR}/../utility/lib/glog/include)
include_directories(${PROJECT_SOURCE_DIR}/../protocol)
include_directories(${PROJECT_SOURCE_DIR}/../utility)

aux_source_directory(${CMAKE_SOURCE_DIR}/   SOURCE)
aux_source_directory(${CMAKE_SOURCE_DIR}/../protocol  PROTOCOL)
aux_source_directory(${CMAKE_SOURCE_DIR}/../utility  UTIL)

add_executable(${PROJECT_NAME} ${SOURCE} ${PROTOCOL} ${UTIL})
target_link_libraries( ${PROJECT_NAME} libglog -ldl -lz -lpthread)