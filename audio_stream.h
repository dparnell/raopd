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
#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include "encryption.h"
#include "config.h"

#define PCM_BUFLEN 32 * 1024
#define CONVERTED_BUFLEN 32 * 1024
#define ENCRYPTED_BUFLEN 32 * 1024
#define TRANSMIT_BUFLEN 32 * 1024
#define PCM_READ_SIZE 16 * 1024
#define PCM_BYTES_PER_SAMPLE 2

#define AUDIO_WRITE_RETRIES 10
#define SERVER_READ_RETRIES 10

#define SERVER_POLL_TIMEOUT 3000 /* miliseconds */

struct transmit_buffer {
	uint8_t header[16];
	uint8_t data[];
};

struct audio_stream {
	char pcm_data_file[MAX_FILE_NAME_LEN];
	int pcm_fd;
	int session_fd;
	int server_ready_for_reading;
	int server_ready_for_writing;

	uint8_t *pcm_buf;
	size_t pcm_bufsize;
	size_t pcm_len;
	size_t pcm_num_samples_read;
	int pcm_data_available;

	uint8_t *converted_buf;
	size_t converted_bufsize;
	size_t converted_len;

	uint8_t *encrypted_buf;
	size_t encrypted_bufsize;
	size_t encrypted_len;

	uint8_t *transmit_buf;
	size_t transmit_bufsize;
	size_t transmit_len;

	size_t written;
	unsigned long long total_bytes_transmitted;
};

//#define USE_RAOP_PLAY_CODE

#ifdef USE_RAOP_PLAY_CODE

#define CODE_VERSION "raop_play"
#define send_audio_stream_internal raop_play_send_audio_stream

#else /* #ifdef USE_RAOP_PLAY_CODE */

#define CODE_VERSION "raopd"
#define send_audio_stream_internal raopd_send_audio_stream

#endif /* #ifdef USE_RAOP_PLAY_CODE */

utility_retcode_t init_audio_stream(struct audio_stream *audio_stream);
utility_retcode_t raop_play_send_audio_stream(struct audio_stream *audio_stream,
					      struct aes_data *aes_data);
utility_retcode_t raopd_send_audio_stream(struct audio_stream *audio_stream,
					  struct aes_data *aes_data);
utility_retcode_t send_audio_stream(struct audio_stream *audio_stream,
				    struct aes_data *aes_data);

utility_retcode_t convert_audio_data(struct audio_stream *audio_stream);
utility_retcode_t raop_play_convert_audio_data(struct audio_stream *audio_stream);
utility_retcode_t raopd_convert_audio_data(struct audio_stream *audio_stream);

#endif /* #ifndef AUDIO_STREAM_H */
