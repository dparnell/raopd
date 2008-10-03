/* 
Copyright 2008, David Allan

This file is part of raopd.

raopd is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version.

raopd is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with raopd.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CONFIG_H
#define CONFIG_H

#include "utility.h"
#include "rtsp.h"

// #define HTTPD_TEST

#ifndef HTTPD_TEST

#define CLIENT_HOST "10.0.33.222"
#define SERVER_HOST "10.0.33.1"
#define SERVER_PORT 5000

#else /* #ifndef HTTPD_TEST */

#define CLIENT_HOST "127.0.0.1"
#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 80

#endif /* #ifndef HTTPD_TEST */

utility_retcode_t get_client(struct rtsp_client *client);
utility_retcode_t get_server(struct rtsp_server *server);
utility_retcode_t get_user_agent(struct rtsp_client *client);
utility_retcode_t get_client_instance(struct rtsp_client *client);
utility_retcode_t get_server_encoded_rsa_public_key(struct rtsp_server *server);

#endif /* #ifndef CONFIG_H */
