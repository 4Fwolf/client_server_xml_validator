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

#ifdef _WIN32
#include "include/app_win.h"

int client(int connect_port, const char* server_address, const char* file_name)
{
    // Buffer for sending and receiving data
    char packet_buff[PACKET_BUFF_SIZE];

    // Buffer for WSA and other metadata
    char buff[1024];

    std::cout << "TCP CLIENT STARTED\n";

    // Sockets library initialisation
    if (WSAStartup(0x202, reinterpret_cast<WSADATA*> (&buff[0]))) {
        std::cout << "WSAStart error!\n";
        return -1;
    }

    // Creating socket
    SOCKET my_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (my_sock < 0) {
        std::cout << "Socket() error!\n";
        return -1;
    }

    // Creating connection
    sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(connect_port);

    // Converting IP from symbols to networking format
    if (inet_addr(server_address) != INADDR_NONE)
        dest_addr.sin_addr.s_addr = inet_addr(server_address);

    // Trying to connect the server
    if (connect(my_sock,
        reinterpret_cast<sockaddr*> (&dest_addr),
        sizeof(dest_addr))) {
      std::cout << "Connect error!\n";
      return -1;
    }

    std::cout << "Connection with " << server_address << " accepted\n\n";

    // Creating the filestream and opening a file
    std::ifstream file(file_name, std::ios::in);
    if (!file) {
        std::cerr << "Error file not found!\n";
        return -1;
    }

    // Sending the file
    while (file.getline(packet_buff, PACKET_BUFF_SIZE)) {
        send(my_sock, &packet_buff[0], PACKET_BUFF_SIZE, 0);
        memset(packet_buff, '\0', PACKET_BUFF_SIZE);
    }

    // Sending a command end of file (EOF)
    send(my_sock, "~~", 2, 0);

    // Closing the file
    file.close();

    recv(my_sock, &packet_buff[0], sizeof(packet_buff)-1, 0);
    std::cout << "Validation of file: " << packet_buff << std::endl;

    closesocket(my_sock);
    WSACleanup();

    return 0;
}
#endif  // _WIN32
