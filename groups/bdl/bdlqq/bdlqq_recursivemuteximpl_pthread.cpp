// bdlqq_recursivemuteximpl_pthread.cpp                               -*-C++-*-
#include <bdlqq_recursivemuteximpl_pthread.h>

#include <bsls_ident.h>
BSLS_IDENT_RCSID(bdlqq_recursivemuteximpl_pthread_cpp,"$Id$ $CSID$")

#ifdef BDLQQ_PLATFORM_POSIX_THREADS

namespace BloombergLP {

          // ------------------------------------------------
          // class RecursiveMutexImpl<Platform::PosixThreads>
          // ------------------------------------------------

// CREATORS
bdlqq::RecursiveMutexImpl<bdlqq::Platform::PosixThreads>::RecursiveMutexImpl()
: d_spin(bsls::SpinLock::s_unlocked)
{
    pthread_mutexattr_t attribute;
    pthread_mutexattr_init(&attribute);
#ifdef PTHREAD_MUTEX_RECURSIVE
    pthread_mutexattr_settype(&attribute,PTHREAD_MUTEX_RECURSIVE);
#else
    d_lockCount = 0;
#endif
    pthread_mutex_init(&d_lock, &attribute);
    pthread_mutexattr_destroy(&attribute);
}

// MANIPULATORS
void bdlqq::RecursiveMutexImpl<bdlqq::Platform::PosixThreads>::lock()
{
#ifdef PTHREAD_MUTEX_RECURSIVE
    pthread_mutex_lock(&d_lock);
#else
    if (pthread_mutex_trylock(&d_lock)) {
        d_spin.lock();
        if (d_lockCount && pthread_equal(d_owner,pthread_self())) {
            ++d_lockCount;
            d_spin.unlock();
            return;                                                   // RETURN
        }
        d_spin.unlock();
        pthread_mutex_lock(&d_lock);
    }
    d_spin.lock();
    d_owner = pthread_self();
    d_lockCount = 1;
    d_spin.unlock();
#endif
}

int bdlqq::RecursiveMutexImpl<bdlqq::Platform::PosixThreads>::tryLock()
{
#ifdef PTHREAD_MUTEX_RECURSIVE
    return pthread_mutex_trylock(&d_lock);                            // RETURN
#else
    if (pthread_mutex_trylock(&d_lock)) {
        d_spin.lock();
        if (d_lockCount && pthread_equal(d_owner,pthread_self())) {
            ++d_lockCount;
            d_spin.unlock();
            return 0;                                                 // RETURN
        }
        else {
            d_spin.unlock();
            return 1;                                                 // RETURN
        }
    }
    else {
        d_spin.lock();
        d_owner = pthread_self();
        d_lockCount = 1;
        d_spin.unlock();
        return 0;                                                     // RETURN
    }
#endif
}

void bdlqq::RecursiveMutexImpl<bdlqq::Platform::PosixThreads>::unlock()
{
#ifdef PTHREAD_MUTEX_RECURSIVE
    pthread_mutex_unlock(&d_lock);
#else
    d_spin.lock();
    if (!--d_lockCount) {
        d_spin.unlock();
        pthread_mutex_unlock(&d_lock);
        return;                                                       // RETURN
    }
    d_spin.unlock();
#endif
}

}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
