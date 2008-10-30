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
#ifndef AUDIO_DEBUG_H
#define AUDIO_DEBUG_H

utility_retcode_t compare_audio_data(uint8_t *buf1,
				     size_t buf1_size,
				     uint8_t *buf2,
				     size_t buf2_size);
utility_retcode_t open_audio_dump_files(void);
utility_retcode_t dump_raw_pcm(uint8_t *buf, size_t size);
utility_retcode_t dump_converted(uint8_t *buf, size_t size);
utility_retcode_t dump_encrypted(uint8_t *buf, size_t size);
utility_retcode_t dump_complete_raopd(uint8_t *buf, size_t size);
utility_retcode_t dump_complete_raop_play(uint8_t *buf, size_t size);
void test_audio(void);

#endif /* #ifndef AUDIO_DEBUG_H */
