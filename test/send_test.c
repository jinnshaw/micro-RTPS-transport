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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ddsxrce_transport.h"
#include "ddsxrce_transport_common.h"


int main(int argc, char *argv[])
{
    printf("\nAt the very beginning everything was black\n\n");

    if (argc < 2) return -1;

    octet buffer[256] = {"Mensaje_del_senderA"};
    size_t buffer_len = 256;
    int len = 0;

    locator_id_t loc_id = add_udp_locator(argv[1], 2020, 2019);

    int loops = 1000;
    while (loops--)
    {
        ++buffer[18];
        if (0 < (len = send_data(buffer, strlen("Mensaje_del_sender_") + 1, loc_id)))
        {
            printf("<< '%s'\n", buffer);
        }
        else
        {
            printf("# send len %d\n", len);
        }

        usleep(1000000);
    }

    printf("exiting...\n");
    return 0;
}
