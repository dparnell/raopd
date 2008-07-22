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
#include <nettle/aes.h>
#include <nettle/yarrow.h>

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

utility_retcode_t set_session_key(struct aes_data *aes_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	aes_set_encrypt_key(&aes_data->aes.ctx, RAOP_AES_KEY_LEN, aes_data->key);
	CBC_SET_IV(&aes_data->aes, &aes_data->iv);

	FUNC_RETURN;
	return ret;
}


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


void raop_rsa_public_key_init(struct rsa_public_key *key)
{
	rsa_public_key_init(key);
	return;
}


void raop_rsa_public_key_clear(struct rsa_public_key *key)
{
	raop_rsa_public_key_clear(key);
	return;
}


utility_retcode_t raop_rsa_import(char *encoded_data,
				  mpz_t rop,
				  size_t *decoded_len)
{
	utility_retcode_t ret;
	uint8_t *decoded_data;
	size_t decoded_data_buflen = 4096;
	size_t datasize;

	decoded_data = syscalls_malloc(decoded_data_buflen);
	if (NULL == decoded_data) {
		ret = UTILITY_FAILURE;
		goto out;
	}

	ret = raopd_base64_decode(decoded_data,
				  decoded_data_buflen,
				  encoded_data,
				  syscalls_strlen(encoded_data),
				  decoded_len);

	if (UTILITY_FAILURE == ret) {
		goto out;
	}

	mpz_import(rop,
		   *decoded_len,
		   1,
		   sizeof(uint8_t),
		   0,
		   0,
		   decoded_data);

	datasize = mpz_sizeinbase(rop, 2);

	DEBG("Imported RSA data of %d bytes (%d bits)\n",
	     (int)*decoded_len, (int)datasize);

out:
	syscalls_free(decoded_data);
	FUNC_RETURN;
	return ret;
}


utility_retcode_t raop_rsa_export(char *buf,
				  size_t buflen,
				  mpz_t op)
{
	utility_retcode_t ret;
	uint8_t *tmpbuf;
	size_t datalen;
	size_t encoded_len;

	DEBG("Exporting RSA data\n");

	/* XXX If mpz_export can't allocate the tempbuf, it terminates
	 * the program.  That's not pretty, but it's not clear to me
	 * whether it checks to see if the buffer is large enough if a
	 * buffer is supplied.  I think the right way to do this is to
	 * supply a custom allocation function to GMP that has a large
	 * buffer that it can hand out repeatedly, although there are
	 * concurrency issues there that would need to be addressed
	 * carefully. */
	tmpbuf = mpz_export(NULL, &datalen, 1, sizeof(uint8_t), 0, 0, op);

	DEBG("mpz_export wrote %d words of size %d bytes\n", datalen, sizeof(uint8_t));

	ret = raopd_base64_encode(buf,
				  buflen,
				  tmpbuf,
				  datalen,
				  &encoded_len);

	syscalls_free(tmpbuf);

	FUNC_RETURN;
	return ret;
}


utility_retcode_t verify_rsa_data(char *original_data,
				  mpz_t rop)
{
	utility_retcode_t ret;
	char *decoded_data;
	size_t decoded_data_buflen = 4096;

	FUNC_ENTER;

	DEBG("Verifying RSA data\n");

	decoded_data = syscalls_malloc(decoded_data_buflen);
	if (NULL == decoded_data) {
		ERRR("Could not allocate %d bytes for temp buffer\n",
		     decoded_data_buflen);
		ret = UTILITY_FAILURE;
		goto out;
	}

	ret = raop_rsa_export(decoded_data, decoded_data_buflen, rop);
	if (UTILITY_FAILURE == ret) {
		ERRR("RSA export failed\n");
		goto out;
	}

	if (0 != strncmp(original_data, decoded_data, decoded_data_buflen)) {
		ERRR("RSA data does not match.\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	INFO("RSA data verified successfully\n");

out:
	syscalls_free(decoded_data);
	FUNC_RETURN;
	return ret;
}



utility_retcode_t setup_rsa_key(struct rsa_data *rsa_data)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	raop_rsa_public_key_init(&rsa_data->key);

	ret = raop_rsa_import(rsa_data->encoded_key.modulo,
			      rsa_data->key.n,
			      &rsa_data->modulo_len);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}

	DEBG("Verifying RSA modulo\n");
	ret = verify_rsa_data(rsa_data->encoded_key.modulo,
			      rsa_data->key.n);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}
	DEBG("RSA modulo verified successfully\n");

	ret = raop_rsa_import(rsa_data->encoded_key.exponent,
			      rsa_data->key.e,
			      &rsa_data->exponent_len);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}

	DEBG("Verifying RSA exponent\n");

	ret = verify_rsa_data(rsa_data->encoded_key.exponent,
			      rsa_data->key.e);
	if (UTILITY_FAILURE == ret) {
		goto out;
	}

	if (1 != rsa_public_key_prepare(&rsa_data->key)) {
		ERRR("Failed to prepare RSA public key\n");
		ret = UTILITY_FAILURE;
	} else {
		DEBG("Prepared RSA public key successfully\n");
	}

out:
	FUNC_RETURN;
	return ret;
}


static void raopd_random_func(void *ctx __attribute__ ((unused)),
			      unsigned length,
			      uint8_t *dst)
{
	/* This function is a wrapper to be used with Nettle's RSA
	 * encryption. */
	get_random_bytes(dst, length);

	return;
}


utility_retcode_t raopd_rsa_encrypt_nettle(const struct rsa_public_key *key,
					   const uint8_t *plaintext,
					   const size_t plaintext_len,
					   uint8_t **ciphertext,
					   size_t *ciphertext_len)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	mpz_t rsa_output;

	mpz_init(rsa_output);

	if (1 != rsa_encrypt(key,
			     NULL,
			     raopd_random_func,
			     plaintext_len,
			     plaintext,
			     rsa_output))
	{
		ERRR("RSA encryption failed\n");
		ret = UTILITY_FAILURE;
	}

	/* XXX If mpz_export can't allocate the tempbuf, it terminates
	 * the program.  That's not pretty, but it's not clear to me
	 * whether it checks to see if the buffer is large enough if a
	 * buffer is supplied.  I think the right way to do this is to
	 * supply a custom allocation function to GMP that has a large
	 * buffer that it can hand out repeatedly, although there are
	 * concurrency issues there that would need to be addressed
	 * carefully. */
	DEBG("ciphertext before: %p\n", (void *)*ciphertext);

	*ciphertext = mpz_export(NULL,
				 ciphertext_len,
				 1,
				 sizeof(uint8_t),
				 0,
				 0,
				 rsa_output);

	DEBG("ciphertext after: %p\n", (void *)*ciphertext);

	DEBG("mpz_export wrote %d words of size %d bytes\n",
	     *ciphertext_len, sizeof(uint8_t));

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

	return size;
}
