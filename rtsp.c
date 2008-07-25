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
#include "lt.h"
#include "rtsp.h"
#include "client.h"
#include "config.h"
#include "syscalls.h"
#include "sdp.h"
#include "encryption.h"
#include "encoding.h"

#define DEFAULT_FACILITY LT_RTSP

static utility_retcode_t header_destructor(void *data)
{
	struct key_value_pair *header = (struct key_value_pair *)data;

	FUNC_ENTER;

	DEBG("Destroying header \"%s\"\n", header->name);

	syscalls_free(header->value);
	syscalls_free(header->name);
	syscalls_free(header);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


void set_method(struct rtsp_request *request,
		const char *method)
{
	FUNC_ENTER;

	syscalls_strncpy(request->request_line.method,
			 method,
			 sizeof(request->request_line.method));

	FUNC_RETURN;
	return;
}


utility_retcode_t add_rtsp_header(struct utility_locked_list *list,
				  const char *name,
				  const char *value,
				  const char *description)
{
	utility_retcode_t ret;
	struct key_value_pair *header;

	header = syscalls_malloc(sizeof(*header));
	if (NULL == header) {
		ERRR("Could not allocate memory for RTSP header \"%s\"\n",
		     description);
		goto malloc_failed;
	}

	header->name = syscalls_strndup(name, MAX_HEADER_NAME_LEN);
	if (NULL == header->name) {
		ERRR("Could not allocate memory for RTSP header name \"%s\"\n",
		     name);
		goto strndup_name_failed;
	}

	header->value = syscalls_strndup(value, MAX_HEADER_VALUE_LEN);
	if (NULL == header->value) {
		ERRR("Could not allocate memory for RTSP header value \"%s\"\n",
		     value);
		goto strndup_value_failed;
	}

	if (UTILITY_FAILURE == utility_list_add(list, header, description)) {
		ERRR("Failed to add RTSP header \"%s\""
		     "to request headers list\n",
		     description);
		goto list_add_failed;
	}

	ret = UTILITY_SUCCESS;
	goto out;

list_add_failed:
	syscalls_free(header->value);
strndup_value_failed:
	syscalls_free(header->name);
strndup_name_failed:
	syscalls_free(header);
malloc_failed:
	ret = UTILITY_FAILURE;
out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_cseq_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char sequence_number[10];

	FUNC_ENTER;

	syscalls_snprintf(sequence_number,
			  sizeof(sequence_number),
			  "%lu",
			  request->session->sequence_number);

	ret = add_rtsp_header(&request->headers,
			      "CSeq",
			      sequence_number,
			      "CSeq header");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_content_type_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char type[32];

	FUNC_ENTER;

	get_content_type(type, sizeof(type));

	ret = add_rtsp_header(&request->headers,
			      "Content-Type",
			      type,
			      "Content-Type header");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_content_length_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	size_t len;
	char s[32];

	FUNC_ENTER;

	DEBG("-->%s<--\n", request->msg_body);
	len = syscalls_strlen(request->msg_body);
	DEBG("msg_body length: %d\n", len);

	syscalls_snprintf(s, sizeof(s), "%lu", len);

	ret = add_rtsp_header(&request->headers,
			      "Content-Length",
			      s,
			      "Content-Length header");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_client_instance_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = add_rtsp_header(&request->headers,
			      "Client-Instance",
			      request->session->client->instance,
			      "Client-Instance header");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_apple_challenge_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char encoded_challenge[64];
	size_t encoded_len;

	FUNC_ENTER;

	ret = raopd_base64_encode(encoded_challenge,
				  sizeof(encoded_challenge),
				  request->session->client->challenge,
				  sizeof(request->session->client->challenge),
				  &encoded_len);

	remove_base64_padding(encoded_challenge);

	ret = add_rtsp_header(&request->headers,
			      "Apple-Challenge",
			      encoded_challenge,
			      "Apple-Challenge header");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_user_agent_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = add_rtsp_header(&request->headers,
			      "User-Agent",
			      request->session->client->user_agent,
			      "User-Agent header");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t create_transport_header(struct rtsp_request *request)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = add_rtsp_header(&request->headers,
			      "Transport",
			      "RTP/AVP/TCP;unicast;interleaved=0-1;mode=record",
			      "Transport header");

	FUNC_RETURN;
	return ret;
}


static utility_retcode_t build_request_line(struct rtsp_request *request)
{
	utility_retcode_t ret;
	int written;

	FUNC_ENTER;

	INFO("Building request line for request method \"%s\"\n",
	     request->request_line.method);

	written = syscalls_snprintf(request->bufp,
				    request->bytes_remaining,
				    "%s %s RTSP/%s\r\n",
				    request->request_line.method,
				    request->request_line.uri,
				    request->request_line.version);
	request->bufp += written;
	request->bytes_remaining -= written;

	ret = UTILITY_SUCCESS;
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t build_one_header(void *entry_data, void *data)
{
	struct key_value_pair *header = (struct key_value_pair *)entry_data;
	struct rtsp_request *request = (struct rtsp_request *)data;
	int written;

	DEBG("name: \"%s\" value: \"%s\"\n",
	     header->name, header->value);

	written = syscalls_snprintf(request->bufp,
				    request->bytes_remaining,
				    "%s: %s\r\n",
				    header->name,
				    header->value);

	request->bufp += written;
	request->bytes_remaining -= written;

	return UTILITY_SUCCESS;
}


static utility_retcode_t build_headers(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	INFO("Building headers for request method \"%s\"\n",
	     request->request_line.method);

	utility_list_walk(&request->headers,
			  build_one_header,
			  request,
			  0);

	ret = UTILITY_SUCCESS;
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t add_blank_line(struct rtsp_request *request)
{
	size_t len;
	DEBG("Adding blank line to request\n");

	len = syscalls_snprintf(request->bufp,
				request->bytes_remaining,
				"\r\n");

	request->bufp += len;
	request->bytes_remaining -= len;

	return UTILITY_SUCCESS;
}

utility_retcode_t build_request_string(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	if (NULL == request->buf) {
		request->buf = syscalls_malloc(RTSP_MAX_REQUEST_LEN);
		request->buflen = RTSP_MAX_REQUEST_LEN;

		if (NULL == request->buf) {
			ERRR("Failed to allocate %d bytes for request\n",
			     RTSP_MAX_REQUEST_LEN);
			ret = UTILITY_FAILURE;
			goto out;
		}

	} else {
		syscalls_memset(request->buf, 0, request->buflen);
	}

	request->bufp = request->buf;
	request->bytes_remaining = request->buflen;

	build_request_line(request);
	build_headers(request);
	add_blank_line(request);

	if (NULL != request->msg_body) {
		syscalls_strncpy(request->bufp,
				 request->msg_body,
				 request->bytes_remaining);
	}

	request->request_length = syscalls_strlen(request->buf);

	ret = UTILITY_SUCCESS;
out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t allocate_response_buffers(struct rtsp_response *response)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	if (NULL != response->buf) {
		syscalls_memset(response->buf, 0, response->buflen);
	} else {
		response->buf = syscalls_malloc(RTSP_MAX_RESPONSE_LEN);
		if (NULL == response->buf) {
			ERRR("Failed to allocate %d bytes for response\n",
			     RTSP_MAX_RESPONSE_LEN);
			goto buf_failed;
		}
		response->buflen = RTSP_MAX_RESPONSE_LEN;
	}

	if (response->tmpbuf != NULL) {
		syscalls_memset(response->tmpbuf, 0, response->tmpbuflen);
	} else {
		response->tmpbuf = syscalls_malloc(RTSP_MAX_RESPONSE_LEN);
		if (NULL == response->tmpbuf) {
			ERRR("Failed to allocate %d bytes for response\n",
			     RTSP_MAX_RESPONSE_LEN);
			goto tmpbuf_failed;
		}
		response->tmpbuflen = RTSP_MAX_RESPONSE_LEN;
	}

	if (NULL != response->current_header.name) {
		syscalls_memset(response->current_header.name, 0, MAX_HEADER_NAME_LEN);
	} else {
		response->current_header.name = syscalls_malloc(MAX_HEADER_NAME_LEN);
		if (NULL == response->current_header.name) {
			ERRR("Failed to allocate %d bytes for header name\n",
			     MAX_HEADER_NAME_LEN);
			goto header_name_failed;
		}
	}

	if (NULL != response->current_header.value) {
		syscalls_memset(response->current_header.value, 0, MAX_HEADER_VALUE_LEN);
	} else {
		response->current_header.value = syscalls_malloc(MAX_HEADER_VALUE_LEN);
		if (NULL == response->current_header.value) {
			ERRR("Failed to allocate %d bytes for header value\n",
			     MAX_HEADER_VALUE_LEN);
			goto header_value_failed;
		}
	}

	ret = UTILITY_SUCCESS;
	goto out;

header_value_failed:
	syscalls_free(response->current_header.name);
header_name_failed:
	syscalls_free(response->tmpbuf);
	response->tmpbuflen = 0;
tmpbuf_failed:
	syscalls_free(response->buf);
	response->buflen = 0;
buf_failed:
	ret = UTILITY_FAILURE;
out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t init_rtsp_client(struct rtsp_client *client)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	syscalls_memset(client, 0, sizeof(*client));
	get_client(client);
	ret = get_user_agent(client);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}

	ret = get_random_bytes(client->challenge, sizeof(client->challenge));
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = get_client_instance(client);

out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t destroy_rtsp_client(struct rtsp_client *client)
{
	syscalls_free(client->user_agent);

	return UTILITY_SUCCESS;
}


utility_retcode_t init_rtsp_server(struct rtsp_server *server)
{
	utility_retcode_t ret;

	syscalls_memset(server, 0, sizeof(*server));
	get_server(server);

	ret = get_server_encoded_rsa_public_key(server);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}

/*
	ret = setup_rsa_key(&server->rsa_data);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}
*/

out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t init_rtsp_session(struct rtsp_session *session,
				    struct rtsp_client *client,
				    struct rtsp_server *server)
{
	utility_retcode_t ret;
	uint8_t buf[4];

	FUNC_ENTER;
	syscalls_memset(session, 0, sizeof(*session));

	session->client = client;
	session->server = server;

	ret = get_random_bytes(buf, sizeof(buf));
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	syscalls_snprintf(session->identifier,
			  MAX_SESSION_ID_LEN,
			  "%lu",
			  buf);

	syscalls_snprintf(session->url, MAX_URL_LEN, "rtsp://%s/%s",
			  client->host, session->identifier);

	ret = generate_aes_iv(&session->aes_data);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = generate_aes_key(&session->aes_data);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = set_session_key(&session->aes_data);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t clear_rtsp_request(struct rtsp_request *request)
{
	FUNC_ENTER;

	request->session->sequence_number++;
	
	utility_list_remove_all(&request->headers, header_destructor);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t init_rtsp_request(struct rtsp_request *request,
				    struct rtsp_session *session)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("initializing request structure\n");

	syscalls_memset(request, 0, sizeof(*request));
	request->session = session;

	syscalls_strncpy(request->request_line.uri,
			 "*",
			 sizeof(request->request_line.uri));

	syscalls_strncpy(request->request_line.version,
			 request->session->client->version,
			 sizeof(request->request_line.version));

	ret = UTILITY_SUCCESS;

	DEBG("done initializing request structure\n");

	FUNC_RETURN;
	return ret;
}


utility_retcode_t clear_rtsp_response(struct rtsp_response *response)
{
	utility_retcode_t ret;

	ret = allocate_response_buffers(response);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to allocate response buffer "
		     "for options request.\n");
		ret = UTILITY_FAILURE;
	}

	response->parsep = response->buf;

	return ret;
}


utility_retcode_t init_rtsp_response(struct rtsp_response *response,
				     struct rtsp_session *session)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	syscalls_memset(response, 0, sizeof(*response));
	response->session = session;

	return ret;
}


static utility_retcode_t next_line(struct rtsp_response *response)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char *nextline;

	FUNC_ENTER;

	nextline = begin_token((const char *)response->parsep, "\r\n");
	if (NULL == nextline) {

		ERRR("Could not find beginning of next line in response\n");
		ret = UTILITY_FAILURE;

	} else {

		response->parsep = nextline;

	}

	FUNC_RETURN;
	return ret;
}

static utility_retcode_t get_status_code(struct rtsp_response *response)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char *begincode;
	char code[4];

	FUNC_ENTER;

	begincode = begin_token((const char *)response->parsep, " ");
	if (NULL == begincode) {
		ERRR("Could not find beginning of status "
		     "code in status line\n");
		goto failed;
	}

	ret = utility_copy_token(code, sizeof(code), begincode, " ", NULL);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not find status code in response status line\n");
		goto failed;
	}

	DEBG("status code string is \"%s\"\n", code);

	response->status_line.status_code = syscalls_strtoul(code, NULL, 0);
	INFO("response status code is: %d\n",
	     response->status_line.status_code);

	ret = UTILITY_SUCCESS;
	goto out;

failed:
	ret = UTILITY_FAILURE;
out:
	FUNC_RETURN;
	return ret;
}

static utility_retcode_t parse_status_line(struct rtsp_response *response)
{
	utility_retcode_t ret;
	char status_line[64];

	FUNC_ENTER;

	syscalls_memset(status_line, 0, sizeof(status_line));

	ret = utility_copy_token(status_line,
				 sizeof(status_line),
				 response->parsep,
				 "\r\n",
				 NULL);

	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to locate end of line in status line\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	INFO("Status line: \"%s\"\n", status_line);

	/* RTSP/1.0 200 OK We don't actually care about the version or
	 * the reason string */

	ret = get_status_code(response);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = next_line(response);

out:
	FUNC_RETURN;
	return ret;
}


static utility_boolean_t more_headers(struct rtsp_response *response)
{
	utility_boolean_t ret = UTILITY_TRUE;

	FUNC_ENTER;

	if (UTILITY_SUCCESS == next_line(response)) {

		DEBG("Found a header line\n");

		if (0 == syscalls_strncmp(response->parsep,
					  "\r\n",
					  sizeof("\r\n"))) {
			ret = UTILITY_FALSE;
		}
	}

	FUNC_RETURN;
	return ret;
}


static utility_retcode_t parse_content_length(struct rtsp_response *response)
{
	char *value = response->current_header.value;

	FUNC_ENTER;

	response->content_length = syscalls_strtoul(value, NULL, 0);
	INFO("Set content length to %d\n", response->content_length);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


static utility_retcode_t parse_apple_response(struct rtsp_response *response)
{
	FUNC_ENTER;

	DEBG("value: \"%s\"\n", response->current_header.value);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}

static utility_retcode_t parse_public(struct rtsp_response *response)
{
	FUNC_ENTER;

	DEBG("value: \"%s\"\n", response->current_header.value);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


static utility_retcode_t parse_audio_jack_status(struct rtsp_response *response)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char *value = response->current_header.value;

	FUNC_ENTER;

	DEBG("value: \"%s\"\n", value);

	if (0 != syscalls_strncmp(value, "connected", sizeof("connected") - 1)) {

		response->session->server->audio_jack_status =
			AUDIO_JACK_DISCONNECTED;

		ERRR("Server reports audio jack is not connected\n");
		ret = UTILITY_FAILURE;
	}

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


static utility_retcode_t parse_session(struct rtsp_response *response)
{
	char *value = response->current_header.value;

	FUNC_ENTER;

	DEBG("value: \"%s\"\n", value);

	syscalls_strncpy(response->session->identifier,
			 value,
			 sizeof(response->session->identifier));

	INFO("Session identifier: \"%s\"\n", response->session->identifier);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


static utility_retcode_t parse_transport(struct rtsp_response *response)
{
	char *value = response->current_header.value;
	char *port_string;

	FUNC_ENTER;

	DEBG("value: \"%s\"\n", value);

	port_string = begin_token(value, "server_port=");

	response->session->port = syscalls_strtoul(port_string, NULL, 0);

	INFO("Session IP port: %d\n", response->session->port);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


static utility_retcode_t parse_unknown_header(struct rtsp_response *response)
{
	FUNC_ENTER;

	WARN("Found unknown header \"%s\" with value \"%s\"\n",
	     response->current_header.name,
	     response->current_header.value);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


static utility_retcode_t parse_one_header(struct rtsp_response *response)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char *start_value;
	char *header_name = response->current_header.name;
	char *header_value = response->current_header.value;

	FUNC_ENTER;

	ret = utility_copy_token(header_name,
				 MAX_HEADER_NAME_LEN,
				 response->parsep,
				 ": ",
				 NULL);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to copy header name\n");
		goto out;
	}

	start_value = begin_token(response->parsep, ": ");
	ret = utility_copy_token(header_value,
				 MAX_HEADER_VALUE_LEN,
				 start_value,
				 "\r\n",
				 NULL);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to copy header value\n");
		goto out;
	}

	DEBG("Found header name: \"%s\" value: \"%s\"\n",
	     header_name, header_value);

	if (0 == syscalls_strcmp(header_name, "Content-Length")) {
		ret = parse_content_length(response);
	} else if (0 == syscalls_strcmp(header_name, "Apple-Response")) {
		ret = parse_apple_response(response);
	} else if (0 == syscalls_strcmp(header_name, "Audio-Jack-Status")) {
		ret = parse_audio_jack_status(response);
	} else if (0 == syscalls_strcmp(header_name, "Session")) {
		ret = parse_session(response);
	} else if (0 == syscalls_strcmp(header_name, "Transport")) {
		ret = parse_transport(response);
	} else if (0 == syscalls_strcmp(header_name, "Public")) {
		ret = parse_public(response);
	} else {
		parse_unknown_header(response);
	}

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t get_header_list(struct rtsp_response *response)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	while (more_headers(response)) {
		ret = utility_copy_token(response->tmpbuf,
					 response->tmpbuflen,
					 response->parsep,
					 "\r\n",
					 NULL);

		if (UTILITY_SUCCESS != ret) {
			ERRR("Header line is not terminated by \\r\\n\n");
			goto failed;
		}

		DEBG("header line: \"%s\"\n", response->tmpbuf);

		ret = parse_one_header(response);

		if (UTILITY_SUCCESS != ret) {
			ERRR("Failed to parse header");
			goto failed;
		}

	}

	INFO("Got blank line (\\r\\n).  Finished parsing headers\n");

failed:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t rtsp_parse_response(struct rtsp_response *response)
{
	utility_retcode_t ret;
	FUNC_ENTER;

	ret = parse_status_line(response);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to parse status line\n");
		goto out;
	}

	ret = get_header_list(response);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to parse headers\n");
		goto out;
	}

	/* Here we will need to parse the message body, if any. */

out:
	FUNC_RETURN;
	return ret;
}
