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

#ifndef TRLWO_1286_INCLUDE_APP_WIN_H_
#define TRLWO_1286_INCLUDE_APP_WIN_H_

#ifdef _WIN32
#include <stdio.h>
#include <time.h>

#include <winsock2.h>
#include <windows.h>

#include <iostream>
#include <fstream>

#include "xmlparser.h"

#pragma comment(lib, "WS2_32.Lib")
#define PACKET_BUFF_SIZE 5120

// Server function
int server(int connect_port);

// Client function
int client(int connect_port, const char *server_address, const char *file_name);

// Function for service the connected users
DWORD WINAPI client_service(LPVOID client_socket);

// Function for getting handling time
inline uint64_t tick()
{
    uint64_t time;
    __asm__ __volatile__("rdtsc" : "=A" (time));
    return time;
}

#endif  // _WIN32

#endif  // TRLWO_1286_INCLUDE_APP_WIN_H_
