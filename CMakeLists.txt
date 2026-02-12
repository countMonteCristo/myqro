cmake_minimum_required(VERSION 3.28.3)

project(myqro LANGUAGES CXX)
set(myqro_VERSION 1.0)

# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, e.g. Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
endif()

message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

include_directories(${CMAKE_HOME_DIRECTORY}/myqro/include/)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true ${CMAKE_HOME_DIRECTORY}/myqro/src/*.cpp)
set(SOURCES ${SOURCES})

add_library(myqro_static STATIC ${SOURCES})
add_library(myqro_shared SHARED ${SOURCES})

set_target_properties(myqro_static PROPERTIES OUTPUT_NAME myqro)
set_target_properties(myqro_shared PROPERTIES OUTPUT_NAME myqro)

set_target_properties(myqro_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib/)
set_target_properties(myqro_shared PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib/)

include(CTest)

if(NOT CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_subdirectory(exe)

include(GNUInstallDirs)
install(TARGETS myqro_static)
install(TARGETS myqro_shared)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION .)
