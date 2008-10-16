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
#include "syscalls.h"
#include "config.h"
#include "utility.h"
#include "lt.h"
#include "client.h"
#include "rtsp.h"
#include "rtsp_client.h"
#include "audio_stream.h"
#include "sdp.h"

#define DEFAULT_FACILITY LT_RTSP_CLIENT

static utility_retcode_t begin_request(struct rtsp_request *request,
				       const char *method)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Starting to send \"%s\" request\n", method);

	clear_rtsp_request(request);
	set_method(request, method);

	ret = create_cseq_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create CSeq header\n");
	}

	FUNC_RETURN;
	return ret;
}


static utility_retcode_t finish_request(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	ret = build_request_string(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to build request string for \"%s\" request.\n",
		     request->request_line.method);
		goto out;
	}

	ret = send_request(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to send \"%s\" request.\n",
		     request->request_line.method);
		goto out;
	}

	DEBG("Sent \"%s\" request successfully\n",
	     request->request_line.method);

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t send_announce_request(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	ret = begin_request(request, "ANNOUNCE");
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	syscalls_strncpy(request->request_line.uri,
			 request->session->url,
			 sizeof(request->request_line.uri));

	ret = create_content_type_header(request, "application/sdp");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Content-Type header\n");
		goto out;
	}

	/* We need to add the SDP fields before we add the rest of the
	 * RTSP headers because we need to know the content length.
	 * It may not matter, but it seems like common practice to put
	 * the content length header early in the header list. */
	add_announce_sdp_fields(request);

	build_msg_body(request);

	ret = create_content_length_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Content-Length header\n");
		goto out;
	}

	ret = create_user_agent_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create User-Agent header\n");
		goto out;
	}

	ret = create_client_instance_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Client-Instance header\n");
		goto out;
	}

	ret = create_apple_challenge_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Apple-Challenge header\n");
		goto out;
	}

	ret = finish_request(request);

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t send_setup_request(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	ret = begin_request(request, "SETUP");
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = create_transport_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Transport header\n");
		goto out;
	}

	ret = create_user_agent_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create User-Agent header\n");
		goto out;
	}

	ret = create_client_instance_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Client-Instance header\n");
		goto out;
	}

	ret = finish_request(request);

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t send_record_request(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	ret = begin_request(request, "RECORD");
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = create_session_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Session header\n");
		goto out;
	}

	ret = create_range_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Range header\n");
		goto out;
	}

	ret = create_rtp_info_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create RTP-Info header\n");
		goto out;
	}

	ret = create_user_agent_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create User-Agent header\n");
		goto out;
	}

	ret = create_client_instance_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Client-Instance header\n");
		goto out;
	}

	ret = finish_request(request);

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t send_volume_request(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	ret = begin_request(request, "SET_PARAMETER");
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = create_session_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Session header\n");
		goto out;
	}

	ret = create_content_type_header(request, "text/parameters");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Content-Type header\n");
		goto out;
	}

	/* The volume message body is not any protocol--it's not SDP,
	   it's in header format, but it's in the message body, so
	   just treat it as a one off and format the string here. */
	get_msg_body(request);
	request->msg_body_bytes_remaining -=
		snprintf(request->msg_body,
			 request->msg_body_bytes_remaining,
			 "volume: 0.000000\r\n");

	ret = create_content_length_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Content-Length header\n");
		goto out;
	}

	ret = create_user_agent_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create User-Agent header\n");
		goto out;
	}

	ret = create_client_instance_header(request);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not create Client-Instance header\n");
		goto out;
	}

	ret = finish_request(request);

out:
	FUNC_RETURN;
	return ret;
}



/*
utility_retcode_t send_options_request(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Starting to send OPTIONS request\n");

	ret = begin_request(request, "OPTIONS");
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = finish_request(request);

out:
	FUNC_RETURN;
	return ret;
}
*/

static utility_retcode_t read_rtsp_response(struct rtsp_response *response)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	clear_rtsp_response(response);

	ret = read_response(response);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to read response from server \"%s\"\n",
			response->session->server->name);
	} else {
		ret = rtsp_parse_response(response);
	}

	FUNC_RETURN;
	return ret;
}

utility_retcode_t rtsp_start_client(struct rtsp_session *session)
{
	utility_retcode_t ret;
	struct rtsp_client *client;
	struct rtsp_server *server;
	struct rtsp_request *request;
	struct rtsp_response *response;

	FUNC_ENTER;

	INFO("Starting RTSP client\n");

	client = syscalls_malloc(sizeof(*client));
	server = syscalls_malloc(sizeof(*server));
	request = syscalls_malloc(sizeof(*request));
	response = syscalls_malloc(sizeof(*response));

	init_rtsp_client(client);

	ret = init_rtsp_server(server);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to initialize server\n");
		goto out;
	}

	init_rtsp_session(session, client, server);
	init_rtsp_request(request, session);
	init_rtsp_response(response, session);

	INFO("Connecting to server \"%s\"\n", server->name);

	ret = connect_server(session);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to connect to server \"%s\"\n", server->name);
		goto out;
	}

	/*
	send_options_request(request);
	read_rtsp_response(response);
	*/

	ret = send_announce_request(request);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to send ANNOUNCE request to server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = read_rtsp_response(response);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to read ANNOUNCE response from server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = send_setup_request(request);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to send SETUP request to server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = read_rtsp_response(response);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to read SETUP response from server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = send_record_request(request);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to send RECORD request to server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = read_rtsp_response(response);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to read RECORD response from server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = send_volume_request(request);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to send SET_PARAMETERS (volume) request to server \"%s\"\n",
		     server->name);
		goto out;
	}

	ret = read_rtsp_response(response);
	if (UTILITY_FAILURE == ret) {
		ERRR("Failed to read SET_PARAMETERS response from server \"%s\"\n",
		     server->name);
		goto out;
	}

out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t rtsp_send_data(struct rtsp_session *session)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	DEBG("Connecting to host \"%s\" port %d\n",
	     session->server->host, session->port);

	ret = connect_to_port(session->server->host,
			      session->port,
			      &session->audio_stream.session_fd);

	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	if (UTILITY_SUCCESS != init_audio_stream(&session->audio_stream)) {
		ERRR("Failed to initialized audio stream\n");
		goto out;
	}

	lt_set_level(LT_AUDIO_STREAM, LT_DEBUG);
	send_audio_stream(&session->audio_stream,
			  &session->aes_data);

out:
	FUNC_RETURN;
	return ret;
}

