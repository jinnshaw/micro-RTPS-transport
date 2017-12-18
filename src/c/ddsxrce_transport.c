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

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "ddsxrce_transport_common.h"
#include "ddsxrce_serial_transport.h"
#include "ddsxrce_udp_transport.h"
#include "ddsxrce_transport.h"

static locator_id_t g_loc_counter = 0;
static locator_id_plus_t g_loc_ids[MAX_NUM_LOCATORS];

locator_kind_t get_kind(const locator_id_t locator_id);
int extract_message(octet* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);


locator_kind_t get_kind(const locator_id_t locator_id)
{
    for (uint8_t i = 0; i < g_loc_counter; ++i)
    {
        if (g_loc_ids[i].id == locator_id)
        {
            return g_loc_ids[i].kind;
        }
    }
    return LOC_NONE;
}

locator_id_t add_serial_locator(const char* device)
{
    if (NULL ==  device)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    locator_id_t id = create_serial(device, ++g_loc_counter);
    if (0 > id)
    {
        serial_channel_t* channel = get_serial_channel(id);
        if (NULL == channel || 0 > open_serial(channel))
        {
            --g_loc_counter;
            return TRANSPORT_ERROR;
        }
    }

    g_loc_ids[g_loc_counter - 1].id = id;
    g_loc_ids[g_loc_counter - 1].kind = LOC_SERIAL;

    return id;
}

locator_id_t add_udp_locator_for_agent(const uint16_t local_udp_port)
{
    locator_id_t id = create_udp(local_udp_port, 0, NULL, ++g_loc_counter);
    if (0 > id)
    {
        udp_channel_t* channel = get_udp_channel(id);
        if (NULL == channel || 0 > open_udp(channel))
        {
            --g_loc_counter;
            return TRANSPORT_ERROR;
        }
    }

    g_loc_ids[g_loc_counter - 1].id = id;
    g_loc_ids[g_loc_counter - 1].kind = LOC_UDP;

    return id;
}

locator_id_t add_udp_locator_for_client(const uint16_t local_udp_port, const uint16_t remote_udp_port, const char* remote_ip)
{
    locator_id_t id = create_udp(local_udp_port, remote_udp_port, remote_ip, ++g_loc_counter);
    if (0 > id)
    {
        udp_channel_t* channel = get_udp_channel(id);
        if (NULL == channel || 0 > open_udp(channel))
        {
            --g_loc_counter;
            return TRANSPORT_ERROR;
        }
    }

    g_loc_ids[g_loc_counter - 1].id = id;
    g_loc_ids[g_loc_counter - 1].kind = LOC_UDP;

    return id;
}

int rm_locator(const locator_id_t locator_id)
{
    switch (get_kind(locator_id))
    {
        case LOC_SERIAL: return destroy_serial(locator_id);
        case LOC_UDP:    return destroy_udp(locator_id);
        default:         return TRANSPORT_ERROR;
    }

    // Remove reference
    for (uint8_t i = 0; i < g_loc_counter; ++i)
    {
        if (g_loc_ids[i].id == locator_id)
        {
            g_loc_ids[i].id = 0;
            g_loc_ids[i].kind = LOC_NONE;
        }
    }

    return  TRANSPORT_OK;
}

int send_data(const octet* in_buffer, const size_t buffer_len, const locator_id_t locator_id)
{
    if (NULL == in_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    static header_t header =
    {
        .marker = {'>', '>', '>'},
        .payload_len_h = 0u,
        .payload_len_l = 0u,
        .crc_h = 0u,
        .crc_l = 0u

    };

    // [>,>,>,topic_ID,seq,payload_length,CRCHigh,CRCLow,payload_start, ... ,payload_end]

    uint16_t crc = crc16(in_buffer, buffer_len);
    header.payload_len_h = (buffer_len >> 8) & 0xff;
    header.payload_len_l = buffer_len & 0xff;
    header.crc_h = (crc >> 8) & 0xff;
    header.crc_l = crc & 0xff;

    switch (get_kind(locator_id))
    {
        case LOC_SERIAL: return send_serial(&header, in_buffer, buffer_len, locator_id);
        case LOC_UDP:    return send_udp(&header, in_buffer, buffer_len, locator_id);
        default:         return TRANSPORT_ERROR;
    }
}

int receive_data(octet* out_buffer, const size_t buffer_len, const locator_id_t locator_id)
{
    if (NULL == out_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    switch (get_kind(locator_id))
    {
        case LOC_SERIAL: return receive_serial(out_buffer, buffer_len, locator_id);
        case LOC_UDP:    return receive_udp(out_buffer, buffer_len, locator_id);
        default:         return TRANSPORT_ERROR;
    }
}

int extract_message(octet* out_buffer, const size_t buffer_len, buffer_t* internal_buffer)
{
    if (NULL == out_buffer || NULL == internal_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    octet* rx_buffer = internal_buffer->buffer;
    uint16_t* rx_buff_pos = &(internal_buffer->buff_pos);

    // We read some
    size_t header_size = sizeof(header_t);

    // but not enough
    if ((*rx_buff_pos) < header_size)
    {
        return 0;
    }

    uint32_t msg_start_pos = 0;

    for (msg_start_pos = 0; msg_start_pos <= (*rx_buff_pos) - header_size; ++msg_start_pos)
    {
        if ('>' == rx_buffer[msg_start_pos] && memcmp(rx_buffer + msg_start_pos, ">>>", 3) == 0)
        {
            break;
        }
    }

    // Start not found
    if (msg_start_pos > (*rx_buff_pos) - header_size)
    {
        printf("                                 (↓↓ %u)\n", msg_start_pos);
        // All we've checked so far is garbage, drop it - but save unchecked bytes
        memmove(rx_buffer, rx_buffer + msg_start_pos, (*rx_buff_pos) - msg_start_pos);
        (*rx_buff_pos) = (*rx_buff_pos) - msg_start_pos;
        return TRANSPORT_ERROR;
    }

    /*
     * [>,>,>,length_H,length_L,CRC_H,CRC_L,payloadStart, ... ,payloadEnd]
     */

    header_t* header = (header_t*) &rx_buffer[msg_start_pos];
    uint32_t payload_len = ((uint32_t) header->payload_len_h << 8) | header->payload_len_l;

    // The message won't fit the buffer.
    if (buffer_len < header_size + payload_len)
    {
        return -EMSGSIZE;
    }

    // We do not have a complete message yet
    if (msg_start_pos + header_size + payload_len > (*rx_buff_pos))
    {
        // If there's garbage at the beginning, drop it
        if (msg_start_pos > 0)
        {
            printf("                                 (↓ %u)\n", msg_start_pos);
            memmove(rx_buffer, rx_buffer + msg_start_pos, (*rx_buff_pos) - msg_start_pos);
            (*rx_buff_pos) -= msg_start_pos;
        }

        return 0;
    }

    uint16_t read_crc = ((uint16_t) header->crc_h << 8) | header->crc_l;
    uint16_t calc_crc = crc16((uint8_t *) rx_buffer + msg_start_pos + header_size, payload_len);
    int ret = 0;

    if (read_crc != calc_crc)
    {
        printf("BAD CRC %u != %u\n", read_crc, calc_crc);
        printf("                                 (↓ %lu)\n", (unsigned long) (header_size + payload_len));
        ret = TRANSPORT_ERROR;

    }
    else
    {
        // copy message to outbuffer and set other return values
        memmove(out_buffer, rx_buffer + msg_start_pos + header_size, payload_len);
        ret = payload_len; // only payload, "+ header_size" for real size.
    }

    // discard message from rx_buffer
    (*rx_buff_pos) -= header_size + payload_len;
    memmove(rx_buffer, rx_buffer + msg_start_pos + header_size + payload_len, (*rx_buff_pos));

    return ret;
}
