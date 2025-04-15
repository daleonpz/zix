# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)
set(BOARD "nucleo_wb55rg")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(otau-example)

if(NOT DEFINED VERSION_MAJOR)
    set(VERSION_MAJOR 0)
else()
    set(VERSION_MAJOR ${VERSION_MAJOR})
endif()
if(NOT DEFINED VERSION_MINOR)
    set(VERSION_MINOR 1)
else()
    set(VERSION_MINOR ${VERSION_MINOR})
endif()
if(NOT DEFINED VERSION_PATCH)
    set(VERSION_PATCH 0)
else()
    set(VERSION_PATCH ${VERSION_PATCH})
endif()

# Set the version string
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" CACHE STRING "Version string" FORCE)
set(VERSION_NUMBER "${VERSION_MAJOR}${VERSION_MINOR}${VERSION_PATCH}" CACHE STRING "Version number" FORCE)

# Pass the version string to the preprocessor
add_compile_definitions(-DVERSION_STRING="${VERSION_STRING}")
add_compile_definitions(-DVERSION_NUMBER="${VERSION_NUMBER}")

target_sources(app PRIVATE src/main.c)
