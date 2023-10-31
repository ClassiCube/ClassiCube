/**
@file

API of the yalloc allocator.
*/

#ifndef YALLOC_H
#define YALLOC_H

#include <stddef.h>

/**
Maximum supported pool size. yalloc_init() will fail for larger pools.
*/
#define MAX_POOL_SIZE ((2 << 24) - 4)

/**
Creates a pool inside a given buffer.

Pools must be deinitialized with yalloc_deinit() when they are no longer needed.

@param pool The starting address of the pool. It must have at least 16bit
alignment (internal structure uses 16bit integers). Allocations are placed at
32bit boundaries starting from this address, so if the user data should be
32bit aligned then this address has to be 32bit aligned. Typically an address
of static memory, or an array on the stack is used if the pool is only used
temporarily.
@param size Size of the pool.
@return 0 on success, nonzero if the size is not supported.
 */
int yalloc_init(void * pool, size_t size);

/**
Deinitializes the buffer that is used by the pool and makes it available for other use.

The content of the buffer is undefined after this.

@param pool The starting address of an initialized pool.
*/
void yalloc_deinit(void * pool);

/**
Allocates a block of memory from a pool.

This function mimics malloc().

The pool must not be in the "defragmenting" state when this function is called.

@param pool The starting address of an initialized pool.
@param size Number of bytes to allocate.
@return Allocated buffer or \c NULL if there was no free range that could serve
the allocation. See @ref yalloc_defrag_start() for a way to remove
fragmentation which may cause allocations to fail even when there is enough
space in total.
*/
void * yalloc_alloc(void * pool, size_t size);

/**
Returns an allocation to a pool.

This function mimics free().

The pool must not be in the "defragmenting" state when this function is called.

@param pool The starting address of the initialized pool the allocation comes from.
@param p An address that was returned from yalloc_alloc() of the same pool.
*/
void yalloc_free(void * pool, void * p);

/**
Returns the maximum size of a successful allocation (assuming a completely unfragmented heap).

After defragmentation the first allocation with the returned size is guaranteed to succeed.

@param pool The starting address of an initialized pool.
@return Number of bytes that can be allocated (assuming the pool is defragmented).
*/
size_t yalloc_count_free(void * pool);

/**
Returns the maximum continuous free area.

@param pool The starting address of an initialized pool.
@return Number of free bytes that exist continuously.
*/
size_t yalloc_count_continuous(void * pool_);

/**
Queries the usable size of an allocated block.

@param pool The starting address of the initialized pool the allocation comes from.
@param p An address that was returned from yalloc_alloc() of the same pool.
@return Size of the memory block. This is the size passed to @ref yalloc_alloc() rounded up to 4.
*/
size_t yalloc_block_size(void * pool, void * p);

/**
Finds the first (in address order) allocation of a pool.

@param pool The starting address of an initialized pool.
@return Address of the allocation the lowest address inside the pool (this is
what @ref yalloc_alloc() returned), or \c NULL if there is no used block.
*/
void * yalloc_first_used(void * pool);

/**
Given a pointer to an allocation finds the next (in address order) used block of a pool.

@param pool The starting address of the initialized pool the allocation comes from.
@param p Pointer to an allocation in that pool, typically comes from a previous
call to @ref yalloc_first_used()
*/
void * yalloc_next_used(void * pool, void * p);

/**
Starts defragmentation for a pool.

Allocations will stay where they are. But the pool is put in the "defagmenting"
state (see @ref yalloc_defrag_in_progress()).

The pool must not be in the "defragmenting" state when this function is called.
The pool is put into the "defragmenting" state by this function.

@param pool The starting address of an initialized pool.
*/
void yalloc_defrag_start(void * pool);

/**
Returns the address that an allocation will have after @ref yalloc_defrag_commit() is called.

The pool must be in the "defragmenting" state when this function is called.

@param pool The starting address of the initialized pool the allocation comes from.
@param p Pointer to an allocation in that pool.
@return The address the alloation will have after @ref yalloc_defrag_commit() is called.
*/
void * yalloc_defrag_address(void * pool, void * p);

/**
Finishes the defragmentation.

The content of all allocations in the pool will be moved to the address that
was reported by @ref yalloc_defrag_address(). The pool will then have only one
free block. This means that an <tt>yalloc_alloc(pool, yalloc_count_free(pool))</tt>
will succeed.

The pool must be in the "defragmenting" state when this function is called. The
pool is put back to normal state by this function.

@param pool The starting address of an initialized pool.
*/
void yalloc_defrag_commit(void * pool);

/**
Tells if the pool is in the "defragmenting" state (after a @ref yalloc_defrag_start() and before a @ref yalloc_defrag_commit()).

@param pool The starting address of an initialized pool.
@return Nonzero if the pool is currently in the "defragmenting" state.
*/
int yalloc_defrag_in_progress(void * pool);


/**
Helper function that dumps the state of the pool to stdout.

This function is only available if build with <tt>yalloc_dump.c</tt>. This
function only exists for debugging purposes and can be ignored by normal users
that are not interested in the internal structure of the implementation.

@param pool The starting address of an initialized pool.
@param name A string that is used as "Title" for the output.
*/
void yalloc_dump(void * pool, char * name);


#endif // YALLOC_H
