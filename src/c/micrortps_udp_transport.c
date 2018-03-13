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

#include "micrortps_udp_transport.h"

#ifndef __PX4_NUTTX

static udp_channel_t* g_channels[MAX_NUM_CHANNELS];
static uint8_t g_num_channels = 0;
static struct pollfd g_poll_fds[MAX_NUM_CHANNELS];

#ifdef _WIN32
static WSADATA wsa;
#endif

#endif

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(const uint8_t* buffer, size_t len);
int extract_message(octet_t* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);
int init_udp(udp_channel_t* channel);

locator_id_t create_udp (uint16_t local_udp_port,
                         uint16_t remote_udp_port, const uint8_t* remote_ip,
                         locator_id_t loc_id, udp_channel_t* channel);
int          remove_udp (const locator_id_t locator_id);
int          open_udp   (udp_channel_t* channel);
int          close_udp  (udp_channel_t* channel);
int          send_udp   (const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id);
int          receive_udp(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms);

udp_channel_t* get_udp_channel(const locator_id_t locator_id);
int read_udp(udp_channel_t* channel);
int write_udp(const void* buffer, const size_t len, udp_channel_t* channel);

udp_channel_t* get_udp_channel(const locator_id_t locator_id)
{
#ifndef __PX4_NUTTX
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
#endif /* __PX4_NUTTX */

    return NULL;
}

locator_id_t create_udp(uint16_t local_udp_port,
                        uint16_t remote_udp_port, const uint8_t* remote_ip,
                        locator_id_t loc_id, udp_channel_t* channel)
{
#ifndef __PX4_NUTTX

    if (0 >= loc_id || NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    // Fill channel struct
    memset(channel->rx_buffer.buffer, 0, RX_BUFFER_LENGTH);
    channel->rx_buffer.buff_pos = 0;
    channel->socket_fd = -1;
    channel->local_udp_port = local_udp_port;
    channel->remote_udp_port = remote_udp_port;
    channel->poll_ms = DFLT_POLL_MS;
    channel->open = false;
    if (NULL != remote_ip)
    {
        memcpy(channel->remote_ip, remote_ip, IP_LENGTH);
    }

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
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int init_udp(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if (NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

#ifdef _WIN32
    //Initialise winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed initialising Winsock, error code: %d\n",WSAGetLastError());
        return TRANSPORT_ERROR;
    }
#endif

    if (0 > (channel->socket_fd = socket(AF_INET, SOCK_DGRAM, 0)))

    {
        printf("# create socket failed\n");
        return TRANSPORT_ERROR;
    }

    memset((char *)&channel->local_addr, 0, sizeof(channel->local_addr));
    memset((char *)&channel->remote_addr, 0, sizeof(channel->remote_addr));
    channel->local_addr.sin_family = AF_INET;
    channel->remote_addr.sin_family = AF_INET;
    channel->local_addr.sin_port = htons(channel->local_udp_port);
    channel->local_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 - To use whatever interface (IP) of the system


    if (0 == strlen(channel->remote_ip)) // AGENT
    {
        if (0 > bind(channel->socket_fd, (struct sockaddr *)&channel->local_addr, sizeof(channel->local_addr)))
        {
            printf("# bind failed, socket_fd: '%d' port: '%u'\n", channel->socket_fd, channel->local_udp_port);
            return TRANSPORT_ERROR;
        }

        #ifdef TRANSPORT_LOGS
        printf("> Agent locator initialized on port %d\n", channel->local_udp_port);
        #endif
    }
    else // CLIENT
    {
        channel->remote_addr.sin_port = htons(channel->remote_udp_port);
        memcpy((void*) &channel->remote_addr.sin_addr, channel->remote_ip, sizeof(channel->remote_addr.sin_addr));

        #ifdef TRANSPORT_LOGS
        printf("> Client initialized on port %d\n", channel->local_udp_port);
        #endif
    }

    return TRANSPORT_OK;

#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int remove_udp(const locator_id_t loc_id)
{
#ifndef __PX4_NUTTX
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

    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int open_udp(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if (NULL == channel ||
        0 > init_udp(channel))
    {
        printf("# ERROR OPENIG UDP CHANNEL\n");
        return TRANSPORT_ERROR;
    }

    #ifdef TRANSPORT_LOGS
    printf("> UDP channel opened\n");
    #endif
    channel->open = true;
    g_poll_fds[channel->idx].fd = channel->socket_fd;
    g_poll_fds[channel->idx].events = POLLIN; // +V+ POLLPRI
    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int close_udp(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if (NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (0 <= channel->socket_fd)
    {
        #ifdef TRANSPORT_LOGS
        printf("> Close socket\n");
        #endif

        #ifdef _WIN32
        shutdown(channel->socket_fd, SD_BOTH);
        closesocket(channel->socket_fd);
        WSACleanup();
        #else
        shutdown(channel->socket_fd, SHUT_RDWR);
        close(channel->socket_fd);
        #endif

        channel->socket_fd = -1;
    }

    channel->open = false;
    memset(&g_poll_fds[channel->idx], 0, sizeof(struct pollfd));
    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int read_udp(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if ( NULL == channel      ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error read UDP channel\n");
        return TRANSPORT_ERROR;
    }

    // TODO: for several channels this can be optimized
    int ret = 0;
    #ifdef _WIN32
    int addrlen = sizeof(channel->remote_addr);
    int r = WSAPoll(g_poll_fds, g_num_channels, channel->poll_ms);
    #else
    static socklen_t addrlen = sizeof(channel->remote_addr);
    int r = poll(g_poll_fds, g_num_channels, channel->poll_ms);
    #endif

    if (r > 0 && (g_poll_fds[channel->idx].revents & POLLIN))
    {
        ret = recvfrom(channel->socket_fd,
                       (void *) (channel->rx_buffer.buffer + channel->rx_buffer.buff_pos),
                       sizeof(channel->rx_buffer.buffer) - channel->rx_buffer.buff_pos,
                       0,
                       (struct sockaddr *) &channel->remote_addr,
                       &addrlen);
    }

    return ret;

#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}


int receive_udp(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms)
{
#ifndef __PX4_NUTTX

    if (NULL == out_buffer)
    {
        printf("# receive_udp(): BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    udp_channel_t* channel = get_udp_channel(locator_id);
    if (NULL == channel ||
        (!channel->open && 0 > open_udp(channel)))
    {
        return TRANSPORT_ERROR;
    }

    channel->poll_ms = timeout_ms;

    int len = read_udp(channel);
    if (len <= 0)
    {
        int errsv = errno;

        if (errsv && EAGAIN != errsv && ETIMEDOUT != errsv)
        {
            printf("# Read fail %d\n", errsv);
        }
    }
    else
    {
        // We read some bytes, trying extract a whole message
        channel->rx_buffer.buff_pos += len;
    }

    return extract_message(out_buffer, buffer_len, &channel->rx_buffer);

#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int write_udp(const void* buffer, const size_t len, udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if ( NULL == buffer       ||
         NULL == channel      ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error write UDP channel\n");
        return TRANSPORT_ERROR;
    }

    if (0 == channel->remote_addr.sin_addr.s_addr)
    {
        printf("# Error write UDP channel, do not exist a send address\n");
        return TRANSPORT_ERROR;
    }

    return sendto(channel->socket_fd, buffer, len, 0, (struct sockaddr *)&channel->remote_addr, sizeof(channel->remote_addr));

#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int send_udp(const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t loc_id)
{
#ifndef __PX4_NUTTX
    if (NULL == in_buffer)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    udp_channel_t* channel = get_udp_channel(loc_id);
    if ( NULL == channel      ||
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
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}
