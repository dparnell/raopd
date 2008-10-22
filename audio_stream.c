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
#include "syscalls.h"
#include "config.h"
#include "utility.h"
#include "lt.h"
#include "audio_stream.h"
#include "raop_play_send_audio.h"
#include "audio_debug.h"

#define DEFAULT_FACILITY LT_AUDIO_STREAM

utility_retcode_t init_audio_stream(struct audio_stream *audio_stream)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	long arg;

	FUNC_ENTER;

	get_pcm_data_file(audio_stream->pcm_data_file,
			  sizeof(audio_stream->pcm_data_file));

	audio_stream->pcm_fd =
		syscalls_open(audio_stream->pcm_data_file, O_RDONLY, 0);

	if (audio_stream->pcm_fd < 0) { 
		ERRR("Failed to open PCM data file\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	audio_stream->pcm_data_available = 1;

	/* XXX need fcntl added to syscalls.c */
	if ((arg = fcntl(audio_stream->session_fd, F_GETFL, NULL)) < 0) { 
		ERRR("Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
		ret = UTILITY_FAILURE;
		goto out;
	}

	arg |= O_NONBLOCK; 

	if (fcntl(audio_stream->session_fd, F_SETFL, arg) < 0) { 
		ERRR("Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		ret = UTILITY_FAILURE;
		goto out;
	}

	audio_stream->pcm_buf = syscalls_malloc(PCM_BUFLEN);
	if (NULL == audio_stream->pcm_buf) {
		ERRR("Failed to allocate memory for PCM audio buffer\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	audio_stream->converted_buf = syscalls_malloc(CONVERTED_BUFLEN);
	if (NULL == audio_stream->converted_buf) {
		ERRR("Failed to allocate memory for converted audio buffer\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	audio_stream->encrypted_buf = syscalls_malloc(ENCRYPTED_BUFLEN);
	if (NULL == audio_stream->encrypted_buf) {
		ERRR("Failed to allocate memory for encrypted audio buffer\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

	audio_stream->transmit_buf = syscalls_malloc(TRANSMIT_BUFLEN);
	if (NULL == audio_stream->transmit_buf) {
		ERRR("Failed to allocate memory for transmit audio buffer\n");
		ret = UTILITY_FAILURE;
		goto out;
	}

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t read_aex(int session_fd)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	uint8_t buf[256];
	uint32_t *aex_data;
	int rsize, read_ret;

	FUNC_ENTER;

	DEBG("Before read from AEX\n");

	read_ret = syscalls_read(session_fd, buf, sizeof(buf));

	if (0 < read_ret) {
		ERRR("Read %d bytes from AEX\n", ret);
		aex_data = (uint32_t *)(buf + 0x2c);
		rsize = syscalls_ntohl(*aex_data);
		ERRR("rsize: %d\n", rsize);
	}

	if (0 > read_ret) {
		if (EAGAIN == errno) {
			ERRR("Got EAGAIN from read\n");
		} else {
			ERRR("Failure reading from server: %s\n", strerror(errno));
			ret = UTILITY_FAILURE;
		}
	}

	if (0 == read_ret) {
		ERRR("Server closed connection\n");
		ret = UTILITY_FAILURE;
	}

	FUNC_RETURN;
	return ret;
}


static utility_retcode_t read_audio_data(struct audio_stream *audio_stream)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int read_ret;

	DEBG("Attempting to read from PCM data file\n");

	syscalls_memset(audio_stream->pcm_buf, 0, PCM_BUFLEN);
	syscalls_memset(audio_stream->converted_buf, 0, CONVERTED_BUFLEN);
	syscalls_memset(audio_stream->encrypted_buf, 0, ENCRYPTED_BUFLEN);
	syscalls_memset(audio_stream->transmit_buf, 0, TRANSMIT_BUFLEN);

	read_ret = syscalls_read(audio_stream->pcm_fd,
				 audio_stream->pcm_buf,
				 PCM_READ_SIZE);

	if (0 > read_ret) {
		ERRR("Read from PCM data file \"%s\" failed\n",
		     audio_stream->pcm_data_file);
		ret = UTILITY_FAILURE;
		goto out;
	}

	audio_stream->pcm_len = read_ret;

	audio_stream->pcm_num_samples_read =
		audio_stream->pcm_len / PCM_BYTES_PER_SAMPLE;

	INFO("Read %d bytes of pcm data (%d samples)\n",
	     (int)audio_stream->pcm_len,
	     (int)audio_stream->pcm_num_samples_read);

	dump_raw_pcm(audio_stream->pcm_buf, audio_stream->pcm_len);

	if (0 == read_ret) {
		DEBG("Finished reading PCM data file \"%s\"\n",
		     audio_stream->pcm_data_file);
		audio_stream->pcm_data_available = 0;
	}

out:
	return ret;
}


utility_retcode_t convert_audio_data(struct audio_stream *audio_stream)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	INFO("Converting %d samples (%d bytes) to bigendian order\n",
	     audio_stream->pcm_num_samples_read, audio_stream->pcm_len);

	ret = raopd_convert_audio_data(audio_stream);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to convert audio data\n");
	}

	return ret;
}


utility_retcode_t raopd_convert_audio_data(struct audio_stream *audio_stream)
{
	uint8_t *readp, *writep;
	uint8_t msb;
	int i;

	/* These statements set the bitfields in the first 3 bytes of
	 * the audio buffer.  It would be better to do this with a
	 * struct. */
	*audio_stream->converted_buf = 0x20;
	*(audio_stream->converted_buf + 1) = 0x0;
	*(audio_stream->converted_buf + 2) = 0x2;

	readp = audio_stream->pcm_buf;
	writep = (audio_stream->converted_buf + 3);

	/* We want big-endian samples; this logic assumes that the
	 * input PCM data is little-endian. */
	for (i = 0 ; i < (int)audio_stream->pcm_num_samples_read ; i++) {
		*writep = *(readp + 1);
		*(writep + 1) = *readp;

		writep += 2;
		readp += 2;
	}

	writep = audio_stream->converted_buf + 3;
	INFO("Bit-shifting %d bytes\n", audio_stream->pcm_len);
	/* Bit-shift everything left one bit across the byte boundary. (?!?) */
	for (i = 0 ; i < (int)(audio_stream->pcm_len) ; i++) {
		msb = (*writep) >> 7;
		*writep = (*writep) << 1;
		*(writep - 1) |= msb;
		writep++;
	}

	audio_stream->converted_len = audio_stream->pcm_len + 3;

	dump_converted(audio_stream->converted_buf,
		       audio_stream->converted_len);

	return UTILITY_SUCCESS;
}


static utility_retcode_t encrypt_audio_data(struct audio_stream *audio_stream,
					    struct aes_data *aes_data)
{
	DEBG("Attempting to encrypt %d bytes of audio data\n",
	     audio_stream->converted_len);

	aes_encrypt_data(aes_data,
			 audio_stream->encrypted_buf,
			 &audio_stream->encrypted_len,
			 audio_stream->converted_buf,
			 audio_stream->converted_len);

	INFO("Encrypted %d bytes of audio data (encrypted length: %d)\n",
	     audio_stream->converted_len, audio_stream->encrypted_len);

	dump_encrypted(audio_stream->encrypted_buf,
		       audio_stream->encrypted_len);

	return UTILITY_SUCCESS;
}


static utility_retcode_t write_data_to_socket(struct audio_stream *audio_stream)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int retries = 0, write_ret;
	size_t written = 0;

	dump_complete_raopd(audio_stream->transmit_buf + written,
			    audio_stream->transmit_len - written);

	while (written <  audio_stream->transmit_len &&
	       retries < AUDIO_WRITE_RETRIES) {

		DEBG("Attempting to write %d bytes to server "
		     "(written: %d audio_stream->transmit_len: %d)\n",
		     audio_stream->transmit_len - written,
		     written, audio_stream->transmit_len);

		write_ret = syscalls_write(audio_stream->session_fd,
					   audio_stream->transmit_buf + written,
					   audio_stream->transmit_len - written);

		if (write_ret > 0) {

			audio_stream->bytes_transmitted += write_ret;
			written += write_ret;
			DEBG("Wrote %d bytes (written this chunk: %d "
			     "total written: %d)\n",
			     write_ret, written, audio_stream->bytes_transmitted);
		}

		if (0 == write_ret) {
			INFO("Server closed connection\n");
			break;
		}

		if (write_ret < 0) {
			if (EAGAIN == errno) {
				ERRR("Got EAGAIN from write\n");
				syscalls_usleep(100000);
			} else {
				ERRR("Failed to write audio data to server\n");
				ret = UTILITY_FAILURE;
				break;
			}
		}

		retries++;
	}

	ERRR("written: %d audio_stream->transmit_len: %d\n",
	     written, audio_stream->transmit_len);

	if (retries == AUDIO_WRITE_RETRIES) {
		ret = UTILITY_FAILURE;
	}

	return ret;
}


static utility_retcode_t send_audio_data(struct audio_stream *audio_stream)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int reported_len;
	struct transmit_buffer *transmit_buf;

	uint8_t header[] = {
		0x24, 0x00, 0x00, 0x00,
		0xF0, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
        };

	transmit_buf = (struct transmit_buffer *)audio_stream->transmit_buf;

	syscalls_memcpy(transmit_buf->header,
			header,
			sizeof(header));

	syscalls_memcpy(transmit_buf->data,
			audio_stream->encrypted_buf,
			audio_stream->encrypted_len);

	/* It's totally unclear why the transmit len should be 3 bytes
	 * longer than the actual data. */
	audio_stream->transmit_len = audio_stream->encrypted_len + sizeof(header) + 3;

	/* The calculation of length to put into the header is
	 * taken from the raop_play and JustePort code.  It's
	 * not clear why the reported length in the header is
	 * 4 bytes less than the actual length.  */
	reported_len = audio_stream->encrypted_len + sizeof(header) - 4;
	ERRR("reported_len: %d\n", reported_len);

	transmit_buf->header[2] = reported_len >> 8;
	transmit_buf->header[3] = reported_len & 0xff;

	write_data_to_socket(audio_stream);

	return ret;
}


utility_retcode_t raop_play_send_audio_stream(struct audio_stream *audio_stream,
					      struct aes_data *aes_data)
{
	hacked_send_audio(audio_stream->pcm_data_file,
			  audio_stream->session_fd,
			  aes_data);

	return UTILITY_SUCCESS;
}


utility_retcode_t raopd_send_audio_stream(struct audio_stream *audio_stream,
					  struct aes_data *aes_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int retries = 0;

	FUNC_ENTER;

	lt_set_level(LT_AUDIO_STREAM, LT_INFO);

	DEBG("Starting to send audio stream\n");

	ret = initialize_aes(aes_data);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Failed to initialize AES data\n");
		goto out;
	}

	do {
		ret = read_audio_data(audio_stream);
		if (UTILITY_SUCCESS != ret) {
			ERRR("Failed to read audio data\n");
			goto out;
		}

		if (0 != audio_stream->pcm_len) {

			ret = convert_audio_data(audio_stream);
			if (UTILITY_SUCCESS != ret) {
				ERRR("Failed to convert audio data\n");
				goto out;
			}

			ret = encrypt_audio_data(audio_stream, aes_data);
			if (UTILITY_SUCCESS != ret) {
				ERRR("Failed to encrypt audio data\n");
				goto out;
			}

			ret = send_audio_data(audio_stream);
			if (UTILITY_SUCCESS != ret) {
				ERRR("Failed to send audio data\n");
				goto out;
			}
		}

	} while (UTILITY_SUCCESS == read_aex(audio_stream->session_fd) &&
		 audio_stream->pcm_data_available);

	while (UTILITY_SUCCESS == read_aex(audio_stream->session_fd) &&
		retries < SERVER_READ_RETRIES) {

		INFO("Waiting for server to close the connection\n");
		syscalls_sleep(2);
		retries++;
	}


out:
	FUNC_RETURN;
	return ret;
}


utility_retcode_t send_audio_stream(struct audio_stream *audio_stream,
				    struct aes_data *aes_data)
{
	open_audio_dump_files();
	return send_audio_stream_internal(audio_stream, aes_data);
}
