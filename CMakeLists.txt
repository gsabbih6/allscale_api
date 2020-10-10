# This CMakeLists.txt should only be run by ExternalProject

#cmake_minimum_required(VERSION 3.2)

#option(INVOKED_AS_EXTERNAL_PROJECT OFF)
#if(NOT INVOKED_AS_EXTERNAL_PROJECT)
#	message(FATAL_ERROR "Root CMakeLists.txt only used for External Projects, call CMake on `code` folder instead.")
#endif()

#enable_testing()
#add_subdirectory(code)


cmake_minimum_required(VERSION 3.5)
project(allscale_api LANGUAGES C CXX)

# -- Module Path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})

# -- Prefix Path
set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/third_party CACHE STRING "Third Party Library Directory")
file(GLOB prefix_paths ${THIRD_PARTY_DIR}/*)
list(APPEND CMAKE_PREFIX_PATH ${prefix_paths})

# -- Project Settings
include(build_settings)
include(coverage)
include(doxygen)

# -- Dependency Settings
include(dependencies/pthread)
include(dependencies/googletest)
#include(dependencies/boost)
include(dependencies/valgrind)

# -- CMake Modules
include(add_module)
include(file_globs)
include(msvc_source_group)
include(nproc)

# -- Project Modules
add_subdirectory(api)
add_subdirectory(utils)
add_subdirectory(tutorials)
