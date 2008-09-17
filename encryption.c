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

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/bn.h>

#include "lt.h"
#include "utility.h"
#include "syscalls.h"
#include "rtsp.h"
#include "encryption.h"
#include "encoding.h"

#define DEFAULT_FACILITY LT_ENCRYPTION


utility_retcode_t generate_aes_iv(struct aes_data *aes_data)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Generating a %d byte initialization vector\n", sizeof(aes_data->iv));

	ret = get_random_bytes((uint8_t *)&aes_data->iv, sizeof(aes_data->iv));

	FUNC_RETURN;
	return ret;
}


utility_retcode_t generate_aes_key(struct aes_data *aes_data)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Generating a %d byte AES key\n", sizeof(aes_data->key));

	ret = get_random_bytes((uint8_t *)&aes_data->key, sizeof(aes_data->key));

	FUNC_RETURN;
	return ret;
}


int raopd_rsa_encrypt_openssl(uint8_t *text, int len, uint8_t *res)
{
	RSA *rsa;
	uint8_t modulo[300];
	uint8_t exponent[24];
	size_t size;

        char n[] = 
            "59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUtwC"
            "5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDR"
            "KSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuB"
            "OitnZ/bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJ"
            "Q+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnh"
            "imNVvYFZeCXg/IdTQ+x4IRdiXNv5hEew==";

        char e[] = "AQAB";

	rsa = RSA_new();
	raopd_base64_decode(modulo, sizeof(modulo), n, syscalls_strlen(n), &size);
	rsa->n = BN_bin2bn(modulo, size, NULL);
	raopd_base64_decode(exponent, sizeof(exponent), e, syscalls_strlen(e), &size);
	rsa->e = BN_bin2bn(exponent, size, NULL);
	size = RSA_public_encrypt(len, text, res, rsa, RSA_PKCS1_OAEP_PADDING);
	RSA_free(rsa);

	DEBG("RSA encrypted result (%d bytes)\n", size);

	return size;
}

