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

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(transport C)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTRANSPORT_LOGS")

file(GLOB ${PROJECT_NAME}_SOURCES
    src/c/*.c
)
set(PROJECT_VERSION 1)
set(PROJECT_MAJOR_VERSION 2)
set(PROJECT_MINOR_VERSION 3)
# Create libraries
if(MSVC OR MSVC_IDE)
    add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SOURCES})

    add_library(${PROJECT_NAME}_static STATIC ${${PROJECT_NAME}_SOURCES})
    # Rename the library to have a "lib" before.
    set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME lib${PROJECT_NAME})

    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_static PROPERTIES RELEASE_POSTFIX -${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION})
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_static PROPERTIES RELWITHDEBINFO_POSTFIX -${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION})
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_static PROPERTIES DEBUG_POSTFIX d-${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION})

    # Export symbols in DLL libraries
    target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME_UPPER}_DYN_LINK)

    # Define public headers
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}>
        $<INSTALL_INTERFACE:include>
        )

    # Define public headers for static library
    target_include_directories(${PROJECT_NAME}_static PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}>
        $<INSTALL_INTERFACE:include>
        )
else()
    #Create library
    add_library(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_MAJOR_VERSION})

    # Define public headers
    target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}>
        $<INSTALL_INTERFACE:include>
        )
endif()


# Link library to external win32 libraries.
target_link_libraries(
    ${PROJECT_NAME} PUBLIC
    Ws2_32 kernel32
)

install(TARGETS transport DESTINATION lib/)

###############################################################################
# Testing
###############################################################################
option(EPROSIMA_BUILD_TESTS "Activate the building and execution unit tests and integral tests" OFF)
if(EPROSIMA_BUILD_TESTS)
    add_subdirectory(test)
endif()