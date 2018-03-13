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

#ifndef _MICRORTPS_TRANSPORT_COMMON_H_
#define _MICRORTPS_TRANSPORT_COMMON_H_

#include "micrortps_transport_dll.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#ifndef _WIN32
#ifndef __PX4_NUTTX
#include <arpa/inet.h>
#endif // __PX4_NUTTX
#endif // _WIN32

#define TRANSPORT_ERROR              -1
#define TRANSPORT_OK                  0

#define RX_BUFFER_LENGTH           CMAKE_MAX_TRANSMISSION_UNIT_SIZE
#define UART_NAME_MAX_LENGTH          64
#define IP_LENGTH                      4

typedef int16_t locator_id_t;
typedef unsigned char octet_t;


/**********************************************************************************************************************/
/****************************** CAUTION: CODE COPIED FROM xrce_protocol_spec.h ****************************************/
/**********************************************************************************************************************/


typedef struct String_t
{
    uint32_t size;
    char* data;

} String_t;


typedef enum TransportLocatorFormat
{
    ADDRESS_FORMAT_SMALL = 0,
    ADDRESS_FORMAT_MEDIUM = 1,
    ADDRESS_FORMAT_LARGE = 2,
    ADDRESS_FORMAT_STRING = 3

} TransportLocatorFormat;


typedef struct TransportLocatorSmall
{
    uint8_t address[2];
    uint16_t locator_port;

} TransportLocatorSmall;


typedef struct TransportLocatorMedium
{
    uint8_t address[4];
    uint16_t locator_port;

} TransportLocatorMedium;


typedef struct TransportLocatorLarge
{
    uint8_t address[16];
    uint16_t locator_port;

} TransportLocatorLarge;


typedef struct TransportLocatorString
{
    char value[CMAKE_MAX_STRING_SIZE];

} TransportLocatorString;


typedef union TransportLocatorU
{
    TransportLocatorSmall small_locator;
    TransportLocatorMedium medium_locator;
    TransportLocatorLarge large_locator;
    TransportLocatorString string_locator;

} TransportLocatorU;


typedef struct TransportLocator
{
    uint8_t format;
    TransportLocatorU _;

} TransportLocator;


typedef struct TransportLocatorSeq
{
    uint32_t size;
    TransportLocator* data;

} TransportLocatorSeq;


/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/


typedef enum Kind
{
    LOC_NONE,
    LOC_SERIAL,
    LOC_UDP_AGENT,
    LOC_UDP_CLIENT

} locator_kind_t;

typedef struct
{
    octet_t buffer[RX_BUFFER_LENGTH];
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

#ifdef _WIN32
    SOCKET socket_fd;
#else
    int socket_fd;
#endif

    uint16_t local_udp_port;
    uint16_t remote_udp_port;


#ifndef __PX4_NUTTX
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
#endif

    uint8_t remote_ip[IP_LENGTH];
    uint32_t poll_ms;

    uint8_t locator_id;
    uint8_t idx;
    bool open;

} udp_channel_t;


typedef union TransportChannelU
{
    serial_channel_t serial;
    udp_channel_t udp;

} TransportChannelU;


typedef struct TransportChannel
{
    uint8_t kind;
    TransportChannelU _;

} TransportChannel;


typedef struct MicroRTPSLocator
{
    TransportLocator public;
    TransportChannel private;

} MicroRTPSLocator;

DLLEXPORT void ms_sleep(int milliseconds);

#ifdef __cplusplus
}
#endif

#endif
