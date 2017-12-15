# Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

###############################################################################
# CMake build rules for micro RTPS transport
###############################################################################
cmake_minimum_required(VERSION 2.8.12)
if (${CMAKE_VERSION} VERSION_GREATER 3.0)
    cmake_policy(SET CMP0048 OLD)
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(transport C)

###############################################################################
# Version information                                                         #
###############################################################################
set(PROJECT_MAJOR_VERSION 0)
set(PROJECT_MINOR_VERSION 0)
set(PROJECT_MICRO_VERSION 1)
set(PROJECT_VERSION ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_MICRO_VERSION})
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

###############################################################################
# Config
###############################################################################
# Project specific options
set(MAX_MESSAGE_SIZE 512 CACHE STRING "Define maximum message size to be used")

configure_file(${PROJECT_SOURCE_DIR}/include/config.h.in
${PROJECT_BINARY_DIR}/include/config.h)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTRANSPORT_LOGS")

file(GLOB TRANSPORT_LIB_SOURCES
    src/c/*.c
)

add_library(transport SHARED ${TRANSPORT_LIB_SOURCES})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_BINARY_DIR}/include
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}>
)

install(TARGETS transport DESTINATION lib/)

###############################################################################
# Testing
###############################################################################
option(EPROSIMA_BUILD_TESTS "Activate the building and execution unit tests and integral tests" OFF)
if(EPROSIMA_BUILD_TESTS)
    add_subdirectory(test)
endif()