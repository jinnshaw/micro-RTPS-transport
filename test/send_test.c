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

#include <transport/ddsxrce_transport.h>

int main(int argc, char *argv[])
{
    printf("\nAt the very beginning everything was black\n\n");

    if (argc < 2) return -1;

    octet buffer[256];
    int len = 0;

    locator_id_t loc_id = add_udp_locator(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), argv[4]);

    int loops = 1000;
    while (loops--)
    {
        strcpy(buffer, "Mensaje___del_client_");
        ++buffer[8];
        if (0 < (len = send_data(buffer, strlen("Mensaje___del_client") + 1, loc_id)))
        {
            printf("<< '%s'\n", buffer);
            while (0 >= receive_data(buffer, sizeof(buffer), loc_id)) eSleep(10000);
            printf(">> '%s'\n", buffer);

        }
        else
        {
            printf("ERROR\n");
        }

        eSleep(1000000);
    }

    printf("exiting...\n");
    return 0;
}
