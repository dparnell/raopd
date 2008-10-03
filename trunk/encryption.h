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

#include "utility.h"
#include "rtsp.h"

#define RAOP_AES_BLOCK_SIZE		64
#define RAOP_AES_IV_LEN			16
#define RAOP_AES_KEY_LEN		16
#define RAOP_RSA_PUB_MODULUS_LEN	256
#define RAOP_RSA_PUB_EXPONENT_LEN	16

struct encoded_rsa_public_key {
	char *modulo;
	char *exponent;
};

struct rsa_public_key_data {
	uint8_t *modulus;
	uint8_t *exponent;
};

struct rsa_data {
	struct encoded_rsa_public_key encoded_key;
	struct rsa_public_key_data decoded_key;
};

struct aes_data {
	uint8_t iv[RAOP_AES_IV_LEN];
	uint8_t key[RAOP_AES_KEY_LEN];
	uint8_t rsa_encrypted_key[RAOP_RSA_PUB_MODULUS_LEN];
};

utility_retcode_t generate_aes_iv(struct aes_data *aes_data);
utility_retcode_t generate_aes_key(struct aes_data *aes_data);

int raopd_rsa_encrypt_openssl(uint8_t *text, int len, uint8_t *res);

utility_retcode_t write_encrypted_data(int data_fd,
				       int session_fd,
				       struct aes_data *aes_data);

#endif /* #ifndef ENCRYPTION_H */
