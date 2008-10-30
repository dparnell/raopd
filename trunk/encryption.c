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

#include <arpa/inet.h>

#include "lt.h"
#include "utility.h"
#include "syscalls.h"
#include "rtsp.h"
#include "encryption.h"
#include "encoding.h"
#include "audio_stream.h"

#define DEFAULT_FACILITY LT_ENCRYPTION


utility_retcode_t generate_aes_iv(struct aes_data *aes_data)
{
	utility_retcode_t ret;
	uint8_t iv[] = { 0x1, 0x2, 0x3, 0x4, 
			 0x1, 0x2, 0x3, 0x4, 
			 0x1, 0x2, 0x3, 0x4, 
			 0x1, 0x2, 0x3, 0x4 };


	FUNC_ENTER;

	DEBG("Generating a %d byte initialization vector\n", sizeof(aes_data->iv));

	ret = get_random_bytes((uint8_t *)&aes_data->iv, sizeof(aes_data->iv));

	memcpy(&aes_data->iv, iv, sizeof(iv));
	ret = UTILITY_SUCCESS;

	FUNC_RETURN;
	return ret;
}


utility_retcode_t generate_aes_key(struct aes_data *aes_data)
{
	utility_retcode_t ret;
	uint8_t key[] = { 0x5, 0x6, 0x7, 0x8, 
			  0x5, 0x6, 0x7, 0x8, 
			  0x5, 0x6, 0x7, 0x8, 
			  0x5, 0x6, 0x7, 0x8 };

	FUNC_ENTER;

	DEBG("Generating a %d byte AES key\n", sizeof(aes_data->key));

	ret = get_random_bytes((uint8_t *)&aes_data->key, sizeof(aes_data->key));

	memcpy(&aes_data->key, key, sizeof(key));
	ret = UTILITY_SUCCESS;

	FUNC_RETURN;
	return ret;
}


utility_retcode_t generate_aes_data(struct aes_data *aes_data)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Generating AES data\n");

	ret = generate_aes_key(aes_data);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	ret = generate_aes_iv(aes_data);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

out:
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


/* XXX error checking? */
utility_retcode_t initialize_aes(struct aes_data *aes_data)
{
	FUNC_ENTER;

	INFO("Initializing AES module\n");

	EVP_CIPHER_CTX_init(&aes_data->ctx);

	EVP_CipherInit_ex(&aes_data->ctx,
			  EVP_aes_128_cbc(),
			  NULL,
			  aes_data->key,
			  aes_data->iv,
			  1 /* encrypt */);

	FUNC_RETURN;
	return UTILITY_SUCCESS;
}

/* XXX error checking? */
utility_retcode_t shutdown_aes(struct aes_data *aes_data,
			       uint8_t *encrypted_buf,
			       size_t *encrypted_len)
{
	EVP_CipherFinal_ex(&aes_data->ctx, encrypted_buf, (int *)encrypted_len); 
	EVP_CIPHER_CTX_cleanup(&aes_data->ctx);

	return UTILITY_SUCCESS;
}


/* XXX error checking? */
utility_retcode_t aes_encrypt_data(struct aes_data *aes_data,
				   uint8_t *encrypted_buf,
				   size_t *encrypted_len,
				   uint8_t *cleartext_buf,
				   size_t cleartext_len)
{
	EVP_CipherUpdate(&aes_data->ctx,
			 encrypted_buf,
			 (int *)encrypted_len,
			 cleartext_buf,
			 cleartext_len);

	return UTILITY_SUCCESS;
}
