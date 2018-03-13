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

# Project specific options

set(_CONFIG_MAX_TRANSMISSION_UNIT_SIZE_ 512 CACHE STRING "Define maximum transmission unit size to be used")

set(_CONFIG_MAX_NUM_LOCATORS_ 8 CACHE STRING "Define maximum transport locators to be used")

set(_CONFIG_MAX_STRING_SIZE_ 255 CACHE STRING "Define maximum size for store the device name")

# Create source file with these defines
configure_file(${PROJECT_SOURCE_DIR}/include/${PRODUCT_NAME}/${PROJECT_NAME}/config.h.in
               ${PROJECT_BINARY_DIR}/include/${PRODUCT_NAME}/${PROJECT_NAME}/config.h
               )