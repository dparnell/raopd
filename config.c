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
#include "config.h"
#include "lt.h"
#include "syscalls.h"
#include "utility.h"
#include "rtsp.h"

#define DEFAULT_FACILITY LT_CONFIG

utility_retcode_t get_client(struct rtsp_client *client)
{
	FUNC_ENTER;

	syscalls_strncpy(client->name, "localhost test client", sizeof(client->name));
	syscalls_strncpy(client->host, CLIENT_HOST, sizeof(client->host));
	syscalls_strncpy(client->version, "1.0", sizeof(client->version));

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_user_agent(struct rtsp_client *client)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	client->user_agent =
		syscalls_strdup("iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)");

	if (NULL == client->user_agent) {
		ret = UTILITY_FAILURE;
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t get_client_instance(struct rtsp_client *client)
{
	utility_retcode_t ret;
	uint8_t buf[32];

	FUNC_ENTER;

	ret = get_random_bytes(buf, sizeof(buf));
	if (UTILITY_SUCCESS == ret) {
		syscalls_snprintf(client->instance,
				  MAX_INSTANCE_LEN,
				  "%08x%08x",
				  buf, buf + 4);
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t get_server(struct rtsp_server *server)
{
	FUNC_ENTER;

	syscalls_strncpy(server->name, "network2", sizeof(server->name));
	syscalls_strncpy(server->host, SERVER_HOST, sizeof(server->host));
	server->port = SERVER_PORT;

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_server_encoded_rsa_public_key(struct rtsp_server *server)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

        char modulo[] =
            "59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUtwC"
            "5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDR"
            "KSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuB"
            "OitnZ/bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJ"
            "Q+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnh"
            "imNVvYFZeCXg/IdTQ+x4IRdiXNv5hEew==";
        char exponent[] = "AQAB";

	FUNC_ENTER;

	server->rsa_data.encoded_key.modulo = syscalls_strdup(modulo);
	if (NULL == server->rsa_data.encoded_key.modulo) {
		ret = UTILITY_FAILURE;
		goto out;
	}

	INFO("RSA public key modulo: \"%s\"\n",
	     server->rsa_data.encoded_key.modulo);

	server->rsa_data.encoded_key.exponent = syscalls_strdup(exponent);
	if (NULL == server->rsa_data.encoded_key.exponent) {
		ret = UTILITY_FAILURE;
		goto out;
	}

	INFO("RSA public key exponent: \"%s\"\n",
	     server->rsa_data.encoded_key.exponent);

	INFO("Obtained base64 encoded RSA modulo and exponent successfully\n");
out:
	FUNC_RETURN;
	return ret;
}
