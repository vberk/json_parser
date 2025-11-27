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

#include "platform.h"



#ifdef WIN32

//
//  Basic functions
//


//  Ignores 'tzp'
int gettimeofday(struct timeval *tp, void *tzp)
{
    struct _timeb t;
    _ftime_s(&t);
    (*tp).tv_sec=(long)(time_t) t.time;   //  In UTC, timezone ignored
    (*tp).tv_usec=((long)t.millitm)*1000;
    return(0);
}


//  Ignores 'rem'
int nanosleep(const struct timespec *req, struct timespec *rem)
{
    DWORD dwMilliseconds=(DWORD)(*req).tv_sec*1000;
    dwMilliseconds+=(DWORD)((*req).tv_nsec/1000000);
    Sleep(dwMilliseconds);
    return(0);
}

long random(void)
{
    return(rand());
}

void srandom(unsigned int seed)
{
    srand(seed);
    return;
}

//  Not exactly atomic, there's a race condition here:
int rand_r(unsigned int *seedp)
{
    int r;
    srand(*seedp);
    r=rand();
    (*seedp)=(unsigned int) r;
    return(r);
}



/**************************************************************/
/*   Posix Threads                                            */
/**************************************************************/


//
//  Thread creation and joining
//
DWORD WINAPI BEDROCK_threadWrapper(LPVOID d)
{
    pthread_t *thread=(pthread_t*)d;
    (*thread).rc=NULL;     //  Ensures it is 'NULL' for the running duration of the thread.
    (*thread).rc=(*thread).start_routine((*thread).arg);
    return(0);
}



int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    //  Parameters to mirror unix:
    (*thread).start_routine=start_routine;
    (*thread).arg=arg;
    if (attr)
        (*thread).attr=*attr;

    //  Launch:
    (*thread).th=CreateThread(
        NULL,                   //  Security attribs
        0,                      //  Default stack size
        (LPTHREAD_START_ROUTINE) BEDROCK_threadWrapper,     //  This calls the actual start_routine
        (LPVOID) thread,        //  This contains the thread handle needed for joining
        0,                      //  Falgs -- should be taken from 'attr'
        &((*thread).tid));      //  The ID, needed for joining.

    //  Success?
    if ((*thread).th == INVALID_HANDLE_VALUE)
    {
        CloseHandle((*thread).th);
        return(-1); 
    }
    return(0);
}



int pthread_join(pthread_t thread, void **value_ptr)
{
    DWORD rc;
    (*value_ptr)=NULL;
    rc=WaitForSingleObject(thread.th, INFINITE);
    if (rc!=WAIT_OBJECT_0)
        return(-1);

    CloseHandle(thread.th);
    (*value_ptr)=thread.rc;
    return(0);
}


//
//  Mutex variables
//
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    InitializeCriticalSection(mutex);
    return(0);
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    EnterCriticalSection(mutex);
    return(0);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);
    return(0);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    DeleteCriticalSection(mutex);
    return(0);
}





//
//  Condition variables
//
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    (*cond).evth=CreateEvent(NULL, FALSE, FALSE, NULL);
    ResetEvent((*cond).evth);
    return(0);
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    CloseHandle((*cond).evth);
    return(0);
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    pthread_mutex_unlock(mutex);
    WaitForSingleObject((*cond).evth, INFINITE);
    pthread_mutex_lock(mutex);
    return(0);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    DWORD rc;
    DWORD msecs;
    struct timeval tv;
    
    //  Figure out the milliseconds of wait time:
    gettimeofday(&tv, NULL);
    msecs=(DWORD)(((*abstime).tv_sec-tv.tv_sec)*1000) + (((*abstime).tv_nsec/1000-tv.tv_usec)/1000);

    //  Like the regular wait:
    pthread_mutex_unlock(mutex);
    rc=WaitForSingleObject((*cond).evth, msecs);
    pthread_mutex_lock(mutex);
    if (rc==WAIT_TIMEOUT)
        return(ETIMEDOUT);

    return(0);
}


int pthread_cond_signal(pthread_cond_t *cond)
{
    SetEvent((*cond).evth);
    Sleep(0);
    return(0);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    return(pthread_cond_signal(cond));
}





/**************************************************************/
/*   Signal Handling                                          */
/**************************************************************/

int sigemptyset(sigset_t *set)
{
    (*set)=0;
    return(0);
}

int sigfillset(sigset_t *set)
{
    (*set)=0xFFFFFFFFFFFFFFFFll;
    return(0);
}

int sigaddset(sigset_t *set, int signum)
{
    sigset_t m=0x1;
    if (signum>64) return(-1);
    m<<=signum;
    (*set)|=m;
    return(0);
}

int sigdelset(sigset_t *set, int signum)
{
    sigset_t m=0x1;
    if (signum>64) return(-1);
    m<<=signum;
    (*set)&=~m;
    return(0);
}

int sigismember(sigset_t *set, int signum)
{
    sigset_t m=0x1;
    if (signum>64) return(-1);
    m<<=signum;
    if ((*set)&&m) return(1);
    return(0);
}

int sigaction(int signum, struct sigaction *act, struct sigaction *oldact)
{
    if (oldact) (*oldact).sa_handler=NULL;
    sigaddset(&(*act).sa_mask, signum);
    switch(signum)
    {
        case SIGABRT:
        case SIGFPE:
        case SIGILL:
        case SIGINT:
        case SIGSEGV:
        case SIGTERM:
            signal(signum, (*act).sa_handler);
            break;
        default:
            return(-1);
    }
    return(0);
}



#endif






