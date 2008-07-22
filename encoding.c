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
#include <nettle/base64.h>

#include "lt.h"
#include "utility.h"
#include "encoding.h"
#include "syscalls.h"

#define DEFAULT_FACILITY LT_ENCODING

utility_retcode_t raopd_base64_encode_nettle(char *dst,
					     size_t dstlen,
					     uint8_t *src,
					     size_t srclen,
					     size_t *encoded_length)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	*encoded_length = BASE64_ENCODE_RAW_LENGTH(srclen);

	DEBG("source length: %d encoded length: %d destination length: %d\n",
	     (int)srclen, (int)*encoded_length, (int)dstlen);

	if (*encoded_length > dstlen) {
		WARN("Destination string was too short (%d bytes) for "
		     "encoded string (%d bytes)\n",
		     dstlen,
		     *encoded_length);

		ret = UTILITY_FAILURE;
	} else {
		base64_encode_raw((uint8_t *)dst, (unsigned)srclen, (const uint8_t *)src);
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t raopd_base64_decode_nettle(uint8_t *dst,
					     size_t dstlen,
					     char *src,
					     size_t srclen,
					     size_t *decoded_length)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	struct base64_decode_ctx ctx;

	FUNC_ENTER;

	*decoded_length = BASE64_DECODE_LENGTH(srclen);

	DEBG("source length: %d decoded length: %d destination length: %d\n",
	     (int)srclen, (int)*decoded_length, (int)dstlen);

	if (*decoded_length > dstlen) {
		WARN("Destination string was too short when base64 decoding\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	base64_decode_init(&ctx);
	if (1 != base64_decode_update(&ctx,
				      decoded_length,
				      dst,
				      srclen,
				      (const uint8_t *)src)) {
		ERRR("base64 decode of \"%s\" failed\n", src);
		ret = UTILITY_FAILURE;
		goto out;
	}

	INFO("Decoded %d bytes\n", (int)*decoded_length);

out:
	FUNC_RETURN;
	return ret;
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
