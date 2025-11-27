/*
 *  Copyright (c) 2024 by Vincent H. Berk
 *  All rights reserved.
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met: 
 * 
 *  1. Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer. 
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution. 
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

//
//  Common file for threading, sockets, etc.  Works Windows, GNU, Darwin, ...
//  This is tailored to how I use systems, including posix threads and BSD sockets.
//



#ifndef __BEDROCK_PLATFORM_H
#define __BEDROCK_PLATFORM_H



//  Common to all platforms:
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>



//
//  Some platform specific definitions:
//
#ifdef _WIN32

    //  
    //  Compiling with winsock2 requires this library
    //  before inclusin of windows.h
    //
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")
#define socklen_t int


    //  
    //  Common includes:
    //
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <sys/timeb.h>




    //  
    //  Basic missing datatypes:
    //
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef unsigned long long u_int64_t;

/*
struct timeval
{
    time_t tv_sec;
    long tv_usec;
};

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};
*/


    //
    //  Basic missing functions:
    //
int gettimeofday(struct timeval *tp, void *tzp);        //  Note:  'tzp' is ignored
int nanosleep(const struct timespec *req, struct timespec *rem);    //  Milliseconds is best windows will give
long random(void);
void srandom(unsigned int seed);
int rand_r(unsigned int *seedp);

    //  
    //  Posix threads are missing on windows:
    //
#define pthread_attr_t u_int64_t          //  Attributes are ignored on Windows
#define pthread_mutex_t CRITICAL_SECTION
#define pthread_mutexattr_t u_int64_t     //  Ignored
#define pthread_condattr_t  u_int64_t     //  Ignored

typedef struct
{
    HANDLE th;
    DWORD tid;
    pthread_attr_t attr;
    void *(*start_routine)(void *);         //  The thread code;
    void *arg;      //  The arguments given in thread start
    void *rc;       //  Return pointer of thread (NULL during running)
}
pthread_t;

typedef struct
{
    HANDLE evth;
}
pthread_cond_t;

//  Create and join:
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **value_ptr);

//  Mutexes:
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

//  Condition variables
//  The associated mutex MUST be locked before waiting for the condition:
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);



    //
    //  Signal handling
    //
#define sigset_t u_int64_t

//  These don't have any meaning in Windows:
#define SA_NOCLDSTOP    0x01
#define SA_NOCLDWAIT    0x02
#define SA_SIGINFO      0x04
#define SA_ONSTACK      0x08
#define SA_RESTART      0x10
#define SA_NODEFER      0x20
#define SA_RESETHAND    0x40

//  Technically windows was an (int, int) for sa_handler:
struct sigaction
{
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;               // ignored
    void (*sa_restorer)(void);  // ignored
};

int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
int sigismember(sigset_t *set, int signum);
int sigaction(int signum, struct sigaction *act, struct sigaction *oldact);








#else

    //  
    //  Common includes:
    //
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>


#endif







#endif
