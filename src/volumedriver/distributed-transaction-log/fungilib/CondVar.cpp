// Copyright (C) 2016 iNuron NV
//
// This file is part of Open vStorage Open Source Edition (OSE),
// as available from
//
//      http://www.openvstorage.org and
//      http://www.openvstorage.com.
//
// This file is free software; you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License v3 (GNU AGPLv3)
// as published by the Free Software Foundation, in version 3 as it comes in
// the LICENSE.txt file of the Open vStorage OSE distribution.
// Open vStorage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY of any kind.

#include <pthread.h>

#include "defines.h"
#include "CondVar.h"
#include "Mutex.h"
#include <youtils/IOException.h>

#include <sys/time.h>



#include <cstring>
#include <cerrno>

namespace fungi {

CondVar::CondVar(Mutex &mutex) :
	mutex_(mutex) {
	// cannot fail on Linux
	pthread_cond_init(&cond_, 0);
}

CondVar::~CondVar() {
	int res = pthread_cond_destroy(&cond_);
	if (res != 0) {
// #pragma warning ( push )
// #pragma warning ( disable: 4996 )
// 		LOG_WARN("Mutex::~Mutex pthread_mutex_destroy " << strerror(res));
// #pragma warning ( pop )
	}
}

void CondVar::signal() {
	mutex_.lock();
	pthread_cond_signal(&cond_);
	mutex_.unlock();
}

void CondVar::signal_no_lock() {
	pthread_cond_signal(&cond_);
}

void CondVar::lock_and_signal() {
	mutex_.lock();
	pthread_cond_signal(&cond_);
	mutex_.unlock();
}

bool CondVar::wait_sec(int sec) {
	struct timespec abstime;
	struct timeval tp;
	gettimeofday(&tp, 0);
	abstime.tv_sec = tp.tv_sec;
	abstime.tv_nsec = tp.tv_usec * 1000;
	abstime.tv_sec += sec;
	mutex_.lock();
	int res = pthread_cond_timedwait(&cond_, mutex_.handle(), &abstime);
	mutex_.unlock();
	if (res == ETIMEDOUT) {
		return true;
	}
	if (res != 0) {
		throw IOException("CondVar::wait_sec", "pthread_cond_timedwait", res);
	}
	return false;
}

namespace
{
const int64_t one_second = 1000000000LL;
}

bool
CondVar::wait_nsec_no_lock(uint64_t nsecs)
{
    struct timespec reltime;

    if (nsecs != 0)
    {
        reltime.tv_sec =  one_second / nsecs;
        reltime.tv_nsec = one_second % nsecs;
    }
    else
    {
        memset(&reltime, 0x0, sizeof(reltime));
    }

    struct timespec abstime;
    struct timeval tp;
    gettimeofday(&tp, 0);
    abstime.tv_sec = tp.tv_sec + reltime.tv_sec;
    abstime.tv_nsec = tp.tv_usec * 1000;

    if (abstime.tv_nsec + reltime.tv_nsec < one_second)
    {
        abstime.tv_nsec += reltime.tv_nsec;
    }
    else
    {
        abstime.tv_sec += 1;
        abstime.tv_nsec += one_second - reltime.tv_nsec;
    }

    int res = pthread_cond_timedwait(&cond_, mutex_.handle(), &abstime);
    if (res == ETIMEDOUT)
    {
        return true;
    }
    if (res != 0)
    {
        throw IOException("CondVar::wait_nsec_no_lock",
                          "pthread_cond_timedwait", res);
    }
    return false;
}

bool
CondVar::wait_sec_no_lock(int sec)
{
    uint64_t nsecs = sec * one_second;
    return wait_nsec_no_lock(nsecs);
}

//    void CondVar::wait() {
//        mutex_.lock();
//        int res = pthread_cond_wait(&cond_, mutex_.handle());
//        mutex_.unlock();
//        if (res != 0) {
//            throw IOException("CondVar::wait", "pthread_cond_wait", res);
//        }
//    }

void CondVar::wait_no_lock() {
// #ifdef LOCKING_DEBUG
// 	Interval i;
// 	i.start();
// #endif
	int res = pthread_cond_wait(&cond_, mutex_.handle());
	if (res != 0) {
		// TODO, we don't want to get into exceptional state while holding a lock...?
		throw IOException("CondVar::wait", "pthread_cond_wait", res);
	}
// #ifdef LOCKING_DEBUG
// 	i.stop();
// 	const double r = i.secondsDouble();
// 	if (r > 0.0001) {
// 		LOG_WARN("LOCKING_DEBUG [" << std::hex << pthread_self() << std::dec << "] waited " << i.secondsDouble() << " on mutex " << mutex_.getName());
// 	}
// #endif
}

}
