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
#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <nettle/aes.h>
#include <nettle/rsa.h>
#include <nettle/cbc.h>

#include "utility.h"
#include "rtsp.h"

#define RAOP_AES_BLOCK_SIZE	64
#define RAOP_AES_IV_LEN		16
#define RAOP_AES_KEY_LEN	16

struct encoded_rsa_public_key {
	char *modulo;
	char *exponent;
};

struct rsa_data {
	struct encoded_rsa_public_key encoded_key;
	struct rsa_public_key key;
	size_t modulo_len;
	size_t exponent_len;
};

struct aes_data {
	uint8_t iv[RAOP_AES_IV_LEN];
	uint8_t key[RAOP_AES_KEY_LEN];
	struct CBC_CTX(struct aes_ctx, AES_BLOCK_SIZE) aes;
};

utility_retcode_t set_session_key(struct aes_data *aes_data);
utility_retcode_t generate_aes_iv(struct aes_data *aes_data);
utility_retcode_t generate_aes_key(struct aes_data *aes_data);
void raop_rsa_public_key_init(struct rsa_public_key *key);
void raop_rsa_public_key_clear(struct rsa_public_key *key);
utility_retcode_t raop_rsa_import(char *buf, mpz_t rop, size_t *len);
utility_retcode_t raop_rsa_export(char *buf,
				  size_t buflen,
				  mpz_t rop);
utility_retcode_t verify_rsa_data(char *original_data,
				  mpz_t rop);
utility_retcode_t setup_rsa_key(struct rsa_data *rsa_data);

#define raopd_rsa_encrypt raopd_rsa_encrypt_openssl

utility_retcode_t raopd_rsa_encrypt_nettle(const struct rsa_public_key *key,
					   const uint8_t *plaintext,
					   const size_t plaintext_len,
					   uint8_t **ciphertext,
					   size_t *ciphertext_len);

int raopd_rsa_encrypt_openssl(uint8_t *text, int len, uint8_t *res);

#endif /* #ifndef ENCRYPTION_H */
