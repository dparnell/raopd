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
#ifndef RTSP_H
#define RTSP_H

#include "utility.h"
#include "encryption.h"

#define RTSP_MAX_REQUEST_LEN	4096
#define RTSP_MAX_RESPONSE_LEN	4096
#define MAX_METHOD_LEN		32
#define MAX_URI_LEN		64
#define MAX_VERSION_LEN		4
#define MAX_SESSION_ID_LEN	16
#define MAX_URL_LEN		64
#define MAX_HEADER_NAME_LEN	4096 /* Total overkill */
#define MAX_HEADER_VALUE_LEN	4096
#define MAX_INSTANCE_LEN	32

struct rtsp_client {
	char name[MAX_NAME_LEN];
	char host[MAX_HOST_NAME_LEN];
	char version[MAX_VERSION_LEN];
	char *user_agent;
	uint8_t challenge[16];
	char instance[MAX_INSTANCE_LEN];
};

#define AUDIO_JACK_DISCONNECTED 0
#define AUDIO_JACK_CONNECTED 1

struct rtsp_server {
	char name[MAX_NAME_LEN];
	char host[MAX_HOST_NAME_LEN];
	short port;
	struct rsa_data rsa_data;
	int audio_jack_status;
};

/* 
 * The following structures represent concepts defined in RFC2326 Real
 * Time Streaming Protocol (RTSP)
 */

/* RFC2326 §6 Request */

struct rtsp_request_line {
	char method[MAX_METHOD_LEN];
	char uri[MAX_URI_LEN];
	char version[MAX_VERSION_LEN];
};

/* XXX The message related fields in request and response should be
 * refactored out into a common struct as they are exactly the same.
 * --DPA */
struct rtsp_request {
	struct rtsp_session *session;
	char *buf;
	size_t buflen;
	char *bufp;
	size_t bytes_remaining;
	size_t request_length;
	struct rtsp_request_line request_line;
	struct utility_locked_list headers;
	/* this is gross--refactor to make a struct for msg body and
	 * header buffer--they're exactly the same logic. */
	char *msg_body;
	size_t msg_body_len;
	char *msg_bodyp;
	size_t msg_body_bytes_remaining;
	struct utility_locked_list sdp_fields;
};

/* RFC2326 §7 Response */

struct rtsp_status_line {
	char *version;
	int status_code;
	char *reason;
};

struct rtsp_response {
	struct rtsp_session *session;
	char *buf;
	size_t buflen;
	char *tmpbuf;
	size_t tmpbuflen;
	struct rtsp_status_line status_line;
	struct utility_locked_list headers;
	char *msg_body;
	char *parsep; /* used to track position when parsing */
	struct key_value_pair current_header;
	size_t content_length;
};

struct rtsp_session {
	struct rtsp_client *client;
	struct rtsp_server *server;
	int fd;
	char identifier[MAX_SESSION_ID_LEN]; /* Note: RFC2326 §3.4
					      * specifies arbitrary
					      * length with 8
					      * characters minimum.
					      * The Apple Airport
					      * Express uses 8. */
	char url[MAX_URL_LEN];
	short port;
	unsigned int sequence_number;
	struct aes_data aes_data;
};

utility_retcode_t add_rtsp_header(struct utility_locked_list *list,
				  const char *name,
				  const char *value,
				  const char *description);

void set_method(struct rtsp_request *request,
		const char *method);
utility_retcode_t create_cseq_header(struct rtsp_request *request);
utility_retcode_t create_content_type_header(struct rtsp_request *request);
utility_retcode_t create_content_length_header(struct rtsp_request *request);
utility_retcode_t create_client_instance_header(struct rtsp_request *request);
utility_retcode_t create_user_agent_header(struct rtsp_request *request);
utility_retcode_t create_transport_header(struct rtsp_request *request);
utility_retcode_t create_apple_challenge_header(struct rtsp_request *request);
utility_retcode_t build_request_string(struct rtsp_request *request);
utility_retcode_t allocate_response_buffer(struct rtsp_response *response);

utility_retcode_t init_rtsp_client(struct rtsp_client *client);
utility_retcode_t destroy_rtsp_client(struct rtsp_client *client);
utility_retcode_t init_rtsp_server(struct rtsp_server *server);
utility_retcode_t init_rtsp_session(struct rtsp_session *session,
				    struct rtsp_client *client,
				    struct rtsp_server *server);
utility_retcode_t clear_rtsp_request(struct rtsp_request *request);
utility_retcode_t init_rtsp_request(struct rtsp_request *request,
				    struct rtsp_session *session);
utility_retcode_t clear_rtsp_response(struct rtsp_response *response);
utility_retcode_t init_rtsp_response(struct rtsp_response *response,
				     struct rtsp_session *session);
utility_retcode_t rtsp_parse_response(struct rtsp_response *response);

#endif /* #ifndef RTSP_H */
