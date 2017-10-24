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
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>

#include "ddsxrce_udp_transport.h"
#include "ddsxrce_transport_common.h"

static udp_channel_t* g_channels[MAX_NUM_CHANNELS];
static uint8_t g_num_channels = 0;

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(uint8_t const *buffer, size_t len);
int extract_message(octet* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);

locator_id_t create_udp (uint16_t udp_port_recv, uint16_t udp_port_send, locator_id_t locator_id);
int          destroy_udp(const locator_id_t locator_id);
int          open_udp   (udp_channel_t* channel);
int          close_udp  (udp_channel_t* channel);
int          send_udp   (const header_t* header, const octet* in_buffer, const size_t length, const locator_id_t locator_id);
int          receive_udp(octet* out_buffer, const size_t buffer_len, const locator_id_t locator_id);

udp_channel_t* get_udp_channel(const locator_id_t locator_id);
int read_udp(void *buffer, const size_t len, udp_channel_t* channel);
int write_udp(const void* buffer, const size_t len, udp_channel_t* channel);

udp_channel_t* get_udp_channel(const locator_id_t locator_id)
{
    udp_channel_t* ret = NULL;
    for (int i = 0; i < g_num_channels; ++i)
    {
        if (NULL != g_channels[i] &&
            g_channels[i]->locator_id == locator_id)
        {
            ret = g_channels[i];
            break;
        }
    }
    return ret;
}
locator_id_t create_udp(uint16_t udp_port_recv, uint16_t udp_port_send, locator_id_t loc_id)
{
    if (0 > loc_id)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    udp_channel_t* channel = malloc(sizeof(udp_channel_t));
    if (NULL == channel)
    {
        return TRANSPORT_ERROR;
    }

    memset(channel->rx_buffer.buffer, 0, RX_BUFFER_LENGTH);
    channel->rx_buffer.buff_pos = 0;
    channel->sender_fd = -1;
    channel->receiver_fd = -1;
    channel->udp_port_recv = udp_port_recv;
    channel->udp_port_send = udp_port_send;
    channel->open = false;

    for (int i = 0; i < MAX_NUM_CHANNELS; ++i)
    {
        if (NULL == g_channels[i])
        {
            channel->locator_id = loc_id;
            channel->idx = i;
            g_channels[i] = channel;
            ++g_num_channels;
            break;
        }
    }

    #ifdef TRANSPORT_LOGS
    printf("> Create udp channel id: %d\n", channel->locator_id);
    #endif
    return channel->locator_id;
}

int init_receiver(udp_channel_t* channel)
{
    if (NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    // udp socket data
    memset((char *)&channel->receiver_inaddr, 0, sizeof(channel->receiver_inaddr));
    channel->receiver_inaddr.sin_family = AF_INET;
    channel->receiver_inaddr.sin_port = htons(channel->udp_port_recv);
    channel->receiver_inaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (0 > (channel->receiver_fd = socket(AF_INET, SOCK_DGRAM, 0)))
    {
        printf("# create socket failed\n");
        return TRANSPORT_ERROR;
    }

    if (0 > bind(channel->receiver_fd, (struct sockaddr *)&channel->receiver_inaddr, sizeof(channel->receiver_inaddr)))
    {
        printf("# bind failed\n");
        return TRANSPORT_ERROR;
    }

    #ifdef TRANSPORT_LOGS
    printf("> Receiver initialized on port %d\n", channel->udp_port_recv);
    #endif
    return TRANSPORT_OK;
}

int init_sender(udp_channel_t* channel)
{
    if (NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (0 > (channel->sender_fd = socket(AF_INET, SOCK_DGRAM, 0)))
    {
        printf("> create socket failed\n");
        return TRANSPORT_ERROR;
    }

    memset((char *) &channel->sender_outaddr, 0, sizeof(channel->sender_outaddr));
    channel->sender_outaddr.sin_family = AF_INET;
    channel->sender_outaddr.sin_port = htons(channel->udp_port_send);

    if (0 == inet_aton("127.0.0.1", &channel->sender_outaddr.sin_addr))
    {
        printf("# inet_aton() failed\n");
        return TRANSPORT_ERROR;
    }

    #ifdef TRANSPORT_LOGS
    printf("> Sender initialized on port %d\n", channel->udp_port_send);
    #endif
    return TRANSPORT_OK;
}

int destroy_udp(const locator_id_t loc_id)
{
    udp_channel_t* channel = get_udp_channel(loc_id);
    if (NULL == channel)
    {
        return TRANSPORT_ERROR;
    }

    if (channel->open)
    {
        close_udp(channel);
    }
    g_channels[channel->idx] = NULL;
    free(channel);

    return TRANSPORT_OK;
}

int open_udp(udp_channel_t* channel)
{
    if (NULL == channel ||
        0 > init_receiver(channel) ||
        0 > init_sender(channel))
    {
        printf("# ERROR OPENIG UDP CHANNEL\n");
        return TRANSPORT_ERROR;
    }

    #ifdef TRANSPORT_LOGS
    printf("> UDP channel opened\n");
    #endif
    channel->open = true;
    return TRANSPORT_OK;
}

int close_udp(udp_channel_t* channel)
{
    if (NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (0 <= channel->sender_fd)
    {
        #ifdef TRANSPORT_LOGS
        printf("> Close sender socket\n");
        #endif
        shutdown(channel->sender_fd, SHUT_RDWR);
        close(channel->sender_fd);
        channel->sender_fd = -1;
    }

    if (0 <= channel->receiver_fd)
    {
        #ifdef TRANSPORT_LOGS
        printf("> Close receiver socket\n");
        #endif
        shutdown(channel->receiver_fd, SHUT_RDWR);
        close(channel->receiver_fd);
        channel->receiver_fd = -1;
    }

    channel->open = false;
    return TRANSPORT_OK;
}

int read_udp(void *buffer, const size_t len, udp_channel_t* channel)
{
    if (NULL == buffer       ||
        NULL == channel      ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error read UDP channel\n");
        return TRANSPORT_ERROR;
    }

    // TODO: for several channels this can be optimized
    int ret = 0;
    // Blocking call
    static socklen_t addrlen = sizeof(channel->receiver_outaddr);
    ret = recvfrom(channel->receiver_fd, buffer, len, 0, (struct sockaddr *) &channel->receiver_outaddr, &addrlen);
    return ret;
}


int receive_udp(octet* out_buffer, const size_t buffer_len, const locator_id_t loc_id)
{
    if (NULL == out_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    udp_channel_t* channel = get_udp_channel(loc_id);
    if (NULL == channel ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error recv UDP channel\n");
        return TRANSPORT_ERROR;
    }

    octet* rx_buffer = channel->rx_buffer.buffer;
    uint16_t* rx_buff_pos = &(channel->rx_buffer.buff_pos);

    int len = read_udp((void *) (rx_buffer + (*rx_buff_pos)), sizeof(channel->rx_buffer.buffer) - (*rx_buff_pos), channel);
    if (len <= 0)
    {
        int errsv = errno;

        if (errsv && EAGAIN != errsv && ETIMEDOUT != errsv)
        {
            printf("# Read fail %d\n", errsv);
        }

        return len;
    }

    // We read some bytes, trying extract a whole message
    (*rx_buff_pos) += len;
    return extract_message(out_buffer, buffer_len, &channel->rx_buffer);
}

int write_udp(const void* buffer, const size_t len, udp_channel_t* channel)
{
    if (NULL == buffer       ||
        NULL == channel      ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error write UDP channel\n");
        return TRANSPORT_ERROR;
    }

    int ret = 0;
    ret = sendto(channel->sender_fd, buffer, len, 0, (struct sockaddr *)&channel->sender_outaddr, sizeof(channel->sender_outaddr));
    return ret;
}

int send_udp(const header_t* header, const octet* in_buffer, const size_t length, const locator_id_t loc_id)
{
    if (NULL == in_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    udp_channel_t* channel = get_udp_channel(loc_id);
    if (NULL == channel      ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error send UDP channel\n");
        return TRANSPORT_ERROR;
    }

    int len = write_udp(header, sizeof(header_t), channel);
    if (len != sizeof(header_t))
    {
        return len;
    }

    len = write_udp(in_buffer, length, channel);
    if (len != length)
    {
        return len;
    }

    return len; // only payload, + sizeof(header); for real size.
}
