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

get_target_property(PROJECT_INCLUDE_FILES ${PROJECT_NAME} INCLUDE_DIRECTORIES)
include_directories(${PROJECT_INCLUDE_FILES})

file(GLOB_RECURSE files "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
foreach(file ${files})
    get_filename_component(file_name ${file} NAME_WE)
    file(GLOB TEST_SRCS "${file_name}.c")
    add_executable(${file_name} ${TEST_SRCS})
    #install(TARGETS ${file_name} DESTINATION ${CMAKE_BINARY_DIR}/install)
	target_link_libraries(${file_name} ${PROJECT_NAME})
endforeach()