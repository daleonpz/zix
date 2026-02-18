# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/crypto.c)
target_sources(app PRIVATE src/key.c)
# target_sources(app PRIVATE src/ecdsa.c)
target_include_directories(app PRIVATE utils)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
