// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _DDSXRCE_TRANSPORT_COMMON_H_
#define _DDSXRCE_TRANSPORT_COMMON_H_

#include <poll.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef __PX4_NUTTX
    #include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif



#define TRANSPORT_ERROR              -1
#define TRANSPORT_OK                  0

typedef unsigned char octet;

typedef struct __attribute__((packed)) Header
{
    char marker[3];
    octet payload_len_h;
    octet payload_len_l;
    octet crc_h;
    octet crc_l;
} header_t;

typedef enum Kind
{
    LOC_NONE,
    LOC_SERIAL,
    LOC_UDP,

} locator_kind_t;

typedef int16_t locator_id_t;

typedef struct __attribute__((packed))
{
    locator_id_t id;
    locator_kind_t kind;
    octet data[16];
} locator_t;

typedef struct __attribute__((packed))
{
    locator_id_t id;
    locator_kind_t kind;
} locator_id_plus_t;

/// SERIAL TRANSPORT

#define DFLT_UART             "/dev/ttyACM0"
#define DFLT_BAUDRATE            115200
#define DFLT_POLL_MS                 20
#define RX_BUFFER_LENGTH           1024
#define UART_NAME_MAX_LENGTH         64
#define MAX_NUM_CHANNELS              8
#define MAX_NUM_LOCATORS              8

typedef struct
{
    octet buffer[RX_BUFFER_LENGTH];
    uint16_t buff_pos;
} buffer_t;

typedef struct
{
    buffer_t rx_buffer;

    char uart_name[UART_NAME_MAX_LENGTH];
    int uart_fd;
    uint32_t baudrate;
    uint32_t poll_ms;

    uint8_t locator_id;
    uint8_t idx;
    bool open;

} serial_channel_t;

typedef struct
{
    buffer_t rx_buffer;

    int sender_fd;
    int receiver_fd;
    uint16_t udp_port_recv;
    uint16_t udp_port_send;
#ifndef __PX4_NUTTX
    struct sockaddr_in sender_outaddr;
    struct sockaddr_in receiver_inaddr;
    struct sockaddr_in receiver_outaddr;
#endif

    uint32_t poll_ms;

    uint8_t locator_id;
    uint8_t idx;
    bool open;

} udp_channel_t;

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(uint8_t const *buffer, size_t len);
void print_buffer(const uint8_t* buffer, const size_t len);

#ifdef __cplusplus
}
#endif

#endif
