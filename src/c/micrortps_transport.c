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

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "micrortps_serial_transport.h"
#include "micrortps_udp_transport.h"
#include <micrortps/transport/micrortps_transport.h>

static locator_id_t g_loc_counter = 0;
static locator_id_plus_t g_loc_ids[MAX_NUM_LOCATORS];

locator_kind_t get_kind(const locator_id_t locator_id);
int extract_message(octet_t* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);


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


uint8_t init_locator_udp_agent(const uint16_t local_port, MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# init_locator_udp_agent(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    memset(locator, 0, sizeof(MicroRTPSLocator));
    locator->public_.format = ADDRESS_FORMAT_MEDIUM;
    locator->public_._.medium_locator.locator_port = local_port;
    locator->private_.kind = LOC_UDP_AGENT;

    return TRANSPORT_OK;
}


uint8_t init_locator_udp_client(const uint16_t remote_port, const uint8_t* address, MicroRTPSLocator* const locator)
{
    if (NULL == locator || NULL == address)
    {
        printf("# init_locator_udp_client(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    memset(locator, 0, sizeof(MicroRTPSLocator));
    locator->public_.format = ADDRESS_FORMAT_MEDIUM;
    locator->public_._.medium_locator.locator_port = remote_port;
    memcpy(locator->public_._.medium_locator.address, address, sizeof(locator->public_._.medium_locator.address));

    locator->private_.kind = LOC_UDP_CLIENT;

    return TRANSPORT_OK;
}


uint8_t init_locator_serial(const char* device, MicroRTPSLocator* const locator)
{
    if (NULL == device || NULL == locator)
    {
        printf("# init_locator_serial(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (strlen(device) + 1 > sizeof(locator->public_._.string_locator.value))
    {
        printf("# init_locator_serial(): error, device name too large\n");
        return TRANSPORT_ERROR;
    }

    memset(locator, 0, sizeof(MicroRTPSLocator));
    locator->public_.format = ADDRESS_FORMAT_STRING;
    strcpy(locator->public_._.string_locator.value, device);

    locator->private_.kind = LOC_SERIAL;

    return TRANSPORT_OK;
}


locator_id_t add_udp_locator(const uint16_t local_udp_port,
                             const uint16_t remote_udp_port, const uint8_t* remote_ip,
                             MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# add_udp_locator(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    locator_id_t id = create_udp(local_udp_port,
                                 remote_udp_port,
                                 remote_ip,
                                 ++g_loc_counter,
                                 &(locator->private_._.udp));
    if (0 >= id)
    {
        --g_loc_counter;
        return TRANSPORT_ERROR;
    }

    g_loc_ids[g_loc_counter - 1].id = id;
    g_loc_ids[g_loc_counter - 1].kind = locator->private_.kind;

    return id;
}

locator_id_t add_init_locator_udp_agent(MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# add_udp_locator_for_agent(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    return add_udp_locator(locator->public_._.medium_locator.locator_port,
                           0,
                           NULL,
                           locator);
}

locator_id_t add_locator_udp_agent(const uint16_t local_port, MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# add_udp_locator_for_agent(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (TRANSPORT_ERROR == init_locator_udp_agent(local_port, locator))
    {
        printf("# add_locator_udp_agent(): BAD INIT!\n");
        return TRANSPORT_ERROR;
    }

    return add_init_locator_udp_agent(locator);
}

locator_id_t add_init_locator_udp_client(MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# add_locator_udp_client(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    return add_udp_locator(0,
                           locator->public_._.medium_locator.locator_port,
                           locator->public_._.medium_locator.address,
                           locator);
}

locator_id_t add_locator_udp_client(const uint16_t remote_port, const uint8_t* remote_ip,
                                    MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# add_locator_udp_client(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (TRANSPORT_ERROR == init_locator_udp_client(remote_port, remote_ip, locator))
    {
        printf("# add_locator_udp_client(): BAD INIT!\n");
        return TRANSPORT_ERROR;
    }

    return add_init_locator_udp_client(locator);
}


locator_id_t add_init_locator_serial(MicroRTPSLocator* locator)
{
    if (NULL ==  locator)
    {
        printf("# add_serial_locator(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    locator_id_t id = create_serial(locator->public_._.string_locator.value,
                                    ++g_loc_counter,
                                    &(locator->private_._.serial));

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


locator_id_t add_locator_serial(const char* device, MicroRTPSLocator* locator)
{
    if (NULL ==  locator)
    {
        printf("# add_serial_locator(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (TRANSPORT_ERROR == init_locator_serial(device, locator))
    {
        printf("# add_serial_locator(): BAD INIT!\n");
        return TRANSPORT_ERROR;
    }

    return add_init_locator_serial(locator);
}


locator_id_t add_locator(locator_kind_t kind, MicroRTPSLocator* const locator)
{
    if (NULL == locator)
    {
        printf("# add_locator_for_client(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    switch (kind)
    {
        case LOC_SERIAL:     return add_init_locator_serial   (locator);
        case LOC_UDP_AGENT:  return add_init_locator_udp_agent(locator);
        case LOC_UDP_CLIENT: return add_init_locator_udp_client(locator);

        default:
        break;
    }

    return TRANSPORT_ERROR;
}


int remove_locator(const locator_id_t locator_id)
{
    int ret = 0;
    switch (get_kind(locator_id))
    {
        case LOC_SERIAL:     ret = remove_serial(locator_id); break;
        case LOC_UDP_AGENT:
        case LOC_UDP_CLIENT: ret = remove_udp(locator_id);    break;

        default:             return TRANSPORT_ERROR;
    }

    // Remove reference
    if (TRANSPORT_OK == ret)
    {
        for (uint8_t i = 0; i < g_loc_counter; ++i)
        {
            if (g_loc_ids[i].id == locator_id)
            {
                g_loc_ids[i].id = 0;
                g_loc_ids[i].kind = LOC_NONE;
            }
        }
    }

    return ret;
}


int send_data(const octet_t* in_buffer, const size_t buffer_len, const locator_id_t locator_id)
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
        case LOC_SERIAL:     return send_serial(&header, in_buffer, buffer_len, locator_id);
        case LOC_UDP_AGENT:
        case LOC_UDP_CLIENT: return send_udp   (&header, in_buffer, buffer_len, locator_id);

        default:         return TRANSPORT_ERROR;
    }
}


int receive_data_timed(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms)
{
    if (NULL == out_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    switch (get_kind(locator_id))
    {
        case LOC_SERIAL:     return receive_serial(out_buffer, buffer_len, locator_id, timeout_ms);
        case LOC_UDP_AGENT:
        case LOC_UDP_CLIENT: return receive_udp   (out_buffer, buffer_len, locator_id, timeout_ms);

        default:             return TRANSPORT_ERROR;
    }
}


int receive_data(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id)
{
    return receive_data_timed(out_buffer, buffer_len, locator_id, DFLT_POLL_MS);
}


int extract_message(octet_t* out_buffer, const size_t buffer_len, buffer_t* internal_buffer)
{
    if (NULL == out_buffer || NULL == internal_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    octet_t* rx_buffer = internal_buffer->buffer;
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
