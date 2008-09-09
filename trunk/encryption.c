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

#include <nss.h>
#include <pk11func.h>

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


utility_retcode_t generate_aes_iv_nss(struct aes_data *aes_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	DEBG("Generating a %d byte initialization vector\n", RAOP_AES_IV_LEN);

	aes_data->nss_iv.data = syscalls_malloc(RAOP_AES_IV_LEN);
	if (NULL == aes_data->nss_iv.data) {
		ERRR("Failed to allocate %d bytes for AES IV\n",
		     RAOP_AES_IV_LEN);
		ret = UTILITY_FAILURE;
		goto out;
	}

	ret = get_random_bytes(aes_data->nss_iv.data, RAOP_AES_IV_LEN);
	if (UTILITY_SUCCESS != ret) {
		goto out;
	}

	aes_data->nss_iv.len = RAOP_AES_IV_LEN;

	DEBG("Setting up PKCS11 parameter from AES IV\n");

	aes_data->sec_param = PK11_ParamFromIV(CKM_AES_CBC_PAD, &aes_data->nss_iv);
	if (NULL == aes_data->sec_param) {
		ERRR("Failed to set up PKCS11 parameter\n");
		goto out;
	}

out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t generate_aes_key_nss(struct aes_data *aes_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	DEBG("Generating a %d byte AES key using NSS\n", RAOP_AES_KEY_LEN);

	aes_data->slot = PK11_GetInternalKeySlot();
	if (NULL == aes_data->slot) {
		ret = UTILITY_FAILURE;
		goto done;
	}

	aes_data->unwrapped_key = PK11_KeyGen(aes_data->slot,
					      CKM_AES_KEY_GEN,
					      0,
					      RAOP_AES_KEY_LEN,
					      0);

	if (NULL == aes_data->unwrapped_key) {
		ret = UTILITY_FAILURE;
	}

done:
	FUNC_RETURN;
	return ret;
}

utility_retcode_t wrap_aes_key(struct aes_data *aes_data,
			       struct rsa_data *rsa_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	SECStatus status;

	FUNC_ENTER;

	DEBG("Wrapping (encrypting) AES key\n");

	aes_data->wrapped_key.len = SECKEY_PublicKeyStrength(rsa_data->key);

	INFO("Wrapped key length: %d\n", aes_data->wrapped_key.len);

	aes_data->wrapped_key.data = syscalls_malloc(aes_data->wrapped_key.len);
	if (NULL == aes_data->wrapped_key.data) {

		ERRR("Failed to allocate %d bytes for encrypted key\n",
		     aes_data->wrapped_key.len);
		ret = UTILITY_FAILURE;
		goto out;

	}

	/* From the openssl manpage:

	   RSA_PKCS1_OAEP_PADDING
           EME-OAEP as defined in PKCS #1 v2.0 with SHA-1, MGF1 and an empty encoding
           parameter. This mode is recommended for all new applications.

	   This is *NOT* what I've done in the call to PK11_PubWrapSymKey.
	*/
	status = PK11_PubWrapSymKey(CKM_RSA_PKCS_OAEP, // CKM_RSA_X_509
				    rsa_data->key,
				    aes_data->unwrapped_key,
				    &aes_data->wrapped_key);

	if (status != SECSuccess) {
		ERRR("Failed to wrap AES key\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	DEBG("Wrapped AES key successfully\n");

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

	return size;
}


/* XXX Need error checking */
static utility_retcode_t decode_rsa_key(struct rsa_data *rsa_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	rsa_data->decoded_key.modulus.data =
		syscalls_malloc(RAOP_RSA_PUB_MODULUS_LEN);

	raopd_base64_decode_nss(rsa_data->decoded_key.modulus.data,
				RAOP_RSA_PUB_MODULUS_LEN,
				rsa_data->encoded_key.modulo,
				syscalls_strlen(rsa_data->encoded_key.modulo),
				&rsa_data->decoded_key.modulus.len);

	rsa_data->decoded_key.modulus.type = siUnsignedInteger;

	rsa_data->decoded_key.exponent.data =
		syscalls_malloc(RAOP_RSA_PUB_EXPONENT_LEN);

	raopd_base64_decode_nss(rsa_data->decoded_key.exponent.data,
				RAOP_RSA_PUB_EXPONENT_LEN,
				rsa_data->encoded_key.modulo,
				syscalls_strlen(rsa_data->encoded_key.exponent),
				&rsa_data->decoded_key.exponent.len);

	rsa_data->decoded_key.exponent.type = siUnsignedInteger;

	FUNC_RETURN;
	return ret;
}


/* See the NSS technical note at:
 * http://www.mozilla.org/projects/security/pki/nss/tech-notes/tn7.html
 */
/* XXX Need error checking */
utility_retcode_t setup_rsa_key(struct rsa_data *rsa_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	const SEC_ASN1Template ASN1_public_key_template[] = {
		{ SEC_ASN1_SEQUENCE,
		  0,
		  NULL,
		  sizeof(struct rsa_public_key_data), },

		{ SEC_ASN1_INTEGER,
		  offsetof(struct rsa_public_key_data, modulus),
		  NULL,
		  0, },

		{ SEC_ASN1_INTEGER,
		  offsetof(struct rsa_public_key_data, exponent),
		  NULL,
		  0, },

		{ 0, 0, NULL, 0, }
	};

	FUNC_ENTER;

	DEBG("Attempting to decode RSA key\n");

	decode_rsa_key(rsa_data);

	DEBG("Attempting to create ASN.1 DER encoded RSA key\n");

	SEC_ASN1EncodeItem(NULL,
			   &rsa_data->der_encoded_pub_key,
			   &rsa_data->decoded_key,
			   ASN1_public_key_template);

	DEBG("Attempting to import ASN.1 encoded RSA key\n");

	rsa_data->key =
		SECKEY_ImportDERPublicKey(&rsa_data->der_encoded_pub_key,
					  CKK_RSA);

	FUNC_RETURN;
	return ret;
}
