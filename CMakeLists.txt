# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008"
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles"
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 2.4)

# enable testing
enable_testing()

# project name and language
project(unquote CXX)

# check build type
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# make uppercase string of build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

# check build type again
if (NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

## set output directory (build/)
#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
#set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

##############################################################################

option(SHIFT_JIS "Use Shift_JIS" OFF)
if (SHIFT_JIS)
    add_definitions(-DSHIFT_JIS)
endif()

# iconv library
find_package(Iconv)
include_directories(${Iconv_INCLUDE_DIRS})

# libunquote.a
add_library(unquote STATIC unquote.cpp)
if (Iconv_FOUND)
    target_compile_definitions(unquote PRIVATE -DHAVE_ICONV)
endif()
target_link_libraries(unquote PRIVATE ${Iconv_LIBRARIES})

# unquote-unittest.exe
add_executable(unquote-unittest unquote_unittest.cpp)
target_link_libraries(unquote-unittest PRIVATE unquote ${Iconv_LIBRARIES})

# tests
add_test(NAME unquote-unittest COMMAND $<TARGET_FILE:unquote-unittest>)

##############################################################################
