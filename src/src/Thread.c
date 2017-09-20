#include "oid_common.h"
//#include <vlc_atomic.h>

//#include "libvlc.h"
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <sys/types.h>
#include <unistd.h> /* fsync() */
#include <pthread.h>
#include <sched.h>


#include <sys/syscall.h> /* SYS_gettid */

static unsigned oid_clock_prec;
static clockid_t oid_clock_id;

static void oid_clock_setup_once (void)
{
    long val = sysconf (_SC_MONOTONIC_CLOCK);
    assert (val != 0);
    oid_clock_id = (val < 0) ? CLOCK_REALTIME : CLOCK_MONOTONIC;

    struct timespec res;
    if (unlikely(clock_getres (oid_clock_id, &res) != 0 || res.tv_sec != 0))
        abort ();
    oid_clock_prec = (res.tv_nsec + 500) / 1000;
}

static pthread_once_t oid_clock_once = PTHREAD_ONCE_INIT;

# define oid_clock_setup() \
    pthread_once(&oid_clock_once, oid_clock_setup_once)



static struct timespec mtime_to_ts (mtime_t date)
{
    lldiv_t d = lldiv (date, CLOCK_FREQ);
    struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };
    return ts;
}

/**
 * Print a backtrace to the standard error for debugging purpose.
 */
void oid_trace (const char *fn, const char *file, unsigned line)
{
     fprintf (stderr, "at %s:%u in %s\n", file, line, fn);
     fflush (stderr); /* needed before switch to low-level I/O */
     fsync (2);
}

static inline unsigned long oid_threadid (void)
{

     union { pthread_t th; unsigned long int i; } v = { };
     v.th = pthread_self ();
     return v.i;
}


/**
 * Initializes a fast mutex.
 */
void oid_mutex_init(oid_mutex_t *p_mutex )
{
    pthread_mutexattr_t attr;

    if (unlikely(pthread_mutexattr_init (&attr)))
        abort();
#ifdef NDEBUG
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_DEFAULT);
#else
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    if (unlikely(pthread_mutex_init (p_mutex, &attr)))
        abort();
    pthread_mutexattr_destroy( &attr );
}

/**
 * Initializes a recursive mutex.
 * \warning This is strongly discouraged. Please use normal mutexes.
 */
void oid_mutex_init_recursive( oid_mutex_t *p_mutex )
{
    pthread_mutexattr_t attr;

    if (unlikely(pthread_mutexattr_init (&attr)))
        abort();
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
    if (unlikely(pthread_mutex_init (p_mutex, &attr)))
        abort();
    pthread_mutexattr_destroy( &attr );
}


/**
 * Destroys a mutex. The mutex must not be locked.
 *
 * @param p_mutex mutex to destroy
 * @return always succeeds
 */
void oid_mutex_destroy (oid_mutex_t *p_mutex)
{
    int val = pthread_mutex_destroy( p_mutex );
}


/**
 * Acquires a mutex. If needed, waits for any other thread to release it.
 * Beware of deadlocks when locking multiple mutexes at the same time,
 * or when using mutexes from callbacks.
 * This function is not a cancellation-point.
 *
 * @param p_mutex mutex initialized with vlc_mutex_init() or
 *                vlc_mutex_init_recursive()
 */
void oid_mutex_lock (oid_mutex_t *p_mutex)
{
    int val = pthread_mutex_lock( p_mutex );
}

/**
 * Acquires a mutex if and only if it is not currently held by another thread.
 * This function never sleeps and can be used in delay-critical code paths.
 * This function is not a cancellation-point.
 *
 * <b>Beware</b>: If this function fails, then the mutex is held... by another
 * thread. The calling thread must deal with the error appropriately. That
 * typically implies postponing the operations that would have required the
 * mutex. If the thread cannot defer those operations, then it must use
 * vlc_mutex_lock(). If in doubt, use vlc_mutex_lock() instead.
 *
 * @param p_mutex mutex initialized with vlc_mutex_init() or
 *                vlc_mutex_init_recursive()
 * @return 0 if the mutex could be acquired, an error code otherwise.
 */
int oid_mutex_trylock (oid_mutex_t *p_mutex)
{
    int val = pthread_mutex_trylock( p_mutex );

   // if (val != EBUSY)
   //     VLC_THREAD_ASSERT ("locking mutex");
    return val;
}

/**
 * Releases a mutex (or crashes if the mutex is not locked by the caller).
 * @param p_mutex mutex locked with vlc_mutex_lock().
 */
void oid_mutex_unlock (oid_mutex_t *p_mutex)
{
    int val = pthread_mutex_unlock( p_mutex );
}

/**
 * Initializes a condition variable.
 */
void oid_cond_init (oid_cond_t *p_condvar)
{
    pthread_condattr_t attr;

    if (unlikely(pthread_condattr_init (&attr)))
        abort ();
//#if (_POSIX_CLOCK_SELECTION > 0)
    oid_clock_setup ();
    pthread_condattr_setclock (&attr, oid_clock_id);
//#endif
    if (unlikely(pthread_cond_init (p_condvar, &attr)))
        abort ();
    pthread_condattr_destroy (&attr);
}

/**
 * Initializes a condition variable.
 * Contrary to vlc_cond_init(), the wall clock will be used as a reference for
 * the vlc_cond_timedwait() time-out parameter.
 */
void oid_cond_init_daytime (oid_cond_t *p_condvar)
{
    if (unlikely(pthread_cond_init (p_condvar, NULL)))
        abort ();
}

/**
 * Destroys a condition variable. No threads shall be waiting or signaling the
 * condition.
 * @param p_condvar condition variable to destroy
 */
void oid_cond_destroy (oid_cond_t *p_condvar)
{
    int val = pthread_cond_destroy( p_condvar );
}

/**
 * Wakes up one thread waiting on a condition variable, if any.
 * @param p_condvar condition variable
 */
void oid_cond_signal (oid_cond_t *p_condvar)
{
    int val = pthread_cond_signal( p_condvar );
}

/**
 * Wakes up all threads (if any) waiting on a condition variable.
 * @param p_cond condition variable
 */
void oid_cond_broadcast (oid_cond_t *p_condvar)
{
    pthread_cond_broadcast (p_condvar);
}

/**
 * Waits for a condition variable. The calling thread will be suspended until
 * another thread calls vlc_cond_signal() or vlc_cond_broadcast() on the same
 * condition variable, the thread is cancelled with vlc_cancel(), or the
 * system causes a "spurious" unsolicited wake-up.
 *
 * A mutex is needed to wait on a condition variable. It must <b>not</b> be
 * a recursive mutex. Although it is possible to use the same mutex for
 * multiple condition, it is not valid to use different mutexes for the same
 * condition variable at the same time from different threads.
 *
 * In case of thread cancellation, the mutex is always locked before
 * cancellation proceeds.
 *
 * The canonical way to use a condition variable to wait for event foobar is:
 @code
   vlc_mutex_lock (&lock);
   mutex_cleanup_push (&lock); // release the mutex in case of cancellation

   while (!foobar)
       vlc_cond_wait (&wait, &lock);

   --- foobar is now true, do something about it here --

   vlc_cleanup_run (); // release the mutex
  @endcode
 *
 * @param p_condvar condition variable to wait on
 * @param p_mutex mutex which is unlocked while waiting,
 *                then locked again when waking up.
 * @param deadline <b>absolute</b> timeout
 */
void oid_cond_wait (oid_cond_t *p_condvar, oid_mutex_t *p_mutex)
{
    int val = pthread_cond_wait( p_condvar, p_mutex );
}

/**
 * Waits for a condition variable up to a certain date.
 * This works like vlc_cond_wait(), except for the additional time-out.
 *
 * If the variable was initialized with vlc_cond_init(), the timeout has the
 * same arbitrary origin as mdate(). If the variable was initialized with
 * vlc_cond_init_daytime(), the timeout is expressed from the Unix epoch.
 *
 * @param p_condvar condition variable to wait on
 * @param p_mutex mutex which is unlocked while waiting,
 *                then locked again when waking up.
 * @param deadline <b>absolute</b> timeout
 *
 * @return 0 if the condition was signaled, an error code in case of timeout.
 */
int oid_cond_timedwait (oid_cond_t *p_condvar, oid_mutex_t *p_mutex,
                        mtime_t deadline)
{
    struct timespec ts = mtime_to_ts (deadline);
    int val = pthread_cond_timedwait (p_condvar, p_mutex, &ts);
  //  if (val != ETIMEDOUT)
  //      VLC_THREAD_ASSERT ("timed-waiting on condition");
    return val;
}

/**
 * Initializes a semaphore.
 */
void oid_sem_init (oid_sem_t *sem, unsigned value)
{
    if (unlikely(sem_init (sem, 0, value)))
        abort ();
}

/**
 * Destroys a semaphore.
 */
void oid_sem_destroy (oid_sem_t *sem)
{
    int val;

    if (likely(sem_destroy (sem) == 0))
        return;

    val = errno;

}

/**
 * Increments the value of a semaphore.
 * @return 0 on success, EOVERFLOW in case of integer overflow
 */
int oid_sem_post (oid_sem_t *sem)
{
    int val;

    if (likely(sem_post (sem) == 0))
        return 0;

    val = errno;

    //if (unlikely(val != EOVERFLOW))
    //    VLC_THREAD_ASSERT ("unlocking semaphore");
    return val;
}

/**
 * Atomically wait for the semaphore to become non-zero (if needed),
 * then decrements it.
 */
void oid_sem_wait (oid_sem_t *sem)
{
    int val;

    do
        if (likely(sem_wait (sem) == 0))
            return;
    while ((val = errno) == EINTR);

}

/**
 * Initializes a read/write lock.
 */
void oid_rwlock_init (oid_rwlock_t *lock)
{
    if (unlikely(pthread_rwlock_init (lock, NULL)))
        abort ();
}

/**
 * Destroys an initialized unused read/write lock.
 */
void oid_rwlock_destroy (oid_rwlock_t *lock)
{
    int val = pthread_rwlock_destroy (lock);
}

/**
 * Acquires a read/write lock for reading. Recursion is allowed.
 * @note This function may be a point of cancellation.
 */
void oid_rwlock_rdlock (oid_rwlock_t *lock)
{
    int val = pthread_rwlock_rdlock (lock);
}

/**
 * Acquires a read/write lock for writing. Recursion is not allowed.
 * @note This function may be a point of cancellation.
 */
void oid_rwlock_wrlock (oid_rwlock_t *lock)
{
    int val = pthread_rwlock_wrlock (lock);
}

/**
 * Releases a read/write lock.
 */
void oid_rwlock_unlock (oid_rwlock_t *lock)
{
    int val = pthread_rwlock_unlock (lock);
}

/**
 * Allocates a thread-specific variable.
 * @param key where to store the thread-specific variable handle
 * @param destr a destruction callback. It is called whenever a thread exits
 * and the thread-specific variable has a non-NULL value.
 * @return 0 on success, a system error code otherwise. This function can
 * actually fail because there is a fixed limit on the number of
 * thread-specific variable in a process on most systems.
 */
int oid_threadvar_create (oid_threadvar_t *key, void (*destr) (void *))
{
    return pthread_key_create (key, destr);
}

void oid_threadvar_delete (oid_threadvar_t *p_tls)
{
    pthread_key_delete (*p_tls);
}

/**
 * Sets a thread-specific variable.
 * @param key thread-local variable key (created with vlc_threadvar_create())
 * @param value new value for the variable for the calling thread
 * @return 0 on success, a system error code otherwise.
 */
int oid_threadvar_set (oid_threadvar_t key, void *value)
{
    return pthread_setspecific (key, value);
}

/**
 * Gets the value of a thread-local variable for the calling thread.
 * This function cannot fail.
 * @return the value associated with the given variable for the calling
 * or NULL if there is no value.
 */
void *oid_threadvar_get (oid_threadvar_t key)
{
    return pthread_getspecific (key);
}

static bool rt_priorities = false;
static int rt_offset;
#if 0
void vlc_threads_setup (libvlc_int_t *p_libvlc)
{
    static vlc_mutex_t lock = VLC_STATIC_MUTEX;
    static bool initialized = false;

    vlc_mutex_lock (&lock);
    /* Initializes real-time priorities before any thread is created,
     * just once per process. */
    if (!initialized)
    {
        if (var_InheritBool (p_libvlc, "rt-priority"))
        {
            rt_offset = var_InheritInteger (p_libvlc, "rt-offset");
            rt_priorities = true;
        }
        initialized = true;
    }
    vlc_mutex_unlock (&lock);
}
#endif

static int oid_clone_attr (oid_thread_t *th, pthread_attr_t *attr,
                           void *(*entry) (void *), void *data, int priority)
{
    int ret;

    /* Block the signals that signals interface plugin handles.
     * If the LibVLC caller wants to handle some signals by itself, it should
     * block these before whenever invoking LibVLC. And it must obviously not
     * start the VLC signals interface plugin.
     *
     * LibVLC will normally ignore any interruption caused by an asynchronous
     * signal during a system call. But there may well be some buggy cases
     * where it fails to handle EINTR (bug reports welcome). Some underlying
     * libraries might also not handle EINTR properly.
     */
    sigset_t oldset;
    {
        sigset_t set;
        sigemptyset (&set);
        sigdelset (&set, SIGHUP);
        sigaddset (&set, SIGINT);
        sigaddset (&set, SIGQUIT);
        sigaddset (&set, SIGTERM);

        sigaddset (&set, SIGPIPE); /* We don't want this one, really! */
        pthread_sigmask (SIG_BLOCK, &set, &oldset);
    }

//#if defined (_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING >= 0) \
// && defined (_POSIX_THREAD_PRIORITY_SCHEDULING) \
// && (_POSIX_THREAD_PRIORITY_SCHEDULING >= 0)
  
   if (rt_priorities)
    {
        struct sched_param sp = { .sched_priority = priority + rt_offset, };
        int policy;

        if (sp.sched_priority <= 0)
            sp.sched_priority += sched_get_priority_max (policy = SCHED_OTHER);
        else
            sp.sched_priority += sched_get_priority_min (policy = SCHED_RR);

        pthread_attr_setschedpolicy (attr, policy);
        pthread_attr_setschedparam (attr, &sp);
    }
//#else
//    (void) priority;
//#endif

    /* The thread stack size.
     * The lower the value, the less address space per thread, the highest
     * maximum simultaneous threads per process. Too low values will cause
     * stack overflows and weird crashes. Set with caution. Also keep in mind
     * that 64-bits platforms consume more stack than 32-bits one.
     *
     * Thanks to on-demand paging, thread stack size only affects address space
     * consumption. In terms of memory, threads only use what they need
     * (rounded up to the page boundary).
     *
     * For example, on Linux i386, the default is 2 mega-bytes, which supports
     * about 320 threads per processes. */
#define OID_STACKSIZE (128 * sizeof (void *) * 1024)

#ifdef OID_STACKSIZE
    ret = pthread_attr_setstacksize (attr, OID_STACKSIZE);
    assert (ret == 0); /* fails iif VLC_STACKSIZE is invalid */
#endif

    ret = pthread_create (th, attr, entry, data);
    pthread_sigmask (SIG_SETMASK, &oldset, NULL);
    pthread_attr_destroy (attr);
    return ret;
}

/**
 * Creates and starts new thread.
 *
 * The thread must be <i>joined</i> with vlc_join() to reclaim resources
 * when it is not needed anymore.
 *
 * @param th [OUT] pointer to write the handle of the created thread to
 *                 (mandatory, must be non-NULL)
 * @param entry entry point for the thread
 * @param data data parameter given to the entry point
 * @param priority thread priority value
 * @return 0 on success, a standard error code on error.
 */
int oid_clone (oid_thread_t *th, void *(*entry) (void *), void *data,
               int priority)
{
    pthread_attr_t attr;

    pthread_attr_init (&attr);
    return oid_clone_attr (th, &attr, entry, data, priority);
}

/**
 * Waits for a thread to complete (if needed), then destroys it.
 * This is a cancellation point; in case of cancellation, the join does _not_
 * occur.
 * @warning
 * A thread cannot join itself (normally VLC will abort if this is attempted).
 * Also, a detached thread <b>cannot</b> be joined.
 *
 * @param handle thread handle
 * @param p_result [OUT] pointer to write the thread return value or NULL
 */
void oid_join (oid_thread_t handle, void **result)
{
    int val = pthread_join (handle, result);
}

/**
 * Creates and starts new detached thread.
 * A detached thread cannot be joined. Its resources will be automatically
 * released whenever the thread exits (in particular, its call stack will be
 * reclaimed).
 *
 * Detached thread are particularly useful when some work needs to be done
 * asynchronously, that is likely to be completed much earlier than the thread
 * can practically be joined. In this case, thread detach can spare memory.
 *
 * A detached thread may be cancelled, so as to expedite its termination.
 * Be extremely careful if you do this: while a normal joinable thread can
 * safely be cancelled after it has already exited, cancelling an already
 * exited detached thread is undefined: The thread handle would is destroyed
 * immediately when the detached thread exits. So you need to ensure that the
 * detached thread is still running before cancellation is attempted.
 *
 * @warning Care must be taken that any resources used by the detached thread
 * remains valid until the thread completes.
 *
 * @note A detached thread must eventually exit just like another other
 * thread. In practice, LibVLC will wait for detached threads to exit before
 * it unloads the plugins.
 *
 * @param th [OUT] pointer to hold the thread handle, or NULL
 * @param entry entry point for the thread
 * @param data data parameter given to the entry point
 * @param priority thread priority value
 * @return 0 on success, a standard error code on error.
 */
int oid_clone_detach (oid_thread_t *th, void *(*entry) (void *), void *data,
                      int priority)
{
    oid_thread_t dummy;
    pthread_attr_t attr;

    if (th == NULL)
        th = &dummy;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    return oid_clone_attr (th, &attr, entry, data, priority);
}

int oid_set_priority (oid_thread_t th, int priority)
{
//#if defined (_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING >= 0) \
// && defined (_POSIX_THREAD_PRIORITY_SCHEDULING) \
// && (_POSIX_THREAD_PRIORITY_SCHEDULING >= 0)
    if (rt_priorities)
    {
        struct sched_param sp = { .sched_priority = priority + rt_offset, };
        int policy;

        if (sp.sched_priority <= 0)
            sp.sched_priority += sched_get_priority_max (policy = SCHED_OTHER);
        else
            sp.sched_priority += sched_get_priority_min (policy = SCHED_RR);

        if (pthread_setschedparam (th, policy, &sp))
            return OID_EGENERIC;
    }
//#else
//    (void) th; (void) priority;
//#endif
    return OID_SUCCESS;
}

/**
 * Marks a thread as cancelled. Next time the target thread reaches a
 * cancellation point (while not having disabled cancellation), it will
 * run its cancellation cleanup handler, the thread variable destructors, and
 * terminate. vlc_join() must be used afterward regardless of a thread being
 * cancelled or not.
 */
void oid_cancel (oid_thread_t thread_id)
{
    pthread_cancel (thread_id);
}

/**
 * Save the current cancellation state (enabled or disabled), then disable
 * cancellation for the calling thread.
 * This function must be called before entering a piece of code that is not
 * cancellation-safe, unless it can be proven that the calling thread will not
 * be cancelled.
 * @return Previous cancellation state (opaque value for vlc_restorecancel()).
 */
int oid_savecancel (void)
{
    int state;
    int val = pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &state);

    return state;
}

/**
 * Restore the cancellation state for the calling thread.
 * @param state previous state as returned by vlc_savecancel().
 * @return Nothing, always succeeds.
 */
void oid_restorecancel (int state)
{
//#ifndef NDEBUG
//    int oldstate, val;
//
//    val = pthread_setcancelstate (state, &oldstate);
//    /* This should fail if an invalid value for given for state */
//    VLC_THREAD_ASSERT ("restoring cancellation");
//
//    if (unlikely(oldstate != PTHREAD_CANCEL_DISABLE))
//         vlc_thread_fatal ("restoring cancellation while not disabled", EINVAL,
//                           __func__, __FILE__, __LINE__);
//#else
    pthread_setcancelstate (state, NULL);
//#endif
}

/**
 * Issues an explicit deferred cancellation point.
 * This has no effect if thread cancellation is disabled.
 * This can be called when there is a rather slow non-sleeping operation.
 * This is also used to force a cancellation point in a function that would
 * otherwise "not always" be a one (block_FifoGet() is an example).
 */
void oid_testcancel (void)
{
    pthread_testcancel ();
}

void oid_control_cancel (int cmd, ...)
{
    (void) cmd;
    assert (0);
}

/**
 * Precision monotonic clock.
 *
 * In principles, the clock has a precision of 1 MHz. But the actual resolution
 * may be much lower, especially when it comes to sleeping with mwait() or
 * msleep(). Most general-purpose operating systems provide a resolution of
 * only 100 to 1000 Hz.
 *
 * @warning The origin date (time value "zero") is not specified. It is
 * typically the time the kernel started, but this is platform-dependent.
 * If you need wall clock time, use gettimeofday() instead.
 *
 * @return a timestamp in microseconds.
 */
mtime_t mdate (void)
{
//#if (_POSIX_TIMERS > 0)
//    struct timespec ts;
//
//    vlc_clock_setup ();
//    if (unlikely(clock_gettime (vlc_clock_id, &ts) != 0))
//        abort ();
//
//    return (INT64_C(1000000) * ts.tv_sec) + (ts.tv_nsec / 1000);

//#else
    struct timeval tv;

    if (unlikely(gettimeofday (&tv, NULL) != 0))
        abort ();
    return (INT64_C(1000000) * tv.tv_sec) + tv.tv_usec;

//#endif
}

#undef mwait
/**
 * Waits until a deadline (possibly later due to OS scheduling).
 * @param deadline timestamp to wait for (see mdate())
 */
void mwait (mtime_t deadline)
{
#if 0
//#if (_POSIX_CLOCK_SELECTION > 0)
    oid_clock_setup ();
    /* If the deadline is already elapsed, or within the clock precision,
     * do not even bother the system timer. */
    deadline -= oid_clock_prec;

    struct timespec ts = mtime_to_ts (deadline);

    while (clock_nanosleep (oid_clock_id, TIMER_ABSTIME, &ts, NULL) == EINTR);
#else
//#else
    deadline -= mdate ();
    if (deadline > 0)
        msleep (deadline);

//#endif
#endif

}

#undef msleep
/**
 * Waits for an interval of time.
 * @param delay how long to wait (in microseconds)
 */
void msleep (mtime_t delay)
{
    struct timespec ts = mtime_to_ts (delay);

#if 0
//#if (_POSIX_CLOCK_SELECTION > 0)
    oid_clock_setup ();
    while (clock_nanosleep (oid_clock_id, 0, &ts, &ts) == EINTR);

//    printf("33333333333333333333\r\n");
//#else
#else
    while (nanosleep (&ts, &ts) == -1)
        assert (errno == EINTR);

//#endif

#endif
}


/**
 * Count CPUs.
 * @return number of available (logical) CPUs.
 */
unsigned vlc_GetCPUCount(void)
{
//    cpu_set_t cpu;
//
//    CPU_ZERO(&cpu);
//    if (sched_getaffinity (0, sizeof (cpu), &cpu) < 0)
//        return 1;

//    return CPU_COUNT (&cpu);

    return 1;
}



