/*
Wrappers around basic libc functions and syscalls, enforcing success
*/

#include <error.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
//#include <sys/eventfd.h>
//#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
//#include <assert.h>
#include <arpa/inet.h>
#include <malloc.h>
//#include <sys/timerfd.h>

#include "constants.h"
#include "helpers.h"

// not sure this fits with what helpers.c has become
unsigned short getPort(const char *string) {
    long lport = strtol(string, NULL, 0);
    if (lport > MAXPORT || lport < 0) {
        printf("Provided port number not in range. Legal port numbers 0..%d", MAXPORT);
        exit(ARGSERROR);
    }
    return (unsigned short)lport;
}

int Socket(int domain, int type, int protocol) {
    int sockfd = socket(domain, type, protocol); // -1 on failure
    if (sockfd < 0) {
        perror("Error: could not open socket");
        exit(BUILDSOCKETERROR);
    }
    return sockfd;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int success = bind(sockfd, addr, addrlen);
    if (success < 0) {
        perror("Error: could not bind socket");
        exit(BUILDSOCKETERROR);
    }
}

void Listen(int sockfd, int backlog) {

    int success = listen(sockfd, backlog);
    if (success < 0) {
        perror("Error: Could not open socket for listening");
        exit(BUILDSOCKETERROR);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int newSockfd = accept(sockfd, addr, addrlen);
    if (newSockfd < 0) {
        perror("Error: Could not open a socket to accept data");
        exit(BUILDSOCKETERROR);
    }
    return newSockfd;
}

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int success = connect(sockfd, addr, addrlen);
    if (success < 0) {
        perror("Error: Could not connect to server");
        exit(BUILDSOCKETERROR);
    }
}

// mainly here because I didn't want to #include in client.c
void Close(int fd) {
    int success = close(fd);
    if (success < 0) {
        perror("Error: Could not close socket");
        exit(CLOSESOCKETERROR);
    }
}

FILE *Fopen(const char *pathname, const char *mode) {
    FILE *fp = fopen(pathname, mode);
    if (fp == NULL) {
        char errorMessage[100];
        snprintf(errorMessage, 100, "Error: Can't open file %s", pathname);
        perror(errorMessage); // dont' want to compile this info into client.
        exit(FILERROR);
    }
    return fp;
}

char *Fgets(char *s, int size, FILE *stream) {
    char *res = fgets(s, size, stream);
    if (res == NULL) {
        return NULL;
    } // !! deal with this somewhere
    //password[strlen(password)-1] = 0;
    char *end = strchr(res, '\r');
    if (end != NULL) {
        *end = 0;
    }
    end = strchr(res, '\n');
    if (end != NULL) {
        *end = 0;
    }
    return res;
}

void Clock_gettime(clockid_t clk_id, struct timespec *tp) {
    int success = clock_gettime(clk_id, tp);
    if (success < 0) {
        perror("Error: can't get time");
        exit(CLOCKERROR);
    }
}

// to standardise clk_id
// now using CLKID constant instead?
// since there were time I had to use clkid different to this
void getTime(struct timespec *tp) {
    int success = clock_gettime(CLOCK_BOOTTIME, tp);
    if (success < 0) {
        perror("Error: can't get time");
        exit(CLOCKERROR);
    }
}

void *Malloc(size_t size) {
    void *mem = malloc(size);
    if (mem == NULL) {
        printf("Error: malloc memory unavailable, or 0 bytes requested\n");
        exit(MALLOCERROR);
    }
    memset(mem, 0, size);
    return mem;
}

void *Reallocarray(void *ptr, size_t nmemb, size_t size) {
    void *mem = reallocarray(ptr, nmemb, size);
    if (mem == NULL) {
        printf("Error: realloc memory unavailable, or 0 bytes requested, or multiplication would overflow\n");
        exit(MALLOCERROR);
    }
    return mem;
}

// THIS COMMENT IS FOR AN OLD VERSION
// Always null-terminates UNLESS 2
// Final string size will be AT MOST n bytes, INCLUDING null byte
// NAME
/*
 * This description is old
 * Starts writing at dst+curSize
 * Idea is that the current null byte is at dst+curSize
 * Copies from src until src is null terminated or address (dst+maxSize-1) is reached, whichever is sooner
 * Null terminate at dst+maxSize-1
 * Will always null terminate dst (unless (2)) and sets curSize to the total length of dst, not including the null byte
 * curSize will equal the given curSize + the number of bytes written, excluding terminating null byte
 * addSize, if not 0, is the number of bytes to add. If 0, will add until null byte is found (or run out of space etc.)
 * RETURNS:
 * 0 if all goes correctly
 * 1 if there was not enough space for src - ie copying was terminated before src's null byte was found, or before addSize was exhausted
 * 2 if the provided curSize is greater than or equal to the provided maxSize. In this case, no changes were made to any provided memory.
 */
char strlcat2(char *dst, const char *src, unsigned int maxSize, unsigned int *curSize, unsigned int addSize) {
    if (*curSize >= maxSize) {
        return 2;
    }

    /*bool maxSizeChanged = FALSE; // this is stupid
    if (addSize != 0 && (*curSize + addSize + 1 <= maxSize)) {
        maxSize = *curSize + addSize + 1; // +1 because maxSize includes null byte while others do not.
        maxSizeChanged = TRUE;
    }*/

    //if (addSize == 0) { addSize = maxSize - *curSize - 1; }
    //if (addSize == 0) { addSize = maxSize; } // makes very big... takes out of equation... seems bad
    // I should be able to change this somehow so only need addSize in if
    // and prob something else to check if terminated copying because of addSize or because of original maxSize ...

    // !! test this bad new stuff
    char *start = dst + *curSize;
    char *cur = start; //dst + *curSize;
    while ((cur < dst + maxSize - 1) && ((addSize == 0 && *src != 0) || (addSize != 0 && cur < start + addSize))) {
        //while ((*src != 0) && (cur < dst + (maxSize - 1)) && !(addSize != 0 && cur >= start + addSize)) { // or do as: if curSize < maxSize - 1
        *(cur++) = *(src++);
        (*curSize)++;
    }
    *cur = 0;
    //*curSize = cur - dst - 1;
    //if (*src != 0 && (cur != dst + (maxSize - 1))) {
    if (cur == dst + (maxSize - 1) && !(addSize != 0 && cur == start + addSize)) {
        // copying ended only because reached maxSize
        return 1;
    }

    return 0;
}

void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    int success = pthread_create(thread, attr, start_routine, arg);
    if (success != 0) {
        error(0, success, "Error: Can't create thread");
        // It seems clearer to call exit() myself rather than with error, that's why I passed 0 to error
        // to make error()'s performance closer to perror. Because I'm only using error
        // instead of perror because it seems pthread_create returns the error number
        // rather than putting it in errno
        exit(THREADERROR);
    }
}

void Sem_init(sem_t *sem, int pshared, unsigned int value) {
    int success = sem_init(sem, pshared, value);
    if (success != 0) {
        perror("Error: Can't initialise semaphore");
        exit(SEMAPHOREERROR);
    }
}

ssize_t Send(int sockfd, const void *buf, size_t len, int flags) {
    ssize_t result = send(sockfd, buf, len, flags);
    if (result < 0) {
        perror("Error: When sending data");
        exit(SENDERROR);
    }
    return result;
}

void Pthread_join(pthread_t thread, void **retval) {
    int success = pthread_join(thread, retval);
    if (success != 0) {
        error(0, success, "Error: Can't join to thread");
        exit(THREADERROR);
    }
}

void Pthread_mutex_lock(pthread_mutex_t *mutex) {
    int success = pthread_mutex_lock(mutex);
    if (success != 0) {
        error(0, success, "Error: Can't lock mutex");
        exit(THREADERROR);
    }
}

void Pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int success = pthread_mutex_unlock(mutex);
    if (success != 0) {
        error(0, success, "Error: Can't unlock mutex");
        exit(THREADERROR);
    }
}

void Pthread_mutex_destroy(pthread_mutex_t *mutex) {
    int success = pthread_mutex_destroy(mutex);
    if (success != 0) {
        error(0, success, "Error: Can't destroy mutex");
        exit(THREADERROR);
    }
}

void Pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr) {
    int success = pthread_mutex_init(mutex, attr);
    if (success != 0) {
        error(0, success, "Error: Can't initialise mutex");
        exit(THREADERROR);
    }
}

void Pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr) {
    int success = pthread_rwlock_init(rwlock, attr);
    if (success != 0) {
        error(0, success, "Error: Can't initialise rwlock");
        exit(THREADERROR);
    }
}

void Pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
    int success = pthread_rwlock_destroy(rwlock);
    if (success != 0) {
        error(0, success, "Error: Can't destroy rwlock");
        exit(THREADERROR);
    }
}

void Pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
    int success = pthread_rwlock_rdlock(rwlock);
    if (success != 0) {
        error(0, success, "Error: Can't readlock rwlock");
        exit(THREADERROR);
    }
}

void Pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
    int success = pthread_rwlock_wrlock(rwlock);
    if (success != 0) {
        error(0, success, "Error: Can't writelock rwlock");
        exit(THREADERROR);
    }
}

// what does this actually do its weird
void Pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *attr, int pref) {
    int success = pthread_rwlockattr_setkind_np(attr, pref);
    if (success != 0) {
        error(0, success, "Error: Can't set attribute on rwlock");
        exit(THREADERROR);
    }
}

// !! CHECK WHAT UNLOCK ACTUALLY RETURNS
void Pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
    int success = pthread_rwlock_unlock(rwlock);
    if (success != 0) {
        error(0, success, "Error: Can't unlock rwlock");
        exit(THREADERROR);
    }
}

int Eventfd(unsigned int initval, int flags) {
    int fd = eventfd(initval, flags);
    if (fd == -1) {
        perror("Error: Can't create eventfd");
        exit(EVENTFDERROR);
    }
    return fd;
}

void Eventfd_read(int fd, eventfd_t *value) {
    int success = eventfd_read(fd, value);
    if (success != 0) {
        fprintf(stderr, "Error: while reading eventfd\n");
        exit(EVENTFDERROR);
    }
}

void Eventfd_write(int fd, eventfd_t value) {
    int success = eventfd_write(fd, value);
    if (success != 0) {
        fprintf(stderr, "Error: while writing to eventfd\n");
        exit(EVENTFDERROR);
    }
}

void Sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    int success = sigaction(signum, act, oldact);
    if (success != 0) {
        perror("Error: in setting sighandler");
        exit(SIGHANDLERERROR);
    }
}

int Timerfd_create(int clockid, int flags) {
    int fd = timerfd_create(clockid, flags);
    if (fd < 0) {
        perror("Error: in creating timerfd");
        exit(TIMERFDERROR);
    }
    return fd;
}

void Timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) {
    int success = timerfd_settime(fd, flags, new_value, old_value);
    if (success != 0) {
        perror("Error: in setting timerfd");
        exit(TIMERFDERROR);
    }
}

/*
void Timerfd_gettime(int fd, struct itimerspec *curr_value)
{
    int success = sigaction(signum, act, oldact);
    if (success != 0) {
        perror("Error: in setting sighandler");
        exit(SIGHANDLERERROR);
    }
}*/

void GetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int success = getsockname(sockfd, addr, addrlen);
    if (success != 0) {
        perror("Error: in getting socket details");
        exit(BUILDSOCKETERROR);
    }
}

void Inet_pton(int af, const char *src, void *dst) {
    int success = inet_pton(af, src, dst);
    if (success != 1) {
        printf("Error: Provided HostIP is invalid.\n");
        exit(ARGSERROR);
    }
}

unsigned long Lseek(int fd, off_t offset, int whence) {
    long result = lseek(fd, offset, whence);
    if (result < 0) {
        perror("Error: in lseeking");
        exit(FILERROR);
    }
    return result;
}
