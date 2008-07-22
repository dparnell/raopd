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
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#include "lt.h"
#include "syscalls.h"

#define DEFAULT_FACILITY LT_LOG

static struct lt_facility default_facility;

void lt(struct lt_facility *custom_facility,
	lt_mask_t mask,
	lt_level_t level,
	const char *file,
	const char *function,
	int line,
	const char *fmt,
	...)
{
	va_list args;
	struct lt_facility *facility;
	char msg[MAX_LT_MSG_LEN];
	char file_func_line[MAX_FUNCTION_NAME_LEN];
	char bits[128];
	size_t used = 0, func_name_used;
	int i;

	bits_to_string((uint64_t)mask, sizeof(mask), bits, sizeof(bits));

	facility = (custom_facility ? custom_facility : &default_facility);

	if ((*facility).masks[level] & mask) {

		func_name_used = syscalls_snprintf(file_func_line,
						   sizeof(file_func_line),
						   "%s:%d %s ",
						   file, line, function);

		for (i = 0 ; i < MAX_FUNCTION_NAME_LEN ; i++) {
			func_name_used += syscalls_snprintf(file_func_line +
							    func_name_used,
							    sizeof(file_func_line) -
							    func_name_used, " ");
		}

		used += syscalls_snprintf(msg + used, sizeof(msg) - used,
					  "%s ", file_func_line);

		va_start(args, fmt);
		used += syscalls_vsnprintf(msg + used, sizeof(msg) - used, fmt, args);
		va_end(args);

		if (used == MAX_LT_MSG_LEN) {
			msg[MAX_LT_MSG_LEN - 1] = '\n';
		}

		LOG_OUTPUT("%s", msg);
	}

	return;
}

void lt_init_custom(struct lt_facility *facility)
{
	lt_level_t i;

	FUNC_ENTER;

	for (i = 0 ; i <= LT_DEFAULT_LEVEL ; i++) {
		(*facility).masks[i] = (lt_mask_t)LT_DEFAULT_MASK;
	}

	FUNC_RETURN;

	return;
}

void lt_init(void)
{
	FUNC_ENTER;

	lt_init_custom(&default_facility);

	FUNC_RETURN;

	return;
}

void lt_set_level_custom(struct lt_facility *facility,
			 lt_mask_t mask_bits,
			 lt_level_t level)
{
	lt_level_t i;
	char bits[128];

	FUNC_ENTER;

	bits_to_string((uint64_t)mask_bits, sizeof(mask_bits) * 8, bits, sizeof(bits));
	DEBG("mask_bits: %s\n", bits);

	/* Set the requested bit in each mask up to level. */
	for (i = 0 ; i <= level ; i++) {
		(*facility).masks[i] |= mask_bits;
	}

	/* Unset the requested bit in the rest of the masks. */
	mask_bits = ~mask_bits;
	for ( ; i < NUM_LT_LEVELS ; i++) {
		(*facility).masks[i] &= mask_bits;
	}

	FUNC_RETURN;
}

void lt_set_level(lt_mask_t mask_bits, lt_level_t level)
{
	FUNC_ENTER;

	lt_set_level_custom(&default_facility, mask_bits, level);

	FUNC_RETURN;

	return;
}

void lt_set_level_by_position_custom(struct lt_facility *facility,
				     lt_facility_position_t position,
				     lt_level_t level)
{
	lt_mask_t mask_bits;

	FUNC_ENTER;

	DEBG("position: %d\n", position);

	mask_bits = ((lt_mask_t)0x1 << position);

	lt_set_level_custom(facility, mask_bits, level);

	FUNC_RETURN;

	return;
}

void lt_set_level_by_position(lt_facility_position_t position,
			      lt_level_t level)
{
	FUNC_ENTER;

	lt_set_level_by_position_custom(&default_facility, position, level);

	FUNC_RETURN;

	return;
}

void lt_show_facility_custom(struct lt_facility *facility)
{
	int i;
	char bits[128];

	FUNC_ENTER;

	for (i = 0 ; i < NUM_LT_LEVELS ; i++) {
		bits_to_string((uint64_t)(*facility).masks[i],
			       sizeof(lt_mask_t) * 8,
			       bits,
			       sizeof(bits));

		EMRG("level %d: %s\n", i, bits);
	}

	FUNC_RETURN;

	return;
}

void lt_show_facility(void)
{
	FUNC_ENTER;

	lt_show_facility_custom(&default_facility);

	FUNC_RETURN;

	return;
}
