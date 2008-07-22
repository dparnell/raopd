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
#ifndef CLIENT_H
#define CLIENT_H

#include "rtsp.h"

utility_retcode_t connect_server(struct rtsp_session *session);
utility_retcode_t connect_to_port(char *host, short port, int *fd);
utility_retcode_t send_request(struct rtsp_request *request);
utility_retcode_t read_response(struct rtsp_response *response);

#endif /* #ifndef CLIENT_H */
