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
#include <unistd.h>

#include <transport/ddsxrce_transport.h>


int main(int argc, char *argv[])
{
    printf("\nAt the very beginning everything was black\n\n");

    if (argc < 2) return -1;

    octet buffer[1024] = {};
    int len = 0;

    locator_id_t loc_id = add_udp_locator(argv[1], 2019, 2020);

    //int loops = 1000;
    while (1)
    {
        if (0 < (len = receive_data(buffer, sizeof(buffer), loc_id)))
        {
            printf(">> '%s'\n", buffer);
        }
        else
        {
            //printf("# recv len %d\n", len);
        }
    }

    printf("exiting...\n");
    return 0;
}
