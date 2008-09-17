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

#include <openssl/evp.h>

#include "lt.h"
#include "utility.h"
#include "encoding.h"
#include "syscalls.h"

#define DEFAULT_FACILITY LT_ENCODING


/* The following function is necessary because the AirPort barfs on
 * properly columnated base64 encoding. */
static void remove_base64_columnation(char *s, size_t *len)
{
	size_t i, j = 0;

	FUNC_ENTER;

	for (i = 0 ; i < *len ; i++) {
		if (s[i] != '\r' && s[i] != '\n') {
			s[j] = s[i];
			j++;
		}
	}

	/* Reset the encoded length */
	*len = j;

	FUNC_RETURN;
	return;
}


void remove_base64_padding(char *encoded_string)
{
	char *begin_padding;

	FUNC_ENTER;

	begin_padding = syscalls_strchr(encoded_string, '=');

	if (NULL != begin_padding) {
		DEBG("Found padding\n");
		*begin_padding = '\0';
	}

	return;
}


utility_retcode_t raopd_base64_encode(char *dst,
				      size_t dstlen,
				      const uint8_t *src,
				      size_t srclen,
				      size_t *encoded_length)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	DEBG("srclen: %d dstlen: %d\n", srclen, dstlen);

	*encoded_length = EVP_EncodeBlock((uint8_t *)dst, src, srclen);

	remove_base64_columnation(dst, encoded_length);

	INFO("encoded_length: %d\n", *encoded_length);

	FUNC_RETURN;
	return ret;
}


utility_retcode_t raopd_base64_decode(uint8_t *dst,
				      size_t dstlen,
				      const char *src,
				      const size_t srclen,
				      size_t *decoded_length)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	char *begin_padding, *end_padding;

	FUNC_ENTER;

	DEBG("srclen: %d dstlen: %d\n", srclen, dstlen);

	*decoded_length = EVP_DecodeBlock(dst, (const uint8_t *)src, srclen);

	/* Adjust length to account for the somewhat broken OpenSSL API.  See:
	   http://osdir.com/ml/encryption.openssl.devel/2002-08/msg00267.html */
	begin_padding = syscalls_strchr(src, '=');
	end_padding = syscalls_strrchr(src, '=');
	if (NULL != end_padding) {
		*decoded_length -= (end_padding - begin_padding + 1);
	}

	INFO("decoded_length: %d\n", *decoded_length);

	FUNC_RETURN;
	return ret;
}


