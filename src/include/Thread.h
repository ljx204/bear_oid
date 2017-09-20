#ifndef __THREAD_H__
#define __THREAD_H__


#include <unistd.h> /* _POSIX_SPIN_LOCKS */
#include <pthread.h>
#include <semaphore.h>

#define OID_USE_PTHREAD           1
#define OID_USE_PTHREAD_CLEANUP   1
#define OID_USE_PTHREAD_CANCEL    1
typedef int64_t mtime_t;

typedef pthread_t       	oid_thread_t;
typedef pthread_mutex_t 	oid_mutex_t;
#define OID_STATIC_MUTEX 	PTHREAD_MUTEX_INITIALIZER
typedef pthread_cond_t  	oid_cond_t;
#define OID_STATIC_COND  	PTHREAD_COND_INITIALIZER
typedef sem_t           	oid_sem_t;
typedef pthread_rwlock_t 	oid_rwlock_t;
#define OID_STATIC_RWLOCK 	PTHREAD_RWLOCK_INITIALIZER
typedef pthread_key_t   	oid_threadvar_t;
typedef struct oid_timer *	oid_timer_t;

/*****************************************************************************
 * Function definitions
 *****************************************************************************/
  void oid_mutex_init( oid_mutex_t * );
  void oid_mutex_init_recursive( oid_mutex_t * );
  void oid_mutex_destroy( oid_mutex_t * );
  void oid_mutex_lock( oid_mutex_t * );
  int oid_mutex_trylock( oid_mutex_t * ) ;
  void oid_mutex_unlock( oid_mutex_t * );
  void oid_cond_init( oid_cond_t * );
  void oid_cond_init_daytime( oid_cond_t * );
  void oid_cond_destroy( oid_cond_t * );
 void oid_cond_signal(oid_cond_t *);
 void oid_cond_broadcast(oid_cond_t *);
  void oid_cond_wait(oid_cond_t *, oid_mutex_t *);
  int oid_cond_timedwait(oid_cond_t *, oid_mutex_t *, mtime_t);
  void oid_sem_init(oid_sem_t *, unsigned);
  void oid_sem_destroy(oid_sem_t *);
  int oid_sem_post(oid_sem_t *);
  void oid_sem_wait(oid_sem_t *);

  void oid_rwlock_init(oid_rwlock_t *);
  void oid_rwlock_destroy(oid_rwlock_t *);
  void oid_rwlock_rdlock(oid_rwlock_t *);
  void oid_rwlock_wrlock(oid_rwlock_t *);
  void oid_rwlock_unlock(oid_rwlock_t *);
  int oid_threadvar_create(oid_threadvar_t * , void (*) (void *) );
  void oid_threadvar_delete(oid_threadvar_t *);
  int oid_threadvar_set(oid_threadvar_t, void *);
  void * oid_threadvar_get(oid_threadvar_t);

  int oid_clone(oid_thread_t *, void * (*) (void *), void *, int);
  void oid_cancel(oid_thread_t);
  void oid_join(oid_thread_t, void **);
  void oid_control_cancel (int cmd, ...);

  mtime_t mdate(void);
  void mwait(mtime_t deadline);
  void msleep(mtime_t delay);

/**
 * Registers a new procedure to run if the thread is cancelled (or otherwise
 * exits prematurely). Any call to oid_cleanup_push() <b>must</b> paired with a
 * call to either oid_cleanup_pop() or oid_cleanup_run(). Branching into or out
 * of the block between these two function calls is not allowed (read: it will
 * likely crash the whole process). If multiple procedures are registered,
 * they are handled in last-in first-out order.
 *
 * @param routine procedure to call if the thread ends
 * @param arg argument for the procedure
 */
# define oid_cleanup_push( routine, arg ) pthread_cleanup_push (routine, arg)

/**
 * Removes a cleanup procedure that was previously registered with
 * oid_cleanup_push().
 */
# define oid_cleanup_pop( ) pthread_cleanup_pop (0)

/**
 * Removes a cleanup procedure that was previously registered with
 * oid_cleanup_push(), and executes it.
 */
# define oid_cleanup_run( ) pthread_cleanup_pop (1)


#define OID_THREAD_PRIORITY_LOW      0
#define OID_THREAD_PRIORITY_INPUT   10
#define OID_THREAD_PRIORITY_AUDIO    5
#define OID_THREAD_PRIORITY_VIDEO    0
#define OID_THREAD_PRIORITY_OUTPUT  15
#define OID_THREAD_PRIORITY_HIGHEST 20



static inline void oid_cleanup_lock (void *lock)
{
    oid_mutex_unlock ((oid_mutex_t *)lock);
}

#define mutex_cleanup_push( lock ) oid_cleanup_push (oid_cleanup_lock, lock)

#endif
