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

#define RTSP_MAX_REQUEST_LEN	4096
#define RTSP_MAX_RESPONSE_LEN	4096
#define MAX_METHOD_LEN		32
#define MAX_URI_LEN		64
#define MAX_VERSION_LEN		4
#define MAX_USER_AGENT_LEN	64
#define MAX_SESSION_ID_LEN	16
#define MAX_URL_LEN		64
#define MAX_HEADER_NAME_LEN	4096 /* Total overkill */
#define MAX_HEADER_VALUE_LEN	4096
#define MAX_INSTANCE_LEN	32
#define MAX_FILE_NAME_LEN	255

// #define HTTPD_TEST

#ifndef HTTPD_TEST

#define CLIENT_HOST "10.0.1.167"
#define SERVER_HOST "10.0.1.192"
#define SERVER_PORT 5000

#else /* #ifndef HTTPD_TEST */

#define CLIENT_HOST "127.0.0.1"
#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 80

#endif /* #ifndef HTTPD_TEST */

utility_retcode_t get_pcm_data_file(char *s, size_t size);
utility_retcode_t get_client_name(char *s, size_t size);
utility_retcode_t get_client_host(char *s, size_t size);
utility_retcode_t get_client_version(char *s, size_t size);
utility_retcode_t get_user_agent(char *s, size_t size);
utility_retcode_t get_client_instance(char *s, size_t size);
utility_retcode_t get_server_name(char *s, size_t size);
utility_retcode_t get_server_host(char *s, size_t size);
utility_retcode_t get_server_port(short *i);
utility_retcode_t get_server_encoded_rsa_public_modulo(char *s, size_t size);
utility_retcode_t get_server_encoded_rsa_public_exponent(char *s, size_t size);

#endif /* #ifndef CONFIG_H */
