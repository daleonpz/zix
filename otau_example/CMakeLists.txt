# Copyright 2025 Daniel Paredes (daleonpz)
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20.0)
# set(BOARD "nucleo_wb55rg")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(otau-example)

# this come from sysbuild
message(STATUS "Board: ${BOARD}")

zephyr_get(VERSION_MAJOR SYSBUILD GLOBAL)
zephyr_get(VERSION_MINOR SYSBUILD GLOBAL)
zephyr_get(VERSION_PATCH SYSBUILD GLOBAL)

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

message(STATUS "Version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
# Set the version string
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" CACHE STRING "Version string" FORCE)
set(VERSION_NUMBER "${VERSION_MAJOR}${VERSION_MINOR}${VERSION_PATCH}" CACHE STRING "Version number" FORCE)

# Pass the version string to the preprocessor
add_compile_definitions(-DVERSION_STRING="${VERSION_STRING}")
add_compile_definitions(-DVERSION_NUMBER="${VERSION_NUMBER}")

target_sources(app PRIVATE src/main.c)
