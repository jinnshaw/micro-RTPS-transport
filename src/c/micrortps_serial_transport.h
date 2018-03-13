// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _MICRORTPS_SERIAL_TRANSPORT_H_
#define _MICRORTPS_SERIAL_TRANSPORT_H_

#include "micrortps_transport_common_priv.h"

#ifdef __cplusplus
extern "C"
{
#endif

locator_id_t create_serial (const char* device, locator_id_t locator_id, serial_channel_t* channel);
int          remove_serial (const locator_id_t locator_id);
int          open_serial   (serial_channel_t* channel);
int          close_serial  (serial_channel_t* channel);
int          send_serial   (const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id);
int          receive_serial(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms);

serial_channel_t* get_serial_channel(const locator_id_t locator_id);

#ifdef __cplusplus
}
#endif

#endif
