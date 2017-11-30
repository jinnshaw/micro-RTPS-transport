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

#ifndef __PX4_NUTTX
static udp_channel_t* g_channels[MAX_NUM_CHANNELS];
static struct pollfd g_poll_fds[MAX_NUM_CHANNELS] = {};
static uint8_t g_num_channels = 0;
#endif

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(uint8_t const *buffer, size_t len);
int extract_message(octet* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);
int init_receiver(udp_channel_t* channel);
int init_sender(udp_channel_t* channel);

locator_id_t create_udp (const char* server_ip, uint16_t udp_port_recv, uint16_t udp_port_send, locator_id_t locator_id);
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

locator_id_t create_udp(const char* server_ip, uint16_t udp_port_recv, uint16_t udp_port_send, locator_id_t loc_id)
{
#ifndef __PX4_NUTTX
    if (NULL == server_ip || 0 > loc_id)
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
    channel->poll_ms = DFLT_POLL_MS;
    channel->open = false;
    strncpy(channel->server_ip, server_ip, strlen(server_ip) + 1);

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

int init_receiver(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
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

    if (0 > (channel->receiver_fd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        printf("# create socket failed\n");
        return TRANSPORT_ERROR;
    }

    if (0 > bind(channel->receiver_fd, (struct sockaddr *)&channel->receiver_inaddr, sizeof(channel->receiver_inaddr)))
    {
        printf("# bind failed\n");
        return TRANSPORT_ERROR;
    }

    if (0 > listen(channel->receiver_fd, MAX_PENDING_CONNECTIONS))
    {
        printf("# listen failed\n");
        return TRANSPORT_ERROR;
    }

    #ifdef TRANSPORT_LOGS
    printf("> Receiver initialized on port %d\n", channel->udp_port_recv);
    #endif
    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int init_sender(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if (NULL == channel)
    {
        printf("# BAD PARAMETERS!\n");
        return TRANSPORT_ERROR;
    }

    if (0 > (channel->sender_fd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        printf("> create socket failed\n");
        return TRANSPORT_ERROR;
    }

    memset((char *) &channel->sender_outaddr, 0, sizeof(channel->sender_outaddr));
    channel->sender_outaddr.sin_family = AF_INET;
    channel->sender_outaddr.sin_port = htons(channel->udp_port_send);
    if (0 >= inet_pton(AF_INET, channel->server_ip, &channel->sender_outaddr.sin_addr))
    {
        printf("# inet_aton() failed\n");
        return TRANSPORT_ERROR;
    }

    if(0 > connect(channel->sender_fd, (struct sockaddr *)&channel->sender_outaddr, sizeof(channel->sender_outaddr)))
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    #ifdef TRANSPORT_LOGS
    printf("> Sender initialized on port %d\n", channel->udp_port_send);
    #endif
    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int destroy_udp(const locator_id_t loc_id)
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
    free(channel);

    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int open_udp(udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
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
    g_poll_fds[channel->idx].fd = channel->receiver_fd;
    g_poll_fds[channel->idx].events = POLLIN;
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
    memset(&g_poll_fds[channel->idx], 0, sizeof(struct pollfd));
    return TRANSPORT_OK;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int read_udp(void *buffer, const size_t len, udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
    if (NULL == buffer       ||
        NULL == channel      ||
        (!channel->open && 0 > open_udp(channel)))
    {
        printf("# Error read UDP channel\n");
        return TRANSPORT_ERROR;
    }

    // TODO: for several channels this can be optimized
    int ret = 0;
    static socklen_t addrlen = sizeof(channel->receiver_outaddr);
    int r = poll(g_poll_fds, g_num_channels, channel->poll_ms);
    if (r > 0 && (g_poll_fds[channel->idx].revents & POLLIN))
    {
        ret = recvfrom(channel->receiver_fd, buffer, len, 0, (struct sockaddr *) &channel->receiver_outaddr, &addrlen);
    }

    return ret;
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}


int receive_udp(octet* out_buffer, const size_t buffer_len, const locator_id_t loc_id)
{
#ifndef __PX4_NUTTX
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
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int write_udp(const void* buffer, const size_t len, udp_channel_t* channel)
{
#ifndef __PX4_NUTTX
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
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}

int send_udp(const header_t* header, const octet* in_buffer, const size_t length, const locator_id_t loc_id)
{
#ifndef __PX4_NUTTX
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
#endif /* __PX4_NUTTX */

    return TRANSPORT_ERROR;
}
