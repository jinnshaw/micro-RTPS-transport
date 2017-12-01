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

# send_test
file(GLOB S_TEST_SOURCES "send_test.c")
add_executable(send_test ${S_TEST_SOURCES})
#install(TARGETS send_test DESTINATION ${CMAKE_BINARY_DIR}/install)
target_link_libraries(send_test transport)

# recv_test
file(GLOB R_TEST_SOURCES "recv_test.c")
add_executable(recv_test ${R_TEST_SOURCES})
#install(TARGETS recv_test DESTINATION ${CMAKE_BINARY_DIR}/install)
target_link_libraries(recv_test transport)
