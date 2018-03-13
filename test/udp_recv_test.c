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

#include <micrortps/transport/micrortps_transport.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    printf("\nAt the very beginning everything was black\n\n");

    if (argc < 2) return -1;

    octet_t buffer[256];
    int len = 0;
    MicroRTPSLocator locator;

    uint16_t port = strtoul (argv[1], NULL, 0);
    locator_id_t loc_id = add_locator_udp_agent(port, &locator);

    int loops = 0;
    while (++loops <= 1000)
    {
        if (0 < (len = receive_data(buffer, sizeof(buffer), loc_id)))
        {
            printf(">> '%s'\n", buffer);
            strcpy(buffer, "Message from receiver");
            while (0 >= send_data(buffer, strlen(buffer) + 1, loc_id)) ms_sleep(100);
            printf("<< '%s'\n", buffer);
        }
        else if (0 > len)
        {
            printf("ERROR\n");
        }
        fflush(stdout);
        ms_sleep(100);
    }

    printf("exiting...\n");
    return 0;
}
