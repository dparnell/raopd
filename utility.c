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

#include "lt.h"
#include "utility.h"
#include "syscalls.h"

#define DEFAULT_FACILITY LT_UTILITY

/* bits_to_string is used by lt()  Do not attempt to log anything. */
void bits_to_string(uint64_t num, int size, char *buf, size_t buflen)
{
	int i;
	size_t used = 0;
	int pos = 0;

	for (i = (size - 1) ; i >= 0 ; i--) {

		used += syscalls_snprintf(buf + used,
					  buflen - used,
					  "%lld",
					  (num >> i) & 1);

		pos++;

		if ((pos != size) && !(pos % 8)) {
			used += syscalls_snprintf(buf + used,
						  buflen - used,
						  " ");
		}
	}

	return;
}


static void add_entry_to_list(struct utility_locked_list *list,
			      struct utility_list_entry *entry,
			      const char *description)
{
	FUNC_ENTER;

	DEBG("&list: %p, &list->lock: %p\n",
	     (void *)list, (void *)&list->lock);

	syscalls_pthread_mutex_lock(&list->lock);

	/* If there's at least one item already in the list */
	if (list->tail) {

		DEBG("this->tail exists\n");

		if (list->tail->next == NULL) {

			list->tail->next = entry;
			entry->prev = list->tail;
			list->tail = entry;

			DEBG("entry: %p "
			     "list->tail->next: %p entry->prev: %p "
			     "list->tail: %p list->head: %p\n",
			     (void *)entry,
			     (void *)list->tail->next,
			     (void *)entry->prev,
			     (void *)list->tail,
			     (void *)list->head);

		} else {
			EMRG("Detected list corruption when adding "
			     "entry \"%s\" to list \"%s\"\n",
			     description, list->description);
			syscalls_abort();
		}

	} else { /* list is empty */

		if (!list->head) {

			list->tail = entry;
			list->head = entry;

		} else {

			EMRG("List was not empty when adding "
			     "first entry \"%s\" to list \"%s\"\n",
			     description, list->description);
			syscalls_abort();
		}
	}

	list->num_entries++;
	DEBG("List \"%s\" now contains %d entries\n",
	     list->description, list->num_entries);

	syscalls_pthread_mutex_unlock(&list->lock);

	FUNC_RETURN;
	return;
}


utility_retcode_t utility_list_add(struct utility_locked_list *list,
			   void *data,
			   const char *description)
{
	utility_retcode_t ret;
	struct utility_list_entry *entry;

	FUNC_ENTER;

	entry = syscalls_malloc(sizeof(*entry));
	if (NULL == entry) {
		ERRR("Could not allocate memory to add entry "
		     "for \"%s\" to list \"%s\"\n",
		     description, list->description);
		goto malloc_entry_failed;
	}

	entry->next = NULL;
	entry->prev = NULL;
	entry->data = data;

	if (NULL != description) {
		syscalls_strncpy(entry->description,
				 description,
				 sizeof(entry->description));
	}

	DEBG("Attempting to add entry %p to list %p\n",
	     (void *)entry, (void *)list);

	add_entry_to_list(list, entry, description);

	DEBG("Added entry for \"%s\" to list \"%s\"\n",
	     description, list->description);

	ret = UTILITY_SUCCESS;
	goto out;

malloc_entry_failed:
	ret = UTILITY_FAILURE;

out:
	FUNC_RETURN;
	return ret;
}


static void remove_entry_from_list(struct utility_locked_list *list,
				   struct utility_list_entry *this,
				   char *description,
				   void **data)
{
	FUNC_ENTER;

	DEBG("this: %p this->prev: %p this->next %p\n",
	     (void *)this,
	     (void *)this->prev,
	     (void *)this->next);

	syscalls_strncpy(description, this->description, MAX_NAME_LEN);

	if (this->prev) {

		this->prev->next = this->next;

	} else {

		DEBG("this->prev does not exist, "
		     "this is the first list entry\n");

		list->head = this->next;
	}

	if (this->next) {
		DEBG("this: %p this->next: %p this->prev: %p\n",
		     (void *)this,
		     (void *)this->next,
		     (void *)this->prev);

		this->next->prev = this->prev;
	} else {

		DEBG("this->next does not exist, "
		     "this is the last list entry\n");

		list->tail = this->prev;
	}

	if (data != NULL) {
		DEBG("data != NULL\n");
		*data = this->data;
	} else {
		DEBG("data == NULL\n");
	}

	DEBG("Freeing this\n");
	syscalls_free(this);
	this = NULL;

	FUNC_RETURN;
	return;
}


/* See header file for uses of parameters. */
utility_retcode_t utility_list_remove(struct utility_locked_list *list,
			      int(*compare_func)(void *, void *),
			      void *comparision_data,
			      void **data,
			      char *description)
{
	utility_retcode_t ret;
	struct utility_list_entry *this;
	int found = UTILITY_FALSE;

	FUNC_ENTER;

	if (compare_func == NULL) {
		EMRG("No comparision function passed to %s\n", __func__);
		syscalls_abort();
	}

	syscalls_pthread_mutex_lock(&list->lock);

	for (this = list->head ; this != NULL ; this = this->next) {

		DEBG("this: %p this->prev: %p this->next %p\n",
		     (void *)this,
		     (void *)this->prev,
		     (void *)this->next);

		if (compare_func(this->data, comparision_data) == UTILITY_TRUE) {

			found = UTILITY_TRUE;

			remove_entry_from_list(list, this, description, data);

			break;
		}
	}

	list->num_entries--;

	syscalls_pthread_mutex_unlock(&list->lock);

	if (found == UTILITY_TRUE) {

		DEBG("Removed list entry for \"%s\" from list \"%s\"\n",
		     description, list->description);

		ret = UTILITY_SUCCESS;

	} else {

		DEBG("List entry not found in list \"%s\"\n",
		     list->description);
		ret = UTILITY_FAILURE;

	}

	return ret;
}


static int list_return_true(void *entry_data,
			    void *compare_data __attribute__ ((unused)))
{
	FUNC_ENTER;

	DEBG("%s: entry_data: %p\n", __func__, entry_data);

	FUNC_RETURN;
	return UTILITY_TRUE;
}


/* See header file for uses of parameters. */
utility_retcode_t utility_list_remove_all(struct utility_locked_list *list,
					  int(*entry_destructor_func)(void *))
{
	utility_retcode_t ret;
	void *data;
	char description[MAX_NAME_LEN];

	FUNC_ENTER;

	if (entry_destructor_func == NULL) {
		EMRG("No entry destructor function passed to %s\n", __func__);
		syscalls_abort();
	}

	DEBG("Attempting to remove all entries from list \"%s\"\n",
	     list->description);

	while (!(LIST_IS_EMPTY(list))) {
		if (utility_list_remove(list,
					list_return_true,
					NULL,
					&data,
					description) == UTILITY_FAILURE) {

			goto remove_failed;

		}

		if (entry_destructor_func(data) == UTILITY_FAILURE) {
			ERRR("Failed to destroy data for list entry \"%s\"\n",
			     description);
		}
	}

	DEBG("Removed all entries from list \"%s\"\n",
	     list->description);

	ret = UTILITY_SUCCESS;
	goto out;

remove_failed:
	ERRR("Failed to remove all entries from list \"%s\"\n",
	     list->description);
	ret = UTILITY_FAILURE;

out:
	return ret;
}

int utility_list_get_num_entries(struct utility_locked_list *list)
{
	int ret;

	FUNC_ENTER;

	syscalls_pthread_mutex_lock(&list->lock);

	ret = list->num_entries;

	syscalls_pthread_mutex_unlock(&list->lock);

	FUNC_RETURN;
	return ret;
}


/* See header file for uses of parameters. */
utility_retcode_t utility_list_update(
	struct utility_locked_list *list,
	int(*comp_func)(void *, void *),
	void *comparision_data,
	int(*update_func)(void *entry_data, void *update_data),
	void *update_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int updated = UTILITY_FALSE;
	int found = UTILITY_FALSE;
	struct utility_list_entry *this;

	FUNC_ENTER;

	if (comp_func == NULL) {
		EMRG("No comparision function passed to %s\n", __func__);
		syscalls_abort();
	}

	syscalls_pthread_mutex_lock(&list->lock);

	for (this = list->head ; this != NULL ; this = this->next) {
		if (comp_func(this->data, comparision_data) == UTILITY_TRUE) {

			DEBG("Found entry \"%s\" in list \"%s\"\n",
			     this->description,
			     list->description);

			found = UTILITY_TRUE;

			if (update_func(this->data, update_data) == UTILITY_SUCCESS) {

				DEBG("Updated entry \"%s\" in "
				     "list \"%s\"\n",
				     this->description,
				     list->description);

				updated = UTILITY_TRUE;
			}

			break;
		}
	}

	syscalls_pthread_mutex_unlock(&list->lock);

	if (found == UTILITY_FALSE) {
		DEBG("List entry not found for update in list \"%s\"\n",
		     list->description);
		ret = UTILITY_FAILURE;
	} else {

		if (updated == UTILITY_FALSE) {
			DEBG("List entry not updated in list \"%s\"\n",
			     list->description);
			ret = UTILITY_FAILURE;
		}
	}

	FUNC_RETURN;
	return ret;
}


/* See header file for uses of parameters. */
utility_retcode_t utility_list_walk(
	struct utility_locked_list *list,
	int(*worker_func)(void *entry_data, void *data),
	void *data,
	size_t offset)
{
	utility_retcode_t ret;
	struct utility_list_entry *this;

	FUNC_ENTER;

	DEBG("Starting list walk of list \"%s\"\n",
	     list->description);

	syscalls_pthread_mutex_lock(&list->lock);

	DEBG("Acquired list lock\n");

	for (this = list->head ; this != NULL ; this = this->next) {

		DEBG("this: %p this->prev: %p this->next %p\n",
		     (void *)this,
		     (void *)this->prev,
		     (void *)this->next);

		if (worker_func(this->data, data) == UTILITY_SUCCESS) {

			DEBG("Processed list entry \"%s\" in "
			     "list \"%s\"\n",
			     this->description,
			     list->description);

		} else {

			DEBG("List entry not processed in list \"%s\"\n",
			     list->description);
			goto worker_failed;

		}
		data = ((uint8_t *)data) + offset;

	}

	syscalls_pthread_mutex_unlock(&list->lock);

	DEBG("List walk of list \"%s\" succeeded\n",
	     list->description);

	ret = UTILITY_SUCCESS;
	goto out;

worker_failed:
	ret = UTILITY_FAILURE;
	DEBG("List walk of list \"%s\" failed\n",
	     list->description);

out:
	FUNC_RETURN;
	return ret;
}


/* See header file for uses of parameters. */
utility_retcode_t utility_list_create(struct utility_locked_list **list,
			      const char *description)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	*list = syscalls_malloc(sizeof(struct utility_locked_list));
	if (NULL == *list) {
		ERRR("Could not allocate memory to create list \"%s\"\n",
		     description);
		goto malloc_failed;
	}

	(*list)->head = NULL;
	(*list)->tail = NULL;

	if ((syscalls_pthread_mutex_init(&(*list)->lock, NULL)) != 0) {
		ERRR("Could not initialize mutex for list \"%s\"\n",
		     description);
		goto mutex_init_failed;
	}

	strncpy((*list)->description, description, MAX_NAME_LEN);

	DEBG("Created list \"%s\"\n", (*list)->description);

	ret = UTILITY_SUCCESS;
	goto out;

mutex_init_failed:
	ERRR("Failed to create list \"%s\"\n", description);
	syscalls_free(*list);
	*list = NULL;

malloc_failed:
	ret = UTILITY_FAILURE;
out:
	FUNC_RETURN;
	return ret;
}


/* Note that this function is *NOT* thread safe.  The caller must be
 * certain that the list is not in use.  */
utility_retcode_t utility_list_destroy(struct utility_locked_list *list)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	if (list->head == NULL && list->tail == NULL) {

		pthread_mutex_destroy(&list->lock);

		DEBG("Freeing list\n");
		syscalls_free(list);
		list = NULL;

		ret = UTILITY_SUCCESS;

	} else {
		ERRR("Failed to destroy list \"%s\" because it is not empty\n",
		     list->description);
		ret = UTILITY_FAILURE;
	}

	FUNC_RETURN;
	return ret;
}


utility_retcode_t utility_copy_token(char *dest,
				     size_t size,
				     const char *src,
				     const char *delim,
				     char **next)
{
	utility_retcode_t ret;
	char *end;
	size_t tokensize;

	FUNC_ENTER;

	if (NULL == dest) {
		ERRR("Destination string was NULL when attempting to copy token\n");
		goto failed;
	}

	if (NULL == src) {
		ERRR("Source string was NULL when attempting to copy token\n");
		goto failed;
	}

	end = syscalls_strstr(src, delim);
	if (NULL == end) {
		ERRR("Token delimiter was not found when "
		     "attempting to copy token\n");
		goto failed;
	}

	if (NULL != next) {
		*next = end + sizeof(delim);
	}

	tokensize = (end - src) + 1;
	INFO("Size of token is %d\n", tokensize);

	if (tokensize > size) {
		ERRR("Token size (%d bytes) is too large for "
		     "destination string (%d bytes) truncating\n",
		     tokensize, size);
		tokensize = size;
	}

	syscalls_strncpy(dest, src, tokensize);

	ret = UTILITY_SUCCESS;
	goto out;
failed:
	ret = UTILITY_FAILURE;
out:
	FUNC_RETURN;
	return ret;
}


char *begin_token(const char *s, const char *delim)
{
	char *begin;

	FUNC_ENTER;

	DEBG("string: \"%s\" delim: \"%s\"\n", s, delim);

	begin = syscalls_strstr(s, delim);
	if (NULL != begin) {
		begin += syscalls_strlen(delim);
	}

	FUNC_RETURN;
	return begin;
}


utility_retcode_t get_random_bytes(uint8_t *buf, size_t size)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	int fd, syscallret;

	FUNC_ENTER;

	fd = syscalls_open(RANDOM_SOURCE_PATH, O_RDONLY, 0);
	if (fd < 0) {
		ERRR("Failed to open random source \"%s\"\n", RANDOM_SOURCE_PATH);
		goto failed;
	}

	DEBG("Opened random source \"%s\"\n", RANDOM_SOURCE_PATH);

	syscallret = syscalls_read(fd, buf, size);
	if (syscallret < 0) {
		ERRR("Read of %d bytes from random source \"%s\" failed: %d\n",
		     size, RANDOM_SOURCE_PATH, syscallret);
	}

	if (syscallret < (int)size) {
		ERRR("Read only %d bytes from \"%s\" when %d bytes were requested\n",
		     syscallret, RANDOM_SOURCE_PATH, size);
	}

	DEBG("Read %d bytes from random source \"%s\"\n",
	     syscallret, RANDOM_SOURCE_PATH);

	ret = UTILITY_SUCCESS;

	goto out;

failed:
	syscallret = syscalls_close(fd);
	if (syscallret < 0) {
		ERRR("Failed to close random source \"%s\"\n",
		     RANDOM_SOURCE_PATH);
	}

	ret = UTILITY_FAILURE;

out:
	FUNC_RETURN;
	return ret;
}


