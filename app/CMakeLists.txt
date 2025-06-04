# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD "nucleo_wb55rg")

set(CODECHECKER_ANALYZE_OPTS
    "--config;${CMAKE_SOURCE_DIR}/config/codechecker.yml;"
    "--skip;${CMAKE_SOURCE_DIR}/config/skip.codechecker"
    CACHE STRING "Code checker options")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
