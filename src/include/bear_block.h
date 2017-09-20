#ifndef __BEAR_BLOCK_H__
#define __BEAR_BLOCK_H__

#include "oid_common.h"
/* block */
typedef struct block_t      block_t;
typedef struct block_fifo_t block_fifo_t;


typedef void (*block_free_t) (block_t *);

struct block_t
{
    block_t    *p_next;

    uint8_t    *p_buffer; /**< Payload start */
    size_t      i_buffer; /**< Payload length */
    uint8_t    *p_start; /**< Buffer start */
    size_t      i_size; /**< Buffer total size */

    uint32_t    i_flags;
    unsigned    i_nb_samples; /* Used for audio */

    mtime_t     i_pts;
    mtime_t     i_dts;
    mtime_t     i_length;

    /* Rudimentary support for overloading block (de)allocation. */
    block_free_t pf_release;
};


void block_Init( block_t *, void *, size_t );

block_t *block_Alloc( size_t ) ;

block_t *block_Realloc( block_t *, ssize_t i_pre, size_t i_body );

static inline void block_Release( block_t *p_block )
{
    p_block->pf_release( p_block );
}
/****************************************************************************
 * Fifos of blocks.
 ****************************************************************************
 * - block_FifoNew : create and init a new fifo
 * - block_FifoRelease : destroy a fifo and free all blocks in it.
 * - block_FifoPace : wait for a fifo to drain to a specified number of packets or total data size
 * - block_FifoEmpty : free all blocks in a fifo
 * - block_FifoPut : put a block
 * - block_FifoGet : get a packet from the fifo (and wait if it is empty)
 * - block_FifoShow : show the first packet of the fifo (and wait if
 *      needed), be carefull, you can use it ONLY if you are sure to be the
 *      only one getting data from the fifo.
 * - block_FifoCount : how many packets are waiting in the fifo
 *
 * block_FifoGet and block_FifoShow are cancellation points.
 ****************************************************************************/

block_fifo_t *block_FifoNew( void ) ;
void block_FifoRelease( block_fifo_t * );
void block_FifoPace( block_fifo_t *fifo, size_t max_depth, size_t max_size );
void block_FifoEmpty( block_fifo_t * );
size_t block_FifoPut( block_fifo_t *, block_t * );
void block_FifoWake( block_fifo_t * );
block_t * block_FifoGet( block_fifo_t * ) ;
block_t * block_FifoShow( block_fifo_t * );
size_t block_FifoSize( const block_fifo_t *p_fifo );
size_t block_FifoCount( const block_fifo_t *p_fifo );

#endif

