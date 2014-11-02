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

std::ofstream log;

int server(int connect_port)
{
    // Buffer for WSA and other metadata
    char buff[1024];

    // Opening file for logging
    log.open("log.txt", std::ios::app);

    std::cout << "\nTCP SERVER STARTED\n";

    // Sockets library initialisation
    if (WSAStartup(0x0202, reinterpret_cast<WSADATA*> (&buff[0]))) {
        // Error
        std::cerr << " Error WSA-Startup! ";
        log << " Error WSA-Startup! ";
        return -1;
    }

    // Creating socket
    SOCKET mysocket;

    // AF_INET - internet socket
    // SOCK_STREAM - stream socket (with creating a connection)
    // 0 - default TCP protocol
    if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // Error
        std::cerr << " Error socket! ";
        log << " Error socket! ";

        // Socket library deinitialisation
        WSACleanup();
        return -1;
    }

    // Binding the socket with local address
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(connect_port);
    local_addr.sin_addr.s_addr = 0;

    // Binding for accepting connections
    if (bind(mysocket,
             reinterpret_cast<sockaddr*> (&local_addr),
             sizeof(local_addr))) {
        // Error
        std::cerr << " Error bind! ";
        log << " Error bind! ";

        // Closing the socket
        closesocket(mysocket);
        WSACleanup();

        return -1;
    }

    // Waiting for connections
    // Size of queue 0x100
    if (listen(mysocket, 0x100)) {
        // Error
        std::cerr << " Error listen! ";
        log << " Error listen! ";

        // Closing the socket
        closesocket(mysocket);
        WSACleanup();

        return -1;
    }

    std::cout << "Waiting for connections\n";

    // Socket for client
    SOCKET client_socket;
    // Address of client
    sockaddr_in client_addr;
    // Size of client address
    int client_addr_size = sizeof(client_addr);

    // Cycle for accepting connections
    for ( ; ; ) {
        client_socket = accept(mysocket,
                               reinterpret_cast<sockaddr*> (&client_addr),
                               &client_addr_size);

        // Printing the client info
        std::cout << " " << __TIME__ << " ";
        std::cout << " [" << inet_ntoa(client_addr.sin_addr) << "] ";
        log << " " << __TIME__ << " ";
        log << " [" << inet_ntoa(client_addr.sin_addr) << "] ";

        // Creating new thread for client service
        DWORD thID;
        CreateThread(NULL, NULL, client_service, &client_socket, NULL, &thID);
    }

    return 0;
}

// This function is being created in new thread
// and is servicing the client (regardless of other)
DWORD WINAPI client_service(LPVOID client_socket)
{
    uint64_t start = tick();
    // Buffer for sending and receiving data
    char packet_buff[PACKET_BUFF_SIZE];

    // Creating socket
    SOCKET my_sock = (reinterpret_cast<SOCKET*> (client_socket))[0];
    int bytes_recv;

    // Creating the filestream and file
    std::fstream tmpfile("tmp.xml", std::ios::out);

    // Receiving the file from client
    while ((bytes_recv = recv(my_sock,
                              &packet_buff[0],
                              sizeof(packet_buff),
                              0))) {
        if (packet_buff[0] == '~' && packet_buff[1] == '~') break;

        tmpfile << packet_buff << std::endl;
        memset(packet_buff, '\0', PACKET_BUFF_SIZE);
    }

    tmpfile.close();

    // Using to sending the validation result
    tmpfile.open("tmp.xml", std::ios::in);
    if (tmpfile.is_open() != 1) {
        std::cerr << "File not found!!!\n";
    }

    ParseEventTracker events;
    Document *pDoc;
    bool is_eof = false;
    bool validating;

    while (is_eof != 1) {
        tmpfile.read(packet_buff, PACKET_BUFF_SIZE);

        is_eof = tmpfile.eof();

        pDoc = Parser::loadXML(packet_buff, &events);
        // pDoc -> dump_tag_tree( pDoc -> get_root(), 0 );

        memset(packet_buff, '\0', PACKET_BUFF_SIZE);
    }

    validating = events.result();

    if (validating == true) {
        send(my_sock, "File is valid!", PACKET_BUFF_SIZE, 0);
        std::cout << " File is valid! ";
        log << " File is valid! ";
    } else if (validating == false) {
        send(my_sock, "File is invalid!", PACKET_BUFF_SIZE, 0);
        std::cout << " File is invalid! ";
        log << " File is invalid! ";
    }

    uint64_t end = tick();
    std::cout << end - start << "\n";
    log << end - start << "\n";

    // Closing the file
    tmpfile.close();
    remove("tmp.xml");
    log.close();
    log.open("log.txt", std::ios::app);

    // Closing the socket
    closesocket(my_sock);

    return 0;
}
#endif  // _WIN32
