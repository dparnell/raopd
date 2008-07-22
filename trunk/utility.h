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
#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include <pthread.h>
#include <sys/param.h>

#include "utility_user.h"

#ifdef MAXHOSTNAMELEN
#define MAX_HOST_NAME_LEN MAXHOSTNAMELEN
#else
#define MAX_HOST_NAME_LEN 64
#endif

#define MAX_NAME_LEN			64	/* Human readable names for objects. */
#define RANDOM_SOURCE_PATH "/dev/urandom"

typedef enum {
	UTILITY_FAILURE = -1,
	UTILITY_SUCCESS
} utility_retcode_t;

typedef enum {
	UTILITY_FALSE,
	UTILITY_TRUE
} utility_boolean_t;

#define LIST_IS_EMPTY(list) (NULL == list->head)

struct key_value_pair {
	char *name;
	char *value;
};

struct utility_locked_list {
	utility_lock_t lock;
	utility_refcnt_t refcnt;
	struct utility_list_entry *head;
	struct utility_list_entry *tail;
	int num_entries;
	char description[MAX_NAME_LEN];
};

struct utility_list_entry {
	struct utility_list_entry *next;
	struct utility_list_entry *prev;
	void *data;
	char description[MAX_NAME_LEN];
};

void bits_to_string(uint64_t num, int size, char *buf, size_t buflen);

utility_retcode_t utility_list_add(struct utility_locked_list *list,
				   void *data,
				   const char *description);

utility_retcode_t utility_list_remove(
 	/* The list to be worked on */
	struct utility_locked_list *list,

	/* comparision function, used to locate the entry This
	 * function must return UTILITY_TRUE or UTILITY_FALSE */
	int(*compare)(void *, void *),

	/* data passed to comparision function to identify an entry */
	void *comparision_data,

	/* if a list entry is removed, its data field will be returned
	 * to the caller in this pointer */
	void **data,

	/* if a list entry is removed, a copy of the removed entry's
	 * description will be returned in this buffer */
	char *description
);

utility_retcode_t utility_list_remove_all(
	/* The list to be worked on */
	struct utility_locked_list *list,

	/* destructor function, used to destroy each entry's data */
	int(*destructor)(void *)
);

int utility_list_get_num_entries(struct utility_locked_list *list);

int utility_list_update(
	/* The list to be worked on */
	struct utility_locked_list *list,

	/* comparision function, used to locate the entry */
	int(*comp_func)(void *, void *),

	/* data passed to comparision function to identify the list entry */
	void *comparision_data,

	/* function that actually carries out the update.  The update
	 * function will be passed the data pointer for the found entry
	 * and the update data pointer passed to this function  */
	int(*update_func)(void *entry_data, void *update_data),

	/* data passed to the update function to modify the list entry */
	void *update_data
);

utility_retcode_t utility_list_walk(
	/* The list to be worked on */
	struct utility_locked_list *list,

	/* worker function to be called on each list entry.  The 
	 * function will be passed the data pointer for the found entry
	 * and the data pointer passed to this function  */
	int(*worker)(void *entry_data, void *data),

	/* data passed to the update function to modify the entry */
	void *data,

	/* Value by which the data pointer is incremented for each
	 * element of the list before being passed to the worker
	 * function.  May be zero if the data pointer should not be
	 * incremented. */
	size_t offset
);

utility_retcode_t utility_list_create(struct utility_locked_list **list,
				      const char *description);
utility_retcode_t utility_list_destroy(struct utility_locked_list *list);
utility_retcode_t utility_copy_token(char *dest,
				     size_t size,
				     const char *src,
				     const char *delim,
				     char **next);
char *begin_token(const char *s, const char *delim);
utility_retcode_t get_random_bytes(uint8_t *buf, size_t size);
utility_retcode_t utility_base64_encode(uint8_t *dst,
					size_t dstlen,
					uint8_t *src,
					size_t srclen);

#endif
