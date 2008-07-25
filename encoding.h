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
#ifndef ENCODING_H
#define ENCODING_H

#include "utility.h"

#define USING_NSS

#ifdef USING_NSS

#define raopd_base64_encode raopd_base64_encode_nss
#define raopd_base64_decode raopd_base64_decode_nss

#else /* #ifdef USING_NSS */

#define raopd_base64_encode raopd_base64_encode_nettle
#define raopd_base64_decode raopd_base64_decode_nettle

#endif /* #ifdef USING_NSS */

utility_retcode_t raopd_base64_encode_nettle(char *dst,
					     size_t dstlen,
					     uint8_t *src,
					     size_t srclen,
					     size_t *encoded_length);

utility_retcode_t raopd_base64_decode_nettle(uint8_t *dst,
					     size_t dstlen,
					     char *src,
					     size_t srclen,
					     size_t *decoded_length);

utility_retcode_t raopd_base64_encode_nss(char *dst,
					  size_t dstlen,
					  uint8_t *src,
					  size_t srclen,
					  size_t *encoded_length);

utility_retcode_t raopd_base64_decode_nss(uint8_t *dst,
					  size_t dstlen,
					  const char *src,
					  const size_t srclen,
					  size_t *decoded_length);

void remove_base64_padding(char *encoded_string);

#endif /* #ifndef ENCODING_H */
