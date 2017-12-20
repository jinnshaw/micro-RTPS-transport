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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#else

#include <unistd.h>
#include <termios.h>
#include <poll.h>

#ifndef __PX4_NUTTX
#include <arpa/inet.h>

#endif // __PX4_NUTTX
#endif // _WIN32

#ifdef __cplusplus
extern "C"
{
#endif


#if defined(_WIN32)
#define __PACKED__(struct_to_pack) __pragma(pack(push, 1)) struct_to_pack __pragma(pack(pop))
#else
#define __PACKED__(struct_to_pack) struct_to_pack __attribute__((__packed__))
#endif


#define TRANSPORT_ERROR              -1
#define TRANSPORT_OK                  0

typedef unsigned char octet;

__PACKED__( struct Header
{
    char marker[3];
    octet payload_len_h;
    octet payload_len_l;
    octet crc_h;
    octet crc_l;
});

typedef struct Header header_t;

typedef enum Kind
{
    LOC_NONE,
    LOC_SERIAL,
    LOC_UDP,

} locator_kind_t;

typedef int16_t locator_id_t;

__PACKED__( struct Locator
{
    locator_id_t id;
    locator_kind_t kind;
    octet data[16];
});

typedef struct Locator locator_t;

__PACKED__( struct Locator_id_plus
{
    locator_id_t id;
    locator_kind_t kind;
});

typedef struct Locator_id_plus locator_id_plus_t;

/// SERIAL TRANSPORT

#define DFLT_UART             "/dev/ttyACM0"
#define DFLT_BAUDRATE            115200
#define DFLT_POLL_MS                 20
#define RX_BUFFER_LENGTH           1024
#define UART_NAME_MAX_LENGTH         64
#define IP_MAX_LENGTH                16
#define MAX_NUM_CHANNELS              8
#define MAX_NUM_LOCATORS              8
#define MAX_PENDING_CONNECTIONS      10

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

/// UDP TRANSPORT

#define DFLT_UDP_PORT               2019


typedef struct
{
    buffer_t rx_buffer;

#ifdef _WIN32
    SOCKET recv_socket_fd;
    SOCKET send_socket_fd;
#else
    int recv_socket_fd;
    int send_socket_fd;
#endif
    uint16_t local_recv_udp_port;
    uint16_t local_send_udp_port;
    uint16_t remote_udp_port;

#ifndef __PX4_NUTTX
    struct sockaddr_in local_recv_addr;
    struct sockaddr_in local_send_addr;
    struct sockaddr_in remote_recv_addr;
    struct sockaddr_in remote_send_addr;
#endif

    char remote_ip[IP_MAX_LENGTH];
    uint32_t poll_ms;

    uint8_t locator_id;
    uint8_t idx;
    bool open;

} udp_channel_t;


uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(uint8_t const *buffer, size_t len);
void print_buffer(const uint8_t* buffer, const size_t len);
void eSleep(int milliseconds);

#ifdef __cplusplus
}
#endif

#endif
