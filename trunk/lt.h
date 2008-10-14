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
#ifndef LT_H
#define LT_H

#include <stdint.h>
#include <stdio.h>

#include "lt_user.h"
#include "utility.h"

#define NUM_LT_LEVELS			8
#define MAX_LT_MSG_LEN			1024
#define NUM_LT_MSGS			4096
#define MAX_FUNCTION_NAME_LEN		36
#define MAX_LT_CUSTOM_FACILITIES	4

#define EMRG(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_EMERG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define ALRT(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_ALERT, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define CRIT(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_CRIT, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define ERRR(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_ERR, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define WARN(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_WARNING, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define NOTC(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_NOTICE, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define INFO(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define DEBG(...)							\
	lt(NULL, DEFAULT_FACILITY, LT_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define LT(level, ...)							\
	lt(NULL, DEFAULT_FACILITY, level, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define LT_CUSTOM(facility, level, ...)					\
	lt(NULL, facility, level, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define FUNC_ENTER	LT_CUSTOM(LT_FUNCTION_CALLS, LT_DEBUG, \
				  "entering %s\n", __func__)
#define FUNC_RETURN	LT_CUSTOM(LT_FUNCTION_CALLS, LT_DEBUG, \
				  "returning from %s\n", __func__)

/* The following values are taken directly from man syslog. */
typedef enum {
	LT_EMERG,	/* 0 system is unusable               */
	LT_ALERT,	/* 1 action must be taken immediately */
	LT_CRIT,	/* 2 critical conditions              */
	LT_ERR,		/* 3 error conditions                 */
	LT_WARNING,	/* 4 warning conditions               */
	LT_NOTICE,	/* 5 normal but significant condition */
	LT_INFO,	/* 6 informational                    */
	LT_DEBUG	/* 7 debug-level messages             */
} lt_level_t;

typedef enum {
	LT_MAIN_POSITION,
	LT_CONFIG_POSITION,
	LT_SYSCALLS_POSITION,
	LT_LOG_POSITION,
	LT_UTILITY_POSITION,
	LT_FUNCTION_CALLS_POSITION,
	LT_CLIENT_POSITION,
	LT_RTSP_POSITION,
	LT_RTSP_CLIENT_POSITION,
	LT_SDP_POSITION,
	LT_ENCODING_POSITION,
	LT_ENCRYPTION_POSITION,
	LT_RAOP_PLAY_SEND_AUDIO_POSITION,
	LT_AUDIO_STREAM_POSITION
} lt_facility_position_t;

typedef uint64_t lt_mask_t;

#define LT_MAIN			(((lt_mask_t)0x1) << LT_MAIN_POSITION)
#define LT_CONFIG		(((lt_mask_t)0x1) << LT_CONFIG_POSITION)
#define LT_SYSCALLS		(((lt_mask_t)0x1) << LT_SYSCALLS_POSITION)
#define LT_LOG			(((lt_mask_t)0x1) << LT_LOG_POSITION)
#define LT_UTILITY		(((lt_mask_t)0x1) << LT_UTILITY_POSITION)
#define LT_FUNCTION_CALLS	(((lt_mask_t)0x1) << LT_FUNCTION_CALLS_POSITION)
#define LT_CLIENT		(((lt_mask_t)0x1) << LT_CLIENT_POSITION)
#define LT_RTSP			(((lt_mask_t)0x1) << LT_RTSP_POSITION)
#define LT_RTSP_CLIENT		(((lt_mask_t)0x1) << LT_RTSP_CLIENT_POSITION)
#define LT_SDP			(((lt_mask_t)0x1) << LT_SDP_POSITION)
#define LT_ENCODING		(((lt_mask_t)0x1) << LT_ENCODING_POSITION)
#define LT_ENCRYPTION		(((lt_mask_t)0x1) << LT_ENCRYPTION_POSITION)
#define LT_RAOP_PLAY_SEND_AUDIO	(((lt_mask_t)0x1) << LT_RAOP_PLAY_SEND_AUDIO_POSITION)
#define LT_AUDIO_STREAM		(((lt_mask_t)0x1) << LT_AUDIO_STREAM_POSITION)

#define LT_DEFAULT_MASK		(((lt_mask_t)(~0)) ^ LT_FUNCTION_CALLS)
#define LT_DEFAULT_LEVEL	LT_WARNING

/* XXX This structure should be called facility_set--a single facility
 * is, e.g., LT_RTSP --DPA */
struct lt_facility {
	char name[MAX_NAME_LEN];
	lt_mask_t masks[NUM_LT_LEVELS];
};

struct lt_message {
	utility_lock_t lock;
	char msg[MAX_LT_MSG_LEN];
};

struct lt_ringbuffer {
	utility_lock_t lock;
	struct lt_message msg[NUM_LT_MSGS];
	int index;
};

/* The format attribute specifies that a function takes printf, scanf,
 * strftime or strfmon style arguments which should be type-checked
 * against a format string.  See:
 *
 * http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 */
void lt(struct lt_facility *custom_facility,
	lt_mask_t mask,
	lt_level_t level,
	const char *file,
	const char *function,
	int line,
	const char *fmt,
	...) __attribute__ ((format (printf, 7, 8)));
void lt_init_custom(struct lt_facility *custom_facility);
void lt_init(void);
void lt_show_facility_custom(struct lt_facility *custom_facility);
void lt_show_facility(void);
void lt_set_level_custom(struct lt_facility *facility,
			 lt_mask_t mask_bits,
			 lt_level_t level);
void lt_set_level(lt_mask_t mask_bits, lt_level_t level);
void lt_set_level_by_position_custom(struct lt_facility *facility,
				     lt_facility_position_t position,
				     lt_level_t level);
void lt_set_level_by_position(lt_facility_position_t position,
			      lt_level_t level);

#endif /* #ifndef LT_H */
