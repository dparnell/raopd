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
#include <unistd.h>
#include <arpa/inet.h>

#include "lt.h"
#include "utility.h"
#include "syscalls.h"
#include "client.h"
#include "config.h"
#include "rtsp.h"

#define DEFAULT_FACILITY LT_CLIENT

utility_retcode_t connect_server(struct rtsp_session *session)
{
	int ret;

	FUNC_ENTER;

	INFO("Connecting to server at \"%s\" port %d\n",
	     session->server->host, (int)session->server->port);

	ret = connect_to_port(session->server->host,
			      session->server->port,
			      &session->fd);

	INFO("fd for server connection: %d\n", session->fd);

	FUNC_RETURN;
	return ret;
}

utility_retcode_t send_request(struct rtsp_request *request)
{
	struct rtsp_session *session = request->session;
	int ret;

	FUNC_ENTER;

	INFO("Writing request (length %d) to server \"%s\"\n",
	     request->request_length, session->server->name);

	NOTC("\n----------------------------------------\n"
	     "%s"
	     "----------------------------------------\n", request->buf);

	/* XXX This write call needs a timeout. */
	ret = syscalls_write(session->fd,
			     request->buf,
			     request->request_length);

	if (-1 == ret) {
		ERRR("Failed to write request (length %d) to server \"%s\".\n",
		     request->request_length, session->server->name);
		ret = UTILITY_FAILURE;
	} else {
		ret = UTILITY_SUCCESS;
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t read_response(struct rtsp_response *response)
{
	struct rtsp_session *session = response->session;
	int ret;

	FUNC_ENTER;

	INFO("Reading response from server \"%s\"\n", session->server->name);

	syscalls_memset(response->buf, 0, response->buflen);

	DEBG("before read from fd %d\n", session->fd);

	/* XXX This read call is not correct--it needs to have a
	   timeout and it assumes that all the data is going to come
	   back in one read.  These problems need to be fixed,
	   although it turns out that the AEX seems always to return
	   all its data for each response in one shot, so this code
	   will work for now.

	   Reading the response is trivially more challenging than it
	   appears because the network client code needs to understand
	   a little bit about RTSP to know how much to read.

	   RFC2326, §9.2 Reliability and Acknowledgements

	   Unlike HTTP, an RTSP message MUST contain a Content-Length
	   header whenever that message contains a payload. Otherwise,
	   an RTSP packet is terminated with an empty line immediately
	   following the last message header.

	   The pseudocode is as follows:

	   Read a chunk of data from the socket.  Parse the received
	   data until you get '\n' which indicates the end of a
	   header.

	   If the line you just parsed is a Content-Length header,
	   record its value in a field in the response structure,
	   payload_length or something, else check to see if you just
	   read a blank line indicating the end of headers.  If you
	   read a blank line, you're done reading headers, so check to
	   see if there's a payload_length.  If there's a payload
	   length, try to read that much data from the socket, and
	   you're done.

	   The rest of the headers and payload will be parsed by the
	   caller.

	*/

	ret = syscalls_read(session->fd,
			    response->buf,
			    response->buflen);
	DEBG("after read from fd %d\n", session->fd);

	if (ret > 0) {
		response->buflen = ret;
		NOTC("response length: %d bytes\n"
		     "----------------------------------------\n"
		     "%s"
		     "----------------------------------------\n",
		     response->buflen,
		     response->buf);
		ret = UTILITY_SUCCESS;
	} else {
		ERRR("Failed to read response from server \"%s\"\n",
		     session->server->name);
		ret = UTILITY_FAILURE;
	}

	INFO("Finished reading response from server.\n");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t connect_to_port(char *host, short port, int *fd)
{
	int ret;
	struct sockaddr_in servaddr;

	FUNC_ENTER;

	DEBG("Attempting to connect to %s:%d\n", host, port);

	*fd = syscalls_socket(AF_INET, SOCK_STREAM, 0);
	if (*fd < 0) {
		ERRR("Could not create socket when connecting to %s:%d\n",
		     host, port);
		goto err;
	}

	syscalls_memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = syscalls_htons(port);

	if (syscalls_inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
		ERRR("Could not convert string to IP address when "
		     "connecting to %s:%d\n", host, port);
		goto err;
	}

	if (syscalls_connect(*fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		ERRR("Connection failed when connecting to %s:%d\n",
		     host, port);
		goto err;
	}

	INFO("Connection to %s:%d succeeded\n", host, port);
	ret = UTILITY_SUCCESS;

	goto out;
err:
	ret = UTILITY_FAILURE;
	ERRR("Could not connect to %s port %d\n", host, port);
out:
	FUNC_RETURN;
	return ret;
}
