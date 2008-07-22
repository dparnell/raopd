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
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "lt.h"
#include "syscalls.h"

#define DEFAULT_FACILITY LT_SYSCALLS

int syscalls_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int ret;

	ret = vsnprintf(str, size, format, args);

	if (ret >= (int)size) {
		str[size] = '\0';
		ret = size;
	}

	return ret;
}

int syscalls_snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = syscalls_vsnprintf(str, size, format, args);
	va_end(args);

	return ret;
}

int syscalls_socket(int domain, int type, int protocol)
{
	int ret;

	if ((ret = socket(domain, type, protocol)) < 0) {
		ERRR("Failed to create socket: %s\n", strerror(errno));
	}

	return ret;
}

int syscalls_inet_pton(int af, const char *src, void *dst)
{
	int ret;

	if ((ret = inet_pton(af, src, dst)) <= 0) {
		ERRR("Failed to convert string to IP address: %s\n",
		     strerror(errno));
	}

	return ret;
}

int syscalls_connect(int sockfd,
		     const struct sockaddr *serv_addr,
		     socklen_t addrlen)
{
	int ret;

	if ((ret = connect(sockfd, serv_addr, addrlen)) < 0) {
		ERRR("Failed to connect to socket: %s\n", strerror(errno));
	}

	return ret;
}

char *syscalls_strncpy(char *dest, const char *src, size_t n)
{
	char *ret;

	DEBG("strncpy'ing up to %d bytes to %p from %p\n",
	     (int)n, dest, src);

	ret = strncpy(dest, src, n);
	/* We always want the result to be null terminated */
	if (n > 0) {
		dest[n - 1] = '\0';
	}

	return ret;
}

ssize_t syscalls_read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	DEBG("Attempting to read %d bytes from fd %d\n",
	     (int)count, (int)fd);

again:
	ret = read(fd, buf, count);

	if (ret < 0) {
		if (errno == EINTR) {
			goto again;
		}
		ERRR("Read failed: %s (fd: %d)\n", strerror(errno), fd);
	}

	INFO("Read %d bytes (fd: %d)\n", (int)ret, (int)fd);

	return ret;
}

/* XXX TODO FIXME Handle SIGPIPE */
ssize_t syscalls_write(int fd, const void *buf, size_t count)
{
	ssize_t ret;

	ret = write(fd, buf, count);
	if (ret < 0) {
		ERRR("Write failed: %s (fd: %d)\n", strerror(errno), (int)fd);
	}
	DEBG("Wrote %d bytes (fd: %d)\n", (int)ret, (int)fd);
 
	return ret;
}

unsigned int syscalls_sleep(unsigned int seconds)
{
	unsigned int ret;

	ret = sleep(seconds);
	if (0 != ret) {
		INFO("Sleep was interrupted by a signal with %u seconds remaining.\n",
		     ret);
	}

	return ret;
}

void *syscalls_malloc(size_t size) {
	void *ret;

	DEBG("Attempting to malloc %d bytes\n", (int)size);

	if ((ret = malloc(size)) == NULL) {
		ERRR("Failed to malloc %d bytes\n", (int)size);
		goto err;
	}

	/* Always want cleared memory. */
	syscalls_memset(ret, 0, size);

	DEBG("malloc'd %d bytes successfully (%p)\n",
	     (int)size, ret);

err:
	return ret;
}

void syscalls_free(void *ptr) {
	DEBG("Freeing %p\n", ptr);
	free(ptr);
}

void *syscalls_memset(void *s, int c, size_t n)
{
	DEBG("memset address: %p value: %d size: %lu\n",
	     s, c, (unsigned long)n);

	memset(s, c, n);

	return s;
}

size_t syscalls_strlen(const char *s)
{
	size_t size;

	size = strlen(s);

	DEBG("Length of string at %p is %lu\n",
	     s, (unsigned long)size);

	return size;
}

int syscalls_pthread_key_create(pthread_key_t *key, void (*destructor)(void*)) {
	int err;

	if ((err = pthread_key_create(key, destructor)) != 0) {
		ERRR("Failed to create thread key: %s\n", strerror(err));
		abort();
	}

	return err;
}

int syscalls_pthread_create(pthread_t  *thread, pthread_attr_t *attr, void *
			    (*start_routine)(void *), void * arg) {
	int err;

	if ((err = pthread_create(thread, attr, start_routine, arg)) != 0) {
		ERRR("Failed to create thread: %s\n", strerror(err));
	}
	return err;
}

int syscalls_pthread_detach(pthread_t thread) {
	int ret;

	if ((ret = pthread_detach(thread)) != 0) {
		ERRR("Failed to detach thread (id: %lu): %s\n",
		     thread, strerror(ret));
	}
	return ret;
}


int syscalls_pthread_rwlock_init(pthread_rwlock_t *rwlock,
				 const pthread_rwlockattr_t *attr) {
	int ret;

	if ((ret = pthread_rwlock_init(rwlock, attr))) {
		ERRR("Failed to initialize rwlock (%p): %s\n",
		     (void *)rwlock, strerror(ret));
	}

	DEBG("Initialized rwlock (%p)\n", (void *)rwlock);
	return ret;

}

int syscalls_pthread_mutex_init(pthread_mutex_t *mutex,
				const pthread_mutexattr_t *attr) {
	int ret;

	if ((ret = pthread_mutex_init(mutex, attr))) {
		ERRR("Failed to initialize mutex (%p): %s\n",
		     (void *)mutex, strerror(ret));
	}

	DEBG("Initialized mutex (%p)\n", (void *)mutex);
	return ret;

}

int syscalls_pthread_rwlock_destroy(pthread_rwlock_t *rwlock){
	int ret;

	if ((ret = pthread_rwlock_destroy(rwlock))) {
		ERRR("Failed to destroy rwlock (%p): %s\n",
		     (void *)rwlock, strerror(ret));
	}
	DEBG("Destroyed rwlock (%p)\n", (void *)rwlock);

	return ret;
}

/* Failures in locking and unlocking are bad.  There are only a few
 * reasons why a lock operation would fail, but all of them indicate
 * programming errors.  Don't attempt to log anything--the
 * opportunities for deadlock are just too available.  */
int syscalls_pthread_mutex_lock(pthread_mutex_t *mutex) {
	int err;

	if ((err = pthread_mutex_lock(mutex))) {
		printf("Error locking mutex: %s\n", strerror(err));
		abort();
	}

	return err;
}

int syscalls_pthread_mutex_trylock(pthread_mutex_t *mutex) {
	int err;

	if ((err = pthread_mutex_trylock(mutex))) {
		switch (err) {
		case EBUSY:
			break;
		default:
			abort();
			break;

		} /* switch */
	}
	return err;
}

int syscalls_pthread_mutex_unlock(pthread_mutex_t *mutex) {
	int err;

	if ((err = pthread_mutex_unlock(mutex))) {
		printf("Error unlocking mutex: %s\n", strerror(err));

		abort();
	}

	return err;
}

int syscalls_pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
	int err;

	if ((err = pthread_rwlock_wrlock(rwlock))) {
		switch (err) {
		case EBUSY:
			break;
		default:
			abort();
			break;
		}
	}

	return err;
}

int syscalls_pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
	int err;

	if ((err = pthread_rwlock_rdlock(rwlock))) {
		abort();
	}

	return err;
}

int syscalls_pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
	int err;

	if ((err = pthread_rwlock_unlock(rwlock))) {
		abort();
	}

	return err;
}

int syscalls_pthread_cond_wait(pthread_cond_t *cond,
			       pthread_mutex_t *mutex) {
	int err;

	if ((err = pthread_cond_wait(cond, mutex))) {
		EMRG("Error invoking pthread_cond_wait "
		     "(cond: %p mutex: %p): %s\n", (void *)cond, (void *)mutex, strerror(err));
		abort();
	}

	return err;
}

int syscalls_pthread_cond_signal(pthread_cond_t *cond) {
	int ret;

	if ((ret = pthread_cond_signal(cond))) {
		EMRG("Error invoking pthread_cond_signal "
		     "(cond: %p): %s\n", (void *)cond, strerror(ret));
		abort();
	}

	return ret;
}

int syscalls_pthread_join(pthread_t thread, void **value_ptr) {
	int ret;

	if ((ret = pthread_join(thread, value_ptr))) {
		ERRR("Failed to join thread: %s\n", strerror(ret));
	}

	return ret;
}


int syscalls_open(const char *pathname, int flags)
{
	int ret;

	if (!pathname) {
		ERRR("No pathname specified to open\n");
		return UTILITY_FAILURE;
	}

	DEBG("Attempting to open \"%s\" with flags 0x%x\n", pathname, flags);

	if ((ret = open(pathname, flags)) < 0) {
		ERRR("Failed to open \"%s\" with flags 0x%x: %s\n",
		     pathname, flags, strerror(ret));
	}

	INFO("Opened \"%s\" with flags 0x%x successfully (fd: %d)\n",
	     pathname, flags, ret);

	return ret;
}


int syscalls_close(int fd)
{
	int ret;

	ret = close(fd);

	DEBG("Attempting to close fd %d\n", fd);
	if (ret == -1) {
		ERRR("Close failed: %s (fd: %d)\n", strerror(errno), fd);
	} else {
		INFO("Close succeeded (fd: %d)\n", fd);
	}

	return ret;
}
