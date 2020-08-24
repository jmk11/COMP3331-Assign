#ifndef PROJECT_HELPERS_H
#define PROJECT_HELPERS_H


#include <stdio.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <sys/timerfd.h>


//#include "constants.h"

#define GETTIME(ts) Clock_gettime(CLOCK_BOOTTIME, ts) // BAD??
// probably just better as a function
void getTime(struct timespec *tp);

#define MIN(a,b) ((a) < (b) ? (a) : (b))

unsigned short getPort(const char *string);

int Socket(int domain, int type, int protocol);
void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void GetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
void Close(int fd);

FILE *Fopen(const char *pathname, const char *mode);
char *Fgets(char *s, int size, FILE *stream);
void Clock_gettime(clockid_t clk_id, struct timespec *tp);
void *Malloc(size_t size);

//int strlcat2(char *dst, const char *src, int maxSize, unsigned int *curSize);
char strlcat2(char *dst, const char *src, unsigned int maxSize, unsigned int *curSize, unsigned int addSize);

void Sem_init(sem_t *sem, int pshared, unsigned int value);
// need more for sem

ssize_t Send(int sockfd, const void *buf, size_t len, int flags);

void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
void Pthread_join(pthread_t thread, void **retval);
void Pthread_mutex_lock(pthread_mutex_t *mutex);
void Pthread_mutex_unlock(pthread_mutex_t *mutex);
void Pthread_mutex_destroy(pthread_mutex_t *mutex);
void Pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);

void Pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);
void Pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
void Pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
void Pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
void Pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
void Pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *attr, int pref);

int Eventfd(unsigned int initval, int flags);
void Eventfd_read(int fd, eventfd_t *value);
void Eventfd_write(int fd, eventfd_t value);

void Sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

int Timerfd_create(int clockid, int flags);
void Timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

void Inet_pton(int af, const char *src, void *dst);

void *Reallocarray(void *ptr, size_t nmemb, size_t size);

unsigned long Lseek(int fd, off_t offset, int whence);

#endif //PROJECT_HELPERS_H
