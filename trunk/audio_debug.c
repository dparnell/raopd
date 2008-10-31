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
#include <sys/types.h>
#include <unistd.h>

#include "syscalls.h"
#include "config.h"
#include "utility.h"
#include "lt.h"
#include "encryption.h"
#include "audio_debug.h"
#include "audio_stream.h"

#define DEFAULT_FACILITY LT_AUDIO_DEBUG

static int rawpcmfd = -1;
static int convertedfd = -1;
static int encryptedfd = -1;
static int raopdcompletefd = -1;
static int raop_playcompletefd = -1;

utility_retcode_t compare_audio_data(uint8_t *buf1,
				     size_t buf1_size,
				     uint8_t *buf2,
				     size_t buf2_size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	size_t size;
	int i;

	if (buf1_size != buf2_size) {
		ERRR("Buffer sizes are not equal, buf1_size: %d, buf2_size: %d\n",
		     buf1_size, buf2_size);
	}

	size = (buf1_size > buf2_size) ? buf1_size : buf2_size;

	INFO("Comparing %d bytes of data\n", size);

	for (i = 0 ; i < (int)size ; i++) {
		if (*(buf1 + i) != *(buf2 + i)) {
			ERRR("Audio data mismatch at byte %d (buf1: 0x%x buf2: 0x%x)\n",
			     i, *(buf1 + i), *(buf2 + i));
			ret = UTILITY_FAILURE;
		}
	}

	return ret;
}


static int open_one_dumpfile(const char *name)
{
	char dumpfile[64];
	int fd, pid;

	INFO("Opening audio dumpfiles\n");

	pid = syscalls_getpid();

	snprintf(dumpfile,
		 sizeof(dumpfile),
		 "./audio_debug/%s.%s.%d", name, CODE_VERSION, pid);

	fd = open(dumpfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (fd < 0) {
		ERRR("Failed to open audio dump file \"%s\"\n", name);
	}

	return fd;
}


utility_retcode_t open_audio_dump_files(void)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	rawpcmfd = open_one_dumpfile("raw_pcm_data");
	if (-1 == rawpcmfd) {
		ret = UTILITY_FAILURE;
		goto out;
	}
	convertedfd = open_one_dumpfile("converted_data");
	if (-1 == convertedfd) {
		ret = UTILITY_FAILURE;
		goto out;
	}
	encryptedfd = open_one_dumpfile("encrypted_data");
	if (-1 == encryptedfd) {
		ret = UTILITY_FAILURE;
		goto out;
	}
	raopdcompletefd = open_one_dumpfile("raopd_complete_data");
	if (-1 == raopdcompletefd) {
		ret = UTILITY_FAILURE;
		goto out;
	}
	raop_playcompletefd = open_one_dumpfile("raop_play_complete_data");
	if (-1 == raop_playcompletefd) {
		ret = UTILITY_FAILURE;
		goto out;
	}

out:
	FUNC_RETURN;
	return ret;
}


static utility_retcode_t dump_audio(int fd, uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int write_ret = 0;

	FUNC_ENTER;

	write_ret = syscalls_write(fd, buf, size);

	if (write_ret != (int)size) {
		ERRR("Write of audio data returned %d\n", write_ret);
		ret = UTILITY_FAILURE;
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t dump_raw_pcm(uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = dump_audio(rawpcmfd, buf, size);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Dump of raw PCM data failed\n");
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t dump_converted(uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = dump_audio(convertedfd, buf, size);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Dump of converted data failed\n");
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t dump_encrypted(uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = dump_audio(encryptedfd, buf, size);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Dump of encrypted data failed\n");
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t dump_complete_raopd(uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = dump_audio(raopdcompletefd, buf, size);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Dump of raopd complete data failed\n");
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t dump_complete_raop_play(uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;

	FUNC_ENTER;

	ret = dump_audio(raop_playcompletefd, buf, size);
	if (UTILITY_SUCCESS != ret) {
		ERRR("Dump of raop_play complete data failed\n");
	}

	FUNC_RETURN;
	return ret;
}


void test_audio(void)
{
	struct aes_data aes_data;
	struct audio_stream audio_stream;
	char pcm_testdata[] = "pcm_testfile";

	CRIT("Testing audio send routines; no connections "
	     "will be made to the server\n");

	lt_set_level(LT_AUDIO_STREAM, LT_INFO);
	lt_set_level(LT_RAOP_PLAY_SEND_AUDIO, LT_INFO);

	generate_aes_data(&aes_data);

	syscalls_strncpy(audio_stream.pcm_data_file,
			 pcm_testdata,
			 sizeof(audio_stream.pcm_data_file));

	audio_stream.session_fd = syscalls_open("./fake_session",
						O_RDWR | O_CREAT | O_TRUNC,
						S_IRUSR | S_IWUSR);

	if (-1 == audio_stream.session_fd) {
		ERRR("Failed to open fake session file descriptor\n");
		goto out;
	}

	init_audio_stream(&audio_stream);
	send_audio_stream(&audio_stream, &aes_data);

out:
	CRIT("Audio test done; exiting\n");
	exit (1);
}
