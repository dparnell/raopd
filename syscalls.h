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
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>

int syscalls_open(const char *pathname, int flags);
int syscalls_close(int fd);

int syscalls_vsnprintf(char *str, size_t size, const char *format, va_list args);
int syscalls_snprintf(char *str, size_t size, const char *format, ...);
char *syscalls_strncpy(char *dest, const char *src, size_t n);
int syscalls_socket(int domain, int type, int protocol);
int syscalls_inet_pton(int af, const char *src, void *dst);
int syscalls_connect(int sockfd,
		     const struct sockaddr *serv_addr,
		     socklen_t addrlen);
ssize_t syscalls_read(int fd, void *buf, size_t count);
ssize_t syscalls_write(int fd, const void *buf, size_t count);
unsigned int syscalls_sleep(unsigned int seconds);
void *syscalls_malloc(size_t size);
void syscalls_free(void *ptr);
void *syscalls_memset(void *s, int c, size_t n);
size_t syscalls_strlen(const char *s);

int syscalls_pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
int syscalls_pthread_create(pthread_t  *thread, pthread_attr_t *attr, void *
			    (*start_routine)(void *), void * arg);
int syscalls_pthread_detach(pthread_t  thread);
int syscalls_pthread_mutex_init(pthread_mutex_t *restrict mutex,
				const pthread_mutexattr_t *restrict attr);
int syscalls_pthread_rwlock_init(pthread_rwlock_t *rwlock,
				 const pthread_rwlockattr_t *attr);
int syscalls_pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int syscalls_pthread_mutex_trylock(pthread_mutex_t *mutex);
int syscalls_pthread_mutex_lock(pthread_mutex_t *mutex);
int syscalls_pthread_mutex_unlock(pthread_mutex_t *mutex);

int syscalls_pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int syscalls_pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int syscalls_pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int syscalls_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int syscalls_pthread_cond_signal(pthread_cond_t *cond);
int syscalls_pthread_join(pthread_t thread, void **value_ptr);

/* These defines are here so we know we've examined the error cases &
 * determined that the standard syscall needs no additional error
 * checking or logging. */

#define syscalls_printf printf
#define syscalls_umask umask
#define syscalls_memcpy memcpy
#define syscalls_setsid setsid
#define syscalls_strndup strndup
#define syscalls_strdup strdup
#define syscalls_strncasecmp strncasecmp
#define syscalls_strncmp strncmp
#define syscalls_strcmp strcmp
#define syscalls_strncat strncat
#define syscalls_strtoul strtoul
#define syscalls_strchr strchr
#define syscalls_strrchr strrchr
#define syscalls_strstr strstr
#define syscalls_index index
#define syscalls_getpid getpid
#define syscalls_fork fork
#define syscalls_fflush fflush

#define syscalls_htonl htonl
#define syscalls_htons htons
#define syscalls_ntohl ntohl
#define syscalls_ntohs ntohs

#define syscalls_abort abort
#define syscalls_gettimeofday gettimeofday

#endif /* #ifndef SYSCALLS_H */
