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

utility_retcode_t raopd_base64_encode(char *dst,
				      size_t dstlen,
				      const uint8_t *src,
				      size_t srclen,
				      size_t *encoded_length);

utility_retcode_t raopd_base64_decode(uint8_t *dst,
				      size_t dstlen,
				      const char *src,
				      const size_t srclen,
				      size_t *decoded_length);

void remove_base64_padding(char *encoded_string);

#endif /* #ifndef ENCODING_H */
