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

#include <iostream>
#include <string>

#ifdef __unix__
#include "include/app_unix.h"
#elif(defined _WIN32)
#include "include/app_win.h"
#else
#error "unknown OS"
#endif

int main()
{
    char change;
    int port;
    std::string ip;
    std::string file_name;

    std::cin >> change >> port >> ip >> file_name;

    if (change == 's') {
        server(port);
    } else if (change == 'c') {
        if (ip == "localhost") {
            client(port, "127.0.0.1", file_name.c_str());
        } else {
            client(port, ip.c_str(), file_name.c_str());
        }
    } else {
        std::cerr << "Error! Missing change!\n";
    }

    return 0;
}
