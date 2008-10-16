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

utility_retcode_t get_pcm_data_file(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s, "./faactest/sndtest_raw", size);
	//syscalls_strncpy(s, "datapattern", size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_client_name(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s, "localhost test client", size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_client_host(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s, CLIENT_HOST, size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_client_version(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s, "1.0", size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_user_agent(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s,
			 "iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)",
			 size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_client_instance(char *s, size_t size)
{
	utility_retcode_t ret;
	uint8_t buf[32];

	FUNC_ENTER;

	ret = get_random_bytes(buf, sizeof(buf));
	if (UTILITY_SUCCESS == ret) {
		syscalls_snprintf(s,
				  size,
				  "%08x%08x",
				  buf, buf + 4);
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t get_server_name(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s, "network2", size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_server_host(char *s, size_t size)
{
	FUNC_ENTER;

	syscalls_strncpy(s, SERVER_HOST, size);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_server_port(short *i)
{
	FUNC_ENTER;

	*i = SERVER_PORT;

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_server_encoded_rsa_public_modulo(char *s, size_t size)
{
        char modulo[] =
            "59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUtwC"
            "5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDR"
            "KSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuB"
            "OitnZ/bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJ"
            "Q+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnh"
            "imNVvYFZeCXg/IdTQ+x4IRdiXNv5hEew==";

	FUNC_ENTER;

	syscalls_strncpy(s, modulo, size);

	INFO("RSA public key modulo: \"%s\"\n", s);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}


utility_retcode_t get_server_encoded_rsa_public_exponent(char *s, size_t size)
{
        char exponent[] = "AQAB";

	FUNC_ENTER;

	syscalls_strncpy(s, exponent, size);

	INFO("RSA public key exponent: \"%s\"\n", s);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}
