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

#include "micrortps_serial_transport.h"

static serial_channel_t* g_channels[MAX_NUM_CHANNELS];
static uint8_t g_num_channels = 0;

#ifndef _WIN32
static struct pollfd g_poll_fds[MAX_NUM_CHANNELS] = {};
#endif

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(uint8_t const *buffer, size_t len);
int extract_message(octet_t* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);

locator_id_t create_serial (const char* device, locator_id_t locator_id);
int          destroy_serial(const locator_id_t locator_id);
int          open_serial   (serial_channel_t* channel);
int          close_serial  (serial_channel_t* channel);
int          send_serial   (const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id);
int          receive_serial(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id);

serial_channel_t* get_serial_channel(const locator_id_t locator_id);
int read_serial(void *buffer, const size_t len, serial_channel_t* channel);
int write_serial(const void* buffer, const size_t len, serial_channel_t* channel);

serial_channel_t* get_serial_channel(const locator_id_t locator_id)
{
    serial_channel_t* ret = NULL;
#ifdef _WIN32
    return ret;
#else
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
#endif
}

locator_id_t create_serial(const char* device, locator_id_t loc_id)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == device || MAX_NUM_CHANNELS <= g_num_channels)
    {
        return TRANSPORT_ERROR;
    }

    serial_channel_t* channel = malloc(sizeof(serial_channel_t));
    if (NULL == channel)
    {
        return TRANSPORT_ERROR;
    }

    memset(channel->rx_buffer.buffer, 0, RX_BUFFER_LENGTH);
    channel->rx_buffer.buff_pos = 0;
    memcpy(channel->uart_name, device, UART_NAME_MAX_LENGTH);
    channel->uart_fd = -1;
    channel->baudrate = DFLT_BAUDRATE;
    channel->poll_ms = DFLT_POLL_MS;
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

    return channel->locator_id;
#endif
}

int destroy_serial(const locator_id_t loc_id)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    serial_channel_t* channel = get_serial_channel(loc_id);
    if (NULL == channel)
    {
        return TRANSPORT_ERROR;
    }

    if (channel->open)
    {
        close_serial(channel);
    }
    g_channels[channel->idx] = NULL;
    free(channel);

    return TRANSPORT_OK;
#endif
}

int open_serial(serial_channel_t* channel)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == channel)
    {
        return TRANSPORT_ERROR;
    }

    // Open a serial port
    channel->uart_fd = open(channel->uart_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (channel->uart_fd < 0)
    {
        printf("failed to open device: %s (%d)\n", channel->uart_name, errno);
        return -errno;
    }

    // Try to set baud rate
    struct termios uart_config;
    int termios_state;

    // Back up the original uart configuration to restore it after exit
    if ((termios_state = tcgetattr(channel->uart_fd, &uart_config)) < 0)
    {
        int errno_bkp = errno;
        printf("ERR GET CONF %s: %d (%d)\n", channel->uart_name, termios_state, errno);
        close_serial(channel);
        return -errno_bkp;
    }

    // Clear ONLCR flag (which appends a CR for every LF)
    uart_config.c_oflag &= ~ONLCR;

    // USB serial is indicated by /dev/ttyACM0
    if (strcmp(channel->uart_name, "/dev/ttyACM0") != 0 && strcmp(channel->uart_name, "/dev/ttyACM1") != 0)
    {
        // Set baud rate
        if (cfsetispeed(&uart_config, channel->baudrate) < 0 || cfsetospeed(&uart_config, channel->baudrate) < 0)
        {
            int errno_bkp = errno;
            printf("ERR SET BAUD %s: %d (%d)\n", channel->uart_name, termios_state, errno);
            close_serial(channel);
            return -errno_bkp;
        }
    }

    if ((termios_state = tcsetattr(channel->uart_fd, TCSANOW, &uart_config)) < 0)
    {
        int errno_bkp = errno;
        printf("ERR SET CONF %s (%d)\n", channel->uart_name, errno);
        close_serial(channel);
        return -errno_bkp;
    }

    char aux[64];
    bool flush = false;
    while (0 < read(channel->uart_fd, (void *)&aux, 64))
    {
        //printf("%s", aux);
        flush = true;
        ms_sleep(1);
    }

    if (flush)
    {
        printf("flush\n");

    } else
    {
        printf("no flush\n");
    }

    channel->open = true;
    g_poll_fds[channel->idx].fd = channel->uart_fd;
    g_poll_fds[channel->idx].events = POLLIN;

    return channel->uart_fd;
#endif
}

int close_serial(serial_channel_t* channel)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == channel || 0 > channel->uart_fd)
    {
        return TRANSPORT_ERROR;
    }

    printf("Close UART\n");
    close(channel->uart_fd);
    channel->uart_fd = -1;
    channel->open = false;
    memset(&g_poll_fds[channel->idx], 0, sizeof(struct pollfd));

    return 0;
#endif
}

int read_serial(void *buffer, const size_t len, serial_channel_t* channel)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == buffer       ||
        NULL == channel      ||
        (!channel->open && 0 > open_serial(channel)))
    {
        return TRANSPORT_ERROR;
    }

    // TODO: for several channels this can be optimized
    int ret = 0;
    int r = poll(g_poll_fds, g_num_channels, channel->poll_ms);
    if (r > 0 && (g_poll_fds[channel->idx].revents & POLLIN))
    {
        ret = read(channel->uart_fd, buffer, len);
    }

    return ret;
#endif
}


int receive_serial(octet_t* out_buffer, const size_t buffer_len, const locator_id_t loc_id)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == out_buffer)
    {
        return TRANSPORT_ERROR;
    }

    serial_channel_t* channel = get_serial_channel(loc_id);
    if (NULL == channel      ||
        (!channel->open && 0 > open_serial(channel)))
    {
        return TRANSPORT_ERROR;
    }

    int len = read_serial((void *) (channel->rx_buffer.buffer + channel->rx_buffer.buff_pos),
                           sizeof(channel->rx_buffer.buffer) - channel->rx_buffer.buff_pos, channel);
    if (len <= 0)
    {
        int errsv = errno;

        if (errsv && EAGAIN != errsv && ETIMEDOUT != errsv)
        {
            printf("Read fail %d\n", errsv);
        }
    }

    // We read some bytes, trying extract a whole message
    if (0 < len) channel->rx_buffer.buff_pos += len;
    return extract_message(out_buffer, buffer_len, &channel->rx_buffer);
#endif
}

int write_serial(const void* buffer, const size_t len, serial_channel_t* channel)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == buffer       ||
        NULL == channel      ||
        (!channel->open && 0 > open_serial(channel)))
    {
        return TRANSPORT_ERROR;
    }

    return write(channel->uart_fd, buffer, len);
#endif
}

int send_serial(const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t loc_id)
{
#ifdef _WIN32
    return TRANSPORT_ERROR;
#else
    if (NULL == in_buffer)
    {
        return TRANSPORT_ERROR;
    }

    serial_channel_t* channel = get_serial_channel(loc_id);
    if (NULL == channel      ||
        (!channel->open && 0 > open_serial(channel)))
    {
        return TRANSPORT_ERROR;
    }

    int len = write_serial(header, sizeof(header_t), channel);
    if (len != sizeof(header_t))
    {
        return len;
    }

    len = write_serial(in_buffer, length, channel);
    if (len != length)
    {
        return len;
    }

    return len; // only payload, + sizeof(header); for real size.
#endif
}
