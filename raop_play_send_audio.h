/* 
Copyright (C) 2004 Shiro Ninomiya <shiron@snino.com>
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

#ifndef RAOP_PLAY_SEND_AUDIO_H
#define RAOP_PLAY_SEND_AUDIO_H

#include <signal.h>
#include <samplerate.h>

#include "encryption.h"

#define MAIN_EVENT_TIMEOUT 3 // sec unit

#define	RAOP_FD_READ (1<<0)
#define RAOP_FD_WRITE (1<<1)

#define RAOP_CONNECTED "connected"
#define RAOP_SONGDONE "done"
#define RAOP_ERROR "error"

#define DEFAULT_SAMPLE_RATE 44100
#define MAX_SAMPLES_IN_CHUNK 4096
#define MINIMUM_SAMPLE_SIZE 32

#define VOLUME_DEF -30
#define VOLUME_MIN -144
#define VOLUME_MAX 0

#define AUDIO_STREAM_C
#define PCM_STREAM_C_

#define GET_BIGENDIAN_INT(x) (*(uint8_t*)(x)<<24)|(*((uint8_t*)(x)+1)<<16)|(*((uint8_t*)(x)+2)<<8)|(*((uint8_t*)(x)+3))

typedef struct pcm_t {
/* public variables */
/* private variables */
#ifdef PCM_STREAM_C_
	int dfd;
	uint8_t *buffer;
#else
	uint32_t dummy;
#endif
} pcm_t;

typedef enum pause_state_t{
	NO_PAUSE=0,
	OP_PAUSE,
	NODATA_PAUSE,
} pause_state_t;

typedef struct rtspcl_t {uint32_t dummy;} rtspcl_t;
typedef struct raopcl_t {uint32_t dummy;} raopcl_t;
typedef int (*fd_callback_t)(void *, int);

typedef enum data_type_t {
	AUD_TYPE_NONE = 0,
	AUD_TYPE_ALAC,
	AUD_TYPE_WAV,
	AUD_TYPE_MP3,
	AUD_TYPE_OGG,
	AUD_TYPE_AAC,
	AUD_TYPE_URLMP3,
	AUD_TYPE_PLS,
	AUD_TYPE_PCM,
	AUD_TYPE_FLAC,
} data_type_t;

typedef enum data_source_type_t {
	DESCRIPTOR=0,
	STREAM,
	MEMORY,
} data_source_type_t;

typedef struct mem_source_t {
	int size;
	int16_t *data;
} mem_source_t;

typedef struct data_source_t {
	data_source_type_t type;
	union{
		int fd;
		FILE *inf;
		mem_source_t mem;
	}u;
} data_source_t;


typedef struct auds_t {
/* public variables */
	void *stream;
	int sample_rate;
 	void (*sigchld_cb)(void *, siginfo_t *);
	int chunk_size;
	data_type_t data_type;
	int channels;
	struct auds_t *auds;
/* private variables */
#ifdef AUDIO_STREAM_C
	SRC_STATE *resamp_st;
	SRC_DATA resamp_sd;
	uint16_t *resamp_buf;
#else
	uint32_t dummy;
#endif
} auds_t;


typedef struct raopcl_data_t {
	rtspcl_t *rtspcl;
	uint8_t iv[16]; // initialization vector for aes-cbc
	uint8_t nv[16]; // next vector for aes-cbc
	uint8_t key[16]; // key for aes-cbc
	char *addr; // target host address
	uint16_t rtsp_port;
	int ajstatus;
	int ajtype;
	int volume;
	int sfd; // stream socket fd
	int wblk_wsize;
	int wblk_remsize;
	pause_state_t pause;
	int wait_songdone;
	uint8_t *data;
	uint8_t min_sdata[MINIMUM_SAMPLE_SIZE*4+16];
	int min_sdata_size;
	time_t paused_time;
	int size_in_aex;
	struct timeval last_read_tv;
} raopcl_data_t;

typedef struct fdev_t{
	int fd;
	void *dp;
	fd_callback_t cbf;
	int flags;
}dfev_t;

#define MAX_NUM_OF_FDS 4

typedef struct raopld_t{
	raopcl_t *raopcl;
	auds_t *auds;
	dfev_t fds[MAX_NUM_OF_FDS];
}raopld_t;

int hacked_send_audio(char *pcm_audio_file, int session_fd, struct aes_data *aes_data);

#endif /* #ifndef RAOP_PLAY_SEND_AUDIO_H */
