/********************************************************************
 * Copyright 2014 Sasha Halchin.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 ********************************************************************/

#ifndef TRLWO_1286_INCLUDE_APP_UNIX_H_
#define TRLWO_1286_INCLUDE_APP_UNIX_H_

#ifdef __unix__
#include <stdio.h>
#include <memory.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include "xmlparser.h"

#define PACKET_BUFF_SIZE 5120

// Server function
int server(int connect_port);

// Client function
int client(int connect_port, const char *server_address, const char *file_name);

// Function for service the connected users
void* client_service(void *client_socket);

// Function for getting handling time
inline uint64_t tick()
{
    uint64_t time;
    __asm__ __volatile__("rdtsc" : "=A" (time));
    return time;
}

#endif  // __unix__

#endif  // TRLWO_1286_INCLUDE_APP_UNIX_H_
