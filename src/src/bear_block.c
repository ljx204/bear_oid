#include "oid_common.h"


# define block_Check(b) ((void)(b))
# define block_Invalidate(b) ((void)(b))


void block_Init( block_t *  b, void *buf, size_t size )
{
    /* Fill all fields to their default */
    b->p_next = NULL;
    b->p_buffer = buf;
    b->i_buffer = size;
    b->p_start = buf;
    b->i_size = size;
    b->i_flags = 0;
    b->i_nb_samples = 0;
//    b->i_pts =
//    b->i_dts = VLC_TS_INVALID;
    b->i_length = 0;
//    b->pf_release = BlockNoRelease;
}

static void block_generic_Release (block_t *block)
{
    /* That is always true for blocks allocated with block_Alloc(). */
    assert (block->p_start == (unsigned char *)(block + 1));
    block_Invalidate (block);
    free (block);
}

/** Initial memory alignment of data block.
 * @note This must be a multiple of sizeof(void*) and a power of two.
 * libavcodec AVX optimizations require at least 32-bytes. */
#define BLOCK_ALIGN        32

/** Initial reserved header and footer size. */
#define BLOCK_PADDING      32

/* Maximum size of reserved footer before shrinking with realloc(). */
#define BLOCK_WASTE_SIZE   2048

block_t *block_Alloc (size_t size)
{
    /* 2 * BLOCK_PADDING: pre + post padding */
    const size_t alloc = sizeof (block_t) + BLOCK_ALIGN + (2 * BLOCK_PADDING)
                       + size;
    if (unlikely(alloc <= size))
        return NULL;

    block_t *b = malloc (alloc);
    if (unlikely(b == NULL))
        return NULL;

    block_Init (b, b + 1, alloc - sizeof (*b));
 //   static_assert ((BLOCK_PADDING % BLOCK_ALIGN) == 0,
 //                  "BLOCK_PADDING must be a multiple of BLOCK_ALIGN");
    b->p_buffer += BLOCK_PADDING + BLOCK_ALIGN - 1;
    b->p_buffer = (void *)(((uintptr_t)b->p_buffer) & ~(BLOCK_ALIGN - 1));
    b->i_buffer = size;
    b->pf_release = block_generic_Release;
    return b;
}


/**
 * @section Thread-safe block queue functions
 */

/**
 * Internal state for block queues
 */
struct block_fifo_t
{
    oid_mutex_t         lock;                         /* fifo data lock */
    oid_cond_t          wait;      /**< Wait for data */
    oid_cond_t          wait_room; /**< Wait for queue depth to shrink */

    block_t             *p_first;
    block_t             **pp_last;
    size_t              i_depth;
    size_t              i_size;
    bool          b_force_wake;
};


block_fifo_t *block_FifoNew( void )
{
    block_fifo_t *p_fifo = malloc( sizeof( block_fifo_t ) );
    if( !p_fifo )
        return NULL;

    oid_mutex_init( &p_fifo->lock );
    oid_cond_init( &p_fifo->wait );
    oid_cond_init( &p_fifo->wait_room );
    p_fifo->p_first = NULL;
    p_fifo->pp_last = &p_fifo->p_first;
    p_fifo->i_depth = p_fifo->i_size = 0;
    p_fifo->b_force_wake = false;

    return p_fifo;
}


void block_FifoRelease( block_fifo_t *p_fifo )
{
    block_FifoEmpty( p_fifo );
    oid_cond_destroy( &p_fifo->wait_room );
    oid_cond_destroy( &p_fifo->wait );
    oid_mutex_destroy( &p_fifo->lock );
    free( p_fifo );
}

void block_FifoEmpty( block_fifo_t *p_fifo )
{
    block_t *block;

    oid_mutex_lock( &p_fifo->lock );
    block = p_fifo->p_first;
    if (block != NULL)
    {
        p_fifo->i_depth = p_fifo->i_size = 0;
        p_fifo->p_first = NULL;
        p_fifo->pp_last = &p_fifo->p_first;
    }
    oid_cond_broadcast( &p_fifo->wait_room );
    oid_mutex_unlock( &p_fifo->lock );

    while (block != NULL)
    {
        block_t *buf;

        buf = block->p_next;
        block_Release (block);
        block = buf;
    }
}

/**
 * Wait until the FIFO gets below a certain size (if needed).
 *
 * Note that if more than one thread writes to the FIFO, you cannot assume that
 * the FIFO is actually below the requested size upon return (since another
 * thread could have refilled it already). This is typically not an issue, as
 * this function is meant for (relaxed) congestion control.
 *
 * This function may be a cancellation point and it is cancel-safe.
 *
 * @param fifo queue to wait on
 * @param max_depth wait until the queue has no more than this many blocks
 *                  (use SIZE_MAX to ignore this constraint)
 * @param max_size wait until the queue has no more than this many bytes
 *                  (use SIZE_MAX to ignore this constraint)
 * @return nothing.
 */
void block_FifoPace (block_fifo_t *fifo, size_t max_depth, size_t max_size)
{
    oid_testcancel ();

    oid_mutex_lock (&fifo->lock);
    while ((fifo->i_depth > max_depth) || (fifo->i_size > max_size))
    {
         mutex_cleanup_push (&fifo->lock);
         oid_cond_wait (&fifo->wait_room, &fifo->lock);
         oid_cleanup_pop ();
    }
    oid_mutex_unlock (&fifo->lock);
}


/**
 * Immediately queue one block at the end of a FIFO.
 * @param fifo queue
 * @param block head of a block list to queue (may be NULL)
 * @return total number of bytes appended to the queue
 */
size_t block_FifoPut( block_fifo_t *p_fifo, block_t *p_block )
{
    size_t i_size = 0, i_depth = 0;
    block_t *p_last;

    if (p_block == NULL)
        return 0;
    for (p_last = p_block; ; p_last = p_last->p_next)
    {
        i_size += p_last->i_buffer;
        i_depth++;
        if (!p_last->p_next)
            break;
    }

    oid_mutex_lock (&p_fifo->lock);
    *p_fifo->pp_last = p_block;
    p_fifo->pp_last = &p_last->p_next;
    p_fifo->i_depth += i_depth;
    p_fifo->i_size += i_size;
    /* We queued at least one block: wake up one read-waiting thread */
    oid_cond_signal( &p_fifo->wait );
    oid_mutex_unlock( &p_fifo->lock );

    return i_size;
}


void block_FifoWake( block_fifo_t *p_fifo )
{
    oid_mutex_lock( &p_fifo->lock );
    if( p_fifo->p_first == NULL )
        p_fifo->b_force_wake = true;
    oid_cond_broadcast( &p_fifo->wait );
    oid_mutex_unlock( &p_fifo->lock );
}


/**
 * Dequeue the first block from the FIFO. If necessary, wait until there is
 * one block in the queue. This function is (always) cancellation point.
 *
 * @return a valid block, or NULL if block_FifoWake() was called.
 */
block_t *block_FifoGet( block_fifo_t *p_fifo )
{
    block_t *b;

    oid_testcancel( );

    oid_mutex_lock( &p_fifo->lock );
    mutex_cleanup_push( &p_fifo->lock );

    /* Remember vlc_cond_wait() may cause spurious wakeups
     * (on both Win32 and POSIX) */
    while( ( p_fifo->p_first == NULL ) && !p_fifo->b_force_wake )
        oid_cond_wait( &p_fifo->wait, &p_fifo->lock );

    oid_cleanup_pop();
    b = p_fifo->p_first;

    p_fifo->b_force_wake = false;
    if( b == NULL )
    {
        /* Forced wakeup */
        oid_mutex_unlock( &p_fifo->lock );
        return NULL;
    }

    p_fifo->p_first = b->p_next;
    p_fifo->i_depth--;
    p_fifo->i_size -= b->i_buffer;

    if( p_fifo->p_first == NULL )
    {
        p_fifo->pp_last = &p_fifo->p_first;
    }

    /* We don't know how many threads can queue new packets now. */
    oid_cond_broadcast( &p_fifo->wait_room );
    oid_mutex_unlock( &p_fifo->lock );

    b->p_next = NULL;
    return b;
}


/**
 * Peeks the first block in the FIFO.
 * If necessary, wait until there is one block.
 * This function is (always) a cancellation point.
 *
 * @warning This function leaves the block in the FIFO.
 * You need to protect against concurrent threads who could dequeue the block.
 * Preferrably, there should be only one thread reading from the FIFO.
 *
 * @return a valid block.
 */
block_t *block_FifoShow( block_fifo_t *p_fifo )
{
    block_t *b;

    oid_testcancel( );

    oid_mutex_lock( &p_fifo->lock );
    mutex_cleanup_push( &p_fifo->lock );

    while( p_fifo->p_first == NULL )
        oid_cond_wait( &p_fifo->wait, &p_fifo->lock );

    b = p_fifo->p_first;

    oid_cleanup_run ();
    return b;
}

/* FIXME: not thread-safe */
size_t block_FifoSize( const block_fifo_t *p_fifo )
{
    return p_fifo->i_size;
}

/* FIXME: not thread-safe */
size_t block_FifoCount( const block_fifo_t *p_fifo )
{
    return p_fifo->i_depth;
}














