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
#include "encoding.h"
#include "encryption.h"

#define DEFAULT_FACILITY LT_SDP

static utility_retcode_t add_sdp_field(struct utility_locked_list *list,
				       const char *name,
				       const char *value,
				       const char *description)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	/* Not strictly correct, but it will work for now.  If it's
	 * necessary, we can refactor to take any account of the
	 * differences between RTSP and SDP key/value usage. */

	ret = add_rtsp_header(list, name, value, description);

	FUNC_RETURN;
	return ret;
}

static utility_retcode_t add_aes_attributes(struct rtsp_request *request,
					    char *value)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char *base64_outbuf;
	uint8_t *aes_key;
	uint8_t encrypted_key[256];
	size_t encrypted_key_len;
	size_t encoded_len;

	FUNC_ENTER;

	aes_key = (uint8_t *)&request->session->aes_data.key;
	/*
	ret = raopd_rsa_encrypt(&request->session->server->rsa_data.key,
				(const uint8_t *)aes_key,
				RAOP_AES_KEY_LEN,
				&encrypted_key,
				&encrypted_key_len);
	*/

	encrypted_key_len = raopd_rsa_encrypt_openssl((uint8_t *)aes_key,
						      RAOP_AES_KEY_LEN,
						      encrypted_key);

	if (UTILITY_FAILURE == ret) {
		goto out;
	}

	base64_outbuf = syscalls_malloc(MAX_HEADER_VALUE_LEN);
	if (NULL == base64_outbuf) {
		ret = UTILITY_FAILURE;
		goto out;
	}

	raopd_base64_encode(base64_outbuf,
			    MAX_HEADER_VALUE_LEN,
			    encrypted_key,
			    encrypted_key_len,
			    &encoded_len);

	remove_base64_padding(base64_outbuf);

	snprintf(value, MAX_HEADER_VALUE_LEN, "rsaaeskey:%s", base64_outbuf);

	ret = add_sdp_field(&request->sdp_fields,
			    "a",
			    value,
			    "attributes rsaaeskey");

	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"attributes aesiv\"\n");
		goto free_base64_outbuf;
	}

	raopd_base64_encode(base64_outbuf,
			    MAX_HEADER_VALUE_LEN,
			    request->session->aes_data.iv,
			    sizeof(request->session->aes_data.iv),
			    &encoded_len);

	remove_base64_padding(base64_outbuf);

	snprintf(value, MAX_HEADER_VALUE_LEN, "aesiv:%s", base64_outbuf);

	ret = add_sdp_field(&request->sdp_fields, "a", value, "attributes aesiv");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"attributes aesiv\"\n");
		goto free_base64_outbuf;
	}

free_base64_outbuf:
	syscalls_free(base64_outbuf);
out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t add_announce_sdp_fields(struct rtsp_request *request)
{
	utility_retcode_t ret;
	char *value;

	FUNC_ENTER;

	/* XXX We should be using a permanent temp buffer in the
	 * request struct here. --DPA */
	value = syscalls_malloc(MAX_HEADER_VALUE_LEN);
	if (NULL == value) {
		ret = UTILITY_FAILURE;
		goto malloc_value_failed;
	}

 	ret = add_sdp_field(&request->sdp_fields, "v", "0", "version");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"version\"\n");
		goto out;
	}

	snprintf(value, MAX_HEADER_VALUE_LEN, "iTunes %s 0 IN IP4 %s",
		 request->session->identifier, request->session->client->host);

 	ret = add_sdp_field(&request->sdp_fields, "o", value, "origin");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"origin\"\n");
		goto out;
	}

 	ret = add_sdp_field(&request->sdp_fields, "s", "iTunes", "session name");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"session name\"\n");
		goto out;
	}

	snprintf(value, MAX_HEADER_VALUE_LEN, "IN IP4 %s",
		 request->session->server->host);

 	ret = add_sdp_field(&request->sdp_fields, "c", value, "connection data");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"connection data\"\n");
		goto out;
	}

 	ret = add_sdp_field(&request->sdp_fields, "t", "0 0", "timing");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"timing\"\n");
		goto out;
	}

 	ret = add_sdp_field(&request->sdp_fields,
			    "m",
			    "audio 0 RTP/AVP 96",
			    "media descriptions");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"media descriptions\"\n");
		goto out;
	}

 	ret = add_sdp_field(&request->sdp_fields,
			    "a",
			    "rtpmap:96 AppleLossless",
			    "attributes rtpmap");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"attributes rtpmap\"\n");
		goto out;
	}

 	ret = add_sdp_field(&request->sdp_fields,
			    "a",
			    "fmtp:96 4096 0 16 40 10 14 2 255 0 0 44100",
			    "attributes fmtp");
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add SDP field \"attributes fmtp\"\n");
		goto out;
	}

	ret = add_aes_attributes(request, value);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Could not add AES attributes to SDP message\n");
		goto out;
	}

out:
	syscalls_free(value);
malloc_value_failed:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t build_one_sdp_field(void *entry_data, void *data)
{
	struct key_value_pair *field = (struct key_value_pair *)entry_data;
	struct rtsp_request *request = (struct rtsp_request *)data;
	int written;

	DEBG("name: \"%s\" value: \"%s\"\n",
	     field->name, field->value);

	written = syscalls_snprintf(request->msg_bodyp,
				    request->msg_body_bytes_remaining,
				    "%s=%s\r\n",
				    field->name,
				    field->value);

	DEBG("written: %d\n", written);

	request->msg_bodyp += written;
	request->msg_body_bytes_remaining -= written;

	return UTILITY_SUCCESS;
}

static utility_retcode_t build_sdp_fields(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	INFO("Building SDP headers for request method \"%s\"\n",
	     request->request_line.method);

	utility_list_walk(&request->sdp_fields,
			  build_one_sdp_field,
			  request,
			  0);

	ret = UTILITY_SUCCESS;

	FUNC_RETURN;
	return ret;
}


utility_retcode_t add_msg_body_blank_line(struct rtsp_request *request)
{
	size_t len;
	DEBG("Adding blank line to request\n");

	len = syscalls_snprintf(request->msg_bodyp,
				request->msg_body_bytes_remaining,
				"\r\n");

	request->msg_bodyp += len;
	request->msg_body_bytes_remaining -= len;

	return UTILITY_SUCCESS;
}


utility_retcode_t build_msg_body(struct rtsp_request *request)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	if (NULL == request->msg_body) {
		request->msg_body = syscalls_malloc(RTSP_MAX_REQUEST_LEN);
		request->msg_body_len = RTSP_MAX_REQUEST_LEN;

		if (NULL == request->msg_body) {
			ERRR("Failed to allocate %d bytes for msg body\n",
			     RTSP_MAX_REQUEST_LEN);
			ret = UTILITY_FAILURE;
			goto out;
		}

	} else {
		syscalls_memset(request->msg_body, 0, request->msg_body_len);
	}

	request->msg_bodyp = request->msg_body;
	request->msg_body_bytes_remaining = request->msg_body_len;

	build_sdp_fields(request);

	/* add_msg_body_blank_line(request); */

	ret = UTILITY_SUCCESS;

out:
	FUNC_RETURN;
	return ret;
}
