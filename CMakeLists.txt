# Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

set(IS_TOP_LEVEL TRUE)
if(NOT "${PROJECT_SOURCE_DIR}" STREQUAL "")
    set(IS_TOP_LEVEL FALSE)
endif()

# Set CMAKE_BUILD_TYPE to Release by default.
if(DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        )
else()
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        )
endif()

###############################################################################
# Product information                                                         #
###############################################################################

project(transport)

# Set product name
if(DEFINED PRODUCT_NAME)
    set(PRODUCT_NAME ${PRODUCT_NAME} CACHE STRING "Choose the product name in which this project is included")
else()
    set(PRODUCT_NAME micrortps CACHE STRING "Choose the product name in which this project is included")
endif()

set(PROJECT_LIBRARY_NAME ${PROJECT_NAME})

###############################################################################
# Version information                                                         #
###############################################################################
set(PROJECT_MAJOR_VERSION 1)
set(PROJECT_MINOR_VERSION 0)
set(PROJECT_MICRO_VERSION 0)
set(PROJECT_VERSION ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_MICRO_VERSION})
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")


###############################################################################
# Default shared libraries
###############################################################################
# Global flag to cause add_library() to create shared libraries if on.
# If set to true, this will cause all libraries to be built shared
# unless the library was explicitly added as a static library.
option(BUILD_SHARED_LIBS "Create shared libraries by default" OFF)



###############################################################################

# Config
###############################################################################
include(${PROJECT_SOURCE_DIR}/cmake/defines/compiling_time_defines.cmake)


#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTRANSPORT_LOGS")
###############################################################################
# Warning level
###############################################################################
if(MSVC OR MSVC_IDE)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
endif()

# Set source files
file(GLOB ${PROJECT_NAME}_SOURCES
    src/c/*.c
)

# Create libraries
if(MSVC OR MSVC_IDE)
    add_library(${PROJECT_NAME}        SHARED ${${PROJECT_NAME}_SOURCES})
    add_library(${PROJECT_NAME}_static STATIC ${${PROJECT_NAME}_SOURCES})
    # Rename the library to have a "lib" before.
    set_target_properties(                ${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME            lib${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_static PROPERTIES RELEASE_POSTFIX        -${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_static PROPERTIES RELWITHDEBINFO_POSTFIX -${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_static PROPERTIES DEBUG_POSTFIX          d-${PROJECT_VERSION})

    # Export symbols in DLL libraries
    target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME_UPPER}_DYN_LINK)

    # Define public_ headers
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

    # Define public_ headers for static library
    target_include_directories(${PROJECT_NAME}_static PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

    # Link library to external win32 libraries.
    target_link_libraries(
        ${PROJECT_NAME} PUBLIC
        Ws2_32 Kernel32
        )

else()
    #Create library
    add_library(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_MAJOR_VERSION})

    # Define public_ headers
    target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
endif()



###############################################################################
# Testing
###############################################################################
option(EPROSIMA_BUILD_TESTS "Activate the building and execution unit tests and integral tests" OFF)

if(EPROSIMA_BUILD)
    set(EPROSIMA_BUILD_TESTS ON)
endif()

if(EPROSIMA_BUILD_TESTS AND IS_TOP_LEVEL)
    add_subdirectory(test)
endif()

###############################################################################
# Installation paths
###############################################################################
set(BIN_INSTALL_DIR bin/ CACHE PATH "Installation directory for binaries")
set(INCLUDE_INSTALL_DIR include/ CACHE PATH "Installation directory for C headers")
set(LIB_INSTALL_DIR lib/ CACHE PATH "Installation directory for libraries")
set(DATA_INSTALL_DIR share/ CACHE PATH "Installation directory for data")
if(WIN32)
    set(LICENSE_INSTALL_DIR . CACHE PATH "Installation directory for licenses")
else()
    set(LICENSE_INSTALL_DIR ${DATA_INSTALL_DIR}/${PROJECT_NAME} CACHE PATH "Installation directory for licenses")
endif()

###############################################################################
# Packaging
###############################################################################
# Install public_ headers
include(${PROJECT_SOURCE_DIR}/cmake/dev/set_sources.cmake)
set_public_headers_directory(${PROJECT_SOURCE_DIR}/include ${PRODUCT_NAME}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    COMPONENT headers
    )

# Install config.h header
set_public_header(${PROJECT_BINARY_DIR}/include ${PRODUCT_NAME}/${PROJECT_NAME} config.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/${PRODUCT_NAME}/${PROJECT_NAME}
    COMPONENT headers
    INSTALL
    )

install(TARGETS ${PROJECT_LIBRARY_NAME}
        EXPORT ${PROJECT_LIBRARY_NAME}Targets
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
        COMPONENT libraries
)

export(TARGETS ${PROJECT_LIBRARY_NAME} FILE ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_LIBRARY_NAME}Targets.cmake)

install(EXPORT ${PROJECT_LIBRARY_NAME}Targets
DESTINATION ${LIB_INSTALL_DIR}/${PROJECT_LIBRARY_NAME}/cmake
COMPONENT cmake
)

include(${PROJECT_SOURCE_DIR}/cmake/packaging/eProsimaPackaging.cmake)
