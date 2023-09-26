#include "yalloc.h"
#include "yalloc_internals.h"

#include <assert.h>
#include <string.h>

#define ALIGN(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#if defined(YALLOC_VALGRIND) && !defined(NVALGRIND)
# define USE_VALGRIND 1
#else
# define USE_VALGRIND 0
#endif

#if USE_VALGRIND
# include <valgrind/memcheck.h>
#else
# define VALGRIND_MAKE_MEM_UNDEFINED(p, s) ((void)0)
# define VALGRIND_MAKE_MEM_DEFINED(p, s) ((void)0)
# define VALGRIND_MAKE_MEM_NOACCESS(p, s) ((void)0)
# define VALGRIND_CREATE_MEMPOOL(pool, rz, z) ((void)0)
# define VALGRIND_MEMPOOL_ALLOC(pool, p, s) ((void)0)
# define VALGRIND_MEMPOOL_FREE(pool, p)  ((void)0)
# define VALGRIND_MEMPOOL_CHANGE(pool, a, b, s)  ((void)0)
#endif

#define MARK_NEW_FREE_HDR(p) VALGRIND_MAKE_MEM_UNDEFINED(p, sizeof(Header) * 2)
#define MARK_NEW_HDR(p) VALGRIND_MAKE_MEM_UNDEFINED(p, sizeof(Header))
#define PROTECT_HDR(p) VALGRIND_MAKE_MEM_NOACCESS(p, sizeof(Header))
#define PROTECT_FREE_HDR(p) VALGRIND_MAKE_MEM_NOACCESS(p, sizeof(Header) * 2)
#define UNPROTECT_HDR(p) VALGRIND_MAKE_MEM_DEFINED(p, sizeof(Header))
#define UNPROTECT_FREE_HDR(p) VALGRIND_MAKE_MEM_DEFINED(p, sizeof(Header) * 2)


#if USE_VALGRIND
static void _unprotect_pool(void * pool)
{
  Header * cur = (Header*)pool;
  for (;;)
  {
    UNPROTECT_HDR(cur);
    if (isFree(cur))
      UNPROTECT_HDR(cur + 1);

    if (isNil(cur->next))
      break;

    cur = HDR_PTR(cur->next);
  }
}

static void _protect_pool(void * pool)
{
  Header * cur = (Header*)pool;
  while (cur)
  {
    Header * next = isNil(cur->next) ? NULL : HDR_PTR(cur->next);

    if (isFree(cur))
      VALGRIND_MAKE_MEM_NOACCESS(cur, (char*)next - (char*)cur);
    else
      PROTECT_HDR(cur);

    cur = next;
  }
}
#define assert_is_pool(pool) assert(VALGRIND_MEMPOOL_EXISTS(pool));

#else

static void _unprotect_pool(void * pool){(void)pool;}
static void _protect_pool(void * pool){(void)pool;}
#define assert_is_pool(pool) ((void)0)
#endif

// internal version that does not unprotect/protect the pool
static int _yalloc_defrag_in_progress(void * pool)
{
  // fragmentation is indicated by a free list with one entry: the last block of the pool, which has its "free"-bit cleared.
  Header * p = (Header*)pool;
  if (isNil(p->prev))
    return 0;

  return !(HDR_PTR(p->prev)->prev & 1);
}

int yalloc_defrag_in_progress(void * pool)
{
  _unprotect_pool(pool);
  int ret = _yalloc_defrag_in_progress(pool);
  _protect_pool(pool);
  return ret;
}

#if YALLOC_INTERNAL_VALIDATE

static size_t _count_free_list_occurences(Header * pool, Header * blk)
{
  int n = 0;
  if (!isNil(pool->prev))
  {
    Header * cur = HDR_PTR(pool->prev);
    for (;;)
    {
      if (cur == blk)
        ++n;

      if (isNil(cur[1].next))
        break;

      cur = HDR_PTR(cur[1].next);
    }
  }
  return n;
}

static size_t _count_addr_list_occurences(Header * pool, Header * blk)
{
  size_t n = 0;
  Header * cur = pool;
  for (;;)
  {
    if (cur == blk)
      ++n;

    if (isNil(cur->next))
      break;

    cur = HDR_PTR(cur->next);
  }
  return n;
}

static void _validate_user_ptr(void * pool, void * p)
{
  Header * hdr = (Header*)p - 1;
  size_t n = _count_addr_list_occurences((Header*)pool, hdr);
  assert(n == 1 && !isFree(hdr));
}

/**
Validates if all the invariants of a pool are intact.

This is very expensive when there are enough blocks in the heap (quadratic complexity!).
*/
static void _yalloc_validate(void * pool_)
{
  Header * pool = (Header*)pool_;
  Header * cur = pool;

  assert(!isNil(pool->next)); // there must always be at least two blocks: a free/used one and the final block at the end

  if (_yalloc_defrag_in_progress(pool))
  {
    Header * prevUsed = NULL;
    while (!isNil(cur->next))
    {
      if (!isFree(cur))
      { // it is a used block
        Header * newAddr = cur == pool ? pool : HDR_PTR(cur->prev);
        assert(newAddr <= cur);
        assert(newAddr >= pool);

        if (prevUsed)
        {
          Header * prevNewAddr = prevUsed == pool ? pool : HDR_PTR(prevUsed->prev);
          size_t prevBruttoSize = (char*)HDR_PTR(prevUsed->next) - (char*)prevUsed;
          if (isPadded(prevUsed))
            prevBruttoSize -= 4; // remove padding
          assert((char*)newAddr == (char*)prevNewAddr + prevBruttoSize);
        }
        else
        {
          assert(newAddr == pool);
        }

        prevUsed = cur;
      }

      cur = HDR_PTR(cur->next);
    }

    assert(cur == HDR_PTR(pool->prev)); // the free-list should point to the last block
    assert(!isFree(cur)); // the last block must not be free
  }
  else
  {
    Header * prev = NULL;

    // iterate blocks in address order
    for (;;)
    {
      if (prev)
      {
        Header * x = HDR_PTR(cur->prev);
        assert(x == prev);
      }

      int n = _count_free_list_occurences(pool, cur);
      if (isFree(cur))
      { // it is a free block
        assert(n == 1);
        assert(!isPadded(cur)); // free blocks must have a zero padding-bit

        if (prev)
        {
          assert(!isFree(prev)); // free blocks must not be direct neighbours
        }
      }
      else
      {
        assert(n == 0);
      }

      if (isNil(cur->next))
        break;

      Header * next = HDR_PTR(cur->next);
      assert((char*)next >= (char*)cur + sizeof(Header) * 2);
      prev = cur;
      cur = next;
    }

    assert(isNil(cur->next));

    if (!isNil(pool->prev))
    {
      // iterate free-list
      Header * f = HDR_PTR(pool->prev);
      assert(isNil(f[1].prev));
      for (;;)
      {
        assert(isFree(f)); // must be free

        int n = _count_addr_list_occurences(pool, f);
        assert(n == 1);

        if (isNil(f[1].next))
          break;

        f = HDR_PTR(f[1].next);
      }
    }
  }
}

#else
static void _yalloc_validate(void * pool){(void)pool;}
static void _validate_user_ptr(void * pool, void * p){(void)pool; (void)p;}
#endif

int yalloc_init(void * pool, size_t size)
{
  if (size > MAX_POOL_SIZE)
    return -1;

  // TODO: Error when pool is not properly aligned

  // TODO: Error when size is not a multiple of the alignment?
  while (size % sizeof(Header))
    --size;

  if(size < sizeof(Header) * 3)
    return -1;

  VALGRIND_CREATE_MEMPOOL(pool, 0, 0);

  Header * first = (Header*)pool;
  Header * last = (Header*)((char*)pool + size) - 1;

  MARK_NEW_FREE_HDR(first);
  MARK_NEW_HDR(first);

  first->prev = HDR_OFFSET(first) | 1;
  first->next = HDR_OFFSET(last);
  first[1].prev = NIL;
  first[1].next = NIL;

  last->prev = HDR_OFFSET(first);
  last->next = NIL;

  _unprotect_pool(pool);
  _yalloc_validate(pool);
  _protect_pool(pool);
  return 0;
}

void yalloc_deinit(void * pool)
{
#if USE_VALGRIND
  VALGRIND_DESTROY_MEMPOOL(pool);

  Header * last = (Header*)pool;
  UNPROTECT_HDR(last);
  while (!isNil(last->next))
  {
    Header * next = HDR_PTR(last->next);
    UNPROTECT_HDR(next);
    last = next;
  }

  VALGRIND_MAKE_MEM_UNDEFINED(pool, (char*)(last + 1) - (char*)pool);
#else
  (void)pool;
#endif
}


void * yalloc_alloc(void * pool, size_t size)
{
  assert_is_pool(pool);
  _unprotect_pool(pool);
  assert(!_yalloc_defrag_in_progress(pool));
  _yalloc_validate(pool);
  if (!size)
  {
    _protect_pool(pool);
    return NULL;
  }

  Header * root = (Header*)pool;
  if (isNil(root->prev))
  {
    _protect_pool(pool);
    return NULL; /* no free block, no chance to allocate anything */ // TODO: Just read up which C standard supports single line comments and then fucking use them!
  }

  /* round up to alignment */
  size = ALIGN(size, 32);

  size_t bruttoSize = size + sizeof(Header);
  Header * prev = NULL;
  Header * cur = HDR_PTR(root->prev);
  for (;;)
  {
    size_t curSize = (char*)HDR_PTR(cur->next) - (char*)cur; /* size of the block, including its header */

    if (curSize >= bruttoSize) // it is big enough
    {
      // take action for unused space in the free block
      if (curSize >= bruttoSize + sizeof(Header) * 2)
      { // the leftover space is big enough to make it a free block
        // Build a free block from the unused space and insert it into the list of free blocks after the current free block
        Header * tail = (Header*)((char*)cur + bruttoSize);
        MARK_NEW_FREE_HDR(tail);

        // update address-order-list
        tail->next = cur->next;
        tail->prev = HDR_OFFSET(cur) | 1;
        HDR_PTR(cur->next)->prev = HDR_OFFSET(tail); // NOTE: We know the next block is used because free blocks are never neighbours. So we don't have to care about the lower bit which would be set for the prev of a free block.
        cur->next = HDR_OFFSET(tail);

        // update list of free blocks
        tail[1].next = cur[1].next;
        // NOTE: tail[1].prev is updated in the common path below (assignment to "HDR_PTR(cur[1].next)[1].prev")

        if (!isNil(cur[1].next))
          HDR_PTR(cur[1].next)[1].prev = HDR_OFFSET(tail);
        cur[1].next = HDR_OFFSET(tail);
      }
      else if (curSize > bruttoSize)
      { // there will be unused space, but not enough to insert a free header
        internal_assert(curSize - bruttoSize == sizeof(Header)); // unused space must be enough to build a free-block or it should be exactly the size of a Header
        cur->next |= 1; // set marker for "has unused trailing space"
      }
      else
      {
        internal_assert(curSize == bruttoSize);
      }

      cur->prev &= NIL; // clear marker for "is a free block"

      // remove from linked list of free blocks
      if (prev)
        prev[1].next = cur[1].next;
      else
      {
        uint32_t freeBit = isFree(root);
        root->prev = (cur[1].next & NIL) | freeBit;
      }

      if (!isNil(cur[1].next))
        HDR_PTR(cur[1].next)[1].prev = prev ? HDR_OFFSET(prev) : NIL;

      _yalloc_validate(pool);
      VALGRIND_MEMPOOL_ALLOC(pool, cur + 1, size);
      _protect_pool(pool);
      return cur + 1; // return address after the header
    }

    if (isNil(cur[1].next))
      break;

    prev = cur;
    cur = HDR_PTR(cur[1].next);
  }

  _yalloc_validate(pool);
  _protect_pool(pool);
  return NULL;
}

// Removes a block from the free-list and moves the pools first-free-bock pointer to its successor if it pointed to that block.
static void unlink_from_free_list(Header * pool, Header * blk)
{
  // update the pools pointer to the first block in the free list if necessary
  if (isNil(blk[1].prev))
  { // the block is the first in the free-list
    // make the pools first-free-pointer point to the next in the free list
    uint32_t freeBit = isFree(pool);
    pool->prev = (blk[1].next & NIL) | freeBit;
  }
  else
    HDR_PTR(blk[1].prev)[1].next = blk[1].next;

  if (!isNil(blk[1].next))
    HDR_PTR(blk[1].next)[1].prev = blk[1].prev;
}

size_t yalloc_block_size(void * pool, void * p)
{
  Header * a = (Header*)p - 1;
  UNPROTECT_HDR(a);
  Header * b = HDR_PTR(a->next);
  size_t payloadSize = (char*)b - (char*)p;
  if (isPadded(a))
    payloadSize -= sizeof(Header);
  PROTECT_HDR(a);
  return payloadSize;
}

void yalloc_free(void * pool_, void * p)
{
  assert_is_pool(pool_);
  assert(!yalloc_defrag_in_progress(pool_));
  if (!p)
    return;

  _unprotect_pool(pool_);

  Header * pool = (Header*)pool_;
  Header * cur = (Header*)p - 1;

  // get pointers to previous/next block in address order
  Header * prev = cur == pool || isNil(cur->prev) ? NULL : HDR_PTR(cur->prev);
  Header * next = isNil(cur->next) ? NULL : HDR_PTR(cur->next);

  int prevFree = prev && isFree(prev);
  int nextFree = next && isFree(next);

#if USE_VALGRIND
  {
    unsigned errs = VALGRIND_COUNT_ERRORS;
    VALGRIND_MEMPOOL_FREE(pool, p);
    if (VALGRIND_COUNT_ERRORS > errs)
    { // early exit if the free was invalid (so we get a valgrind error and don't mess up the pool, which is helpful for testing if invalid frees are detected by valgrind)
      _protect_pool(pool_);
      return;
    }
  }
#endif

  _validate_user_ptr(pool_, p);

  if (prevFree && nextFree)
  { // the freed block has two free neighbors
    unlink_from_free_list(pool, prev);
    unlink_from_free_list(pool, next);

    // join prev, cur and next
    prev->next = next->next;
    HDR_PTR(next->next)->prev = cur->prev;

    // prev is now the block we want to push onto the free-list
    cur = prev;
  }
  else if (prevFree)
  {
    unlink_from_free_list(pool, prev);

    // join prev and cur
    prev->next = cur->next;
    HDR_PTR(cur->next)->prev = cur->prev;

    // prev is now the block we want to push onto the free-list
    cur = prev;
  }
  else if (nextFree)
  {
    unlink_from_free_list(pool, next);

    // join cur and next
    cur->next = next->next;
    HDR_PTR(next->next)->prev = next->prev & NIL;
  }

  // if there is a previous block and that block has padding then we want to grow the new free block into that padding
  if (cur != pool && !isNil(cur->prev))
  { // there is a previous block
    Header * left = HDR_PTR(cur->prev);
    if (isPadded(left))
    { // the previous block has padding, so extend the current block to consume move the padding to the current free block
      Header * grown = cur - 1;
      MARK_NEW_HDR(grown);
      grown->next = cur->next;
      grown->prev = cur->prev;
      left->next = HDR_OFFSET(grown);
      if (!isNil(cur->next))
        HDR_PTR(cur->next)->prev = HDR_OFFSET(grown);

      cur = grown;
    }
  }

  cur->prev |= 1; // it becomes a free block
  cur->next &= NIL; // reset padding-bit
  UNPROTECT_HDR(cur + 1);
  cur[1].prev = NIL; // it will be the first free block in the free list, so it has no prevFree

  if (!isNil(pool->prev))
  { // the free-list was already non-empty
    HDR_PTR(pool->prev)[1].prev = HDR_OFFSET(cur); // make the first entry in the free list point back to the new free block (it will become the first one)
    cur[1].next = pool->prev; // the next free block is the first of the old free-list
  }
  else
    cur[1].next = NIL; // free-list was empty, so there is no successor

  VALGRIND_MAKE_MEM_NOACCESS(cur + 2, (char*)HDR_PTR(cur->next) - (char*)(cur + 2));

  // now the freed block is the first in the free-list

  // update the offset to the first element of the free list
  uint32_t freeBit = isFree(pool); // remember the free-bit of the offset
  pool->prev = HDR_OFFSET(cur) | freeBit; // update the offset and restore the free-bit
  _yalloc_validate(pool);
  _protect_pool(pool);
}

size_t yalloc_count_free(void * pool_)
{
  assert_is_pool(pool_);
  _unprotect_pool(pool_);
  assert(!_yalloc_defrag_in_progress(pool_));
  Header * pool = (Header*)pool_;
  size_t bruttoFree = 0;
  Header * cur = pool;

  _yalloc_validate(pool);

  for (;;)
  {
    if (isFree(cur))
    { // it is a free block
      bruttoFree += (char*)HDR_PTR(cur->next) - (char*)cur;
    }
    else
    { // it is a used block
      if (isPadded(cur))
      { // the used block is padded
        bruttoFree += sizeof(Header);
      }
    }

    if (isNil(cur->next))
      break;

    cur = HDR_PTR(cur->next);
  }

  _protect_pool(pool);

  if (bruttoFree < sizeof(Header))
  {
    internal_assert(!bruttoFree); // free space should always be a multiple of sizeof(Header)
    return 0;
  }

  return bruttoFree - sizeof(Header);
}

size_t yalloc_count_continuous(void * pool_)
{
  assert_is_pool(pool_);
  _unprotect_pool(pool_);
  assert(!_yalloc_defrag_in_progress(pool_));
  Header * pool = (Header*)pool_;
  size_t largestFree = 0;
  Header * cur = pool;

  _yalloc_validate(pool);

  for (;;)
  {
    if (isFree(cur))
    { // it is a free block
      size_t temp = (uintptr_t)HDR_PTR(cur->next) - (uintptr_t)cur;
      if(temp > largestFree)
        largestFree = temp;
    }

    if (isNil(cur->next))
      break;

    cur = HDR_PTR(cur->next);
  }

  _protect_pool(pool);

  if (largestFree < sizeof(Header))
  {
    internal_assert(!largestFree); // free space should always be a multiple of sizeof(Header)
    return 0;
  }

  return largestFree - sizeof(Header);
}

void * yalloc_first_used(void * pool)
{
  assert_is_pool(pool);
  _unprotect_pool(pool);
  Header * blk = (Header*)pool;
  while (!isNil(blk->next))
  {
    if (!isFree(blk))
    {
      _protect_pool(pool);
      return blk + 1;
    }

    blk = HDR_PTR(blk->next);
  }

  _protect_pool(pool);
  return NULL;
}

void * yalloc_next_used(void * pool, void * p)
{
  assert_is_pool(pool);
  _unprotect_pool(pool);
  _validate_user_ptr(pool, p);
  Header * prev = (Header*)p - 1;
  assert(!isNil(prev->next)); // the last block should never end up as input to this function (because it is not user-visible)

  Header * blk = HDR_PTR(prev->next);
  while (!isNil(blk->next))
  {
    if (!isFree(blk))
    {
      _protect_pool(pool);
      return blk + 1;
    }

    blk = HDR_PTR(blk->next);
  }

  _protect_pool(pool);
  return NULL;
}

void yalloc_defrag_start(void * pool_)
{
  assert_is_pool(pool_);
  _unprotect_pool(pool_);
  assert(!_yalloc_defrag_in_progress(pool_));
  Header * pool = (Header*)pool_;

  // iterate over all blocks in address order and store the post-defragment address of used blocks in their "prev" field
  size_t end = 0; // offset for the next used block
  Header * blk = (Header*)pool;
  for (; !isNil(blk->next); blk = HDR_PTR(blk->next))
  {
    if (!isFree(blk))
    { // it is a used block
      blk->prev = end >> 1;
      internal_assert((char*)HDR_PTR(blk->prev) == (char*)pool + end);

      size_t bruttoSize = (char*)HDR_PTR(blk->next) - (char*)blk;

      if (isPadded(blk))
      { // the block is padded
        bruttoSize -= sizeof(Header);
      }

      end += bruttoSize;
      internal_assert(end % sizeof(Header) == 0);
    }
  }

  // blk is now the last block (the dummy "used" block at the end of the pool)
  internal_assert(isNil(blk->next));
  internal_assert(!isFree(blk));

  // mark the pool as "defragementation in progress"
  uint32_t freeBit = isFree(pool);
  pool->prev = (HDR_OFFSET(blk) & NIL) | freeBit;

  _yalloc_validate(pool);
  internal_assert(yalloc_defrag_in_progress(pool));
  _protect_pool(pool);
}

void * yalloc_defrag_address(void * pool_, void * p)
{
  assert_is_pool(pool_);
  assert(yalloc_defrag_in_progress(pool_));
  if (!p)
    return NULL;

  Header * pool = (Header*)pool_;

  _unprotect_pool(pool);
  _validate_user_ptr(pool_, p);

  if (pool + 1 == p)
    return pool + 1; // "prev" of the first block points to the last used block to mark the pool as "defragmentation in progress"

  Header * blk = (Header*)p - 1;

  void * defragP = HDR_PTR(blk->prev) + 1;

  _protect_pool(pool);
  return defragP;
}

void yalloc_defrag_commit(void * pool_)
{
  assert_is_pool(pool_);
  _unprotect_pool(pool_);
  assert(_yalloc_defrag_in_progress(pool_));
  Header * pool = (Header*)pool_;

  // iterate over all blocks in address order and move them
  size_t end = 0; // offset for the next used block
  Header * blk = pool;
  Header * lastUsed = NULL;
  while (!isNil(blk->next))
  {
    if (!isFree(blk))
    { // it is a used block
      size_t bruttoSize = (char*)HDR_PTR(blk->next) - (char*)blk;

      if (isPadded(blk))
      { // the block is padded
        bruttoSize -= sizeof(Header);
      }

      Header * next = HDR_PTR(blk->next);

      blk->prev = lastUsed ? HDR_OFFSET(lastUsed) : NIL;
      blk->next = (end + bruttoSize) >> 1;

      lastUsed = (Header*)((char*)pool + end);
      VALGRIND_MAKE_MEM_UNDEFINED(lastUsed, (char*)blk - (char*)lastUsed);
      memmove(lastUsed, blk, bruttoSize);
      VALGRIND_MEMPOOL_CHANGE(pool, blk + 1, lastUsed + 1, bruttoSize - sizeof(Header));

      end += bruttoSize;
      blk = next;
    }
    else
      blk = HDR_PTR(blk->next);
  }

  // blk is now the last block (the dummy "used" block at the end of the pool)
  internal_assert(isNil(blk->next));
  internal_assert(!isFree(blk));

  if (lastUsed)
  {
    Header * gap = HDR_PTR(lastUsed->next);
    if (gap == blk)
    { // there is no gap
      pool->prev = NIL; // the free list is empty
      blk->prev = HDR_OFFSET(lastUsed);
    }
    else if (blk - gap > 1)
    { // the gap is big enouogh for a free Header

      // set a free list that contains the gap as only element
      gap->prev = HDR_OFFSET(lastUsed) | 1;
      gap->next = HDR_OFFSET(blk);
      gap[1].prev = NIL;
      gap[1].next = NIL;
      pool->prev = blk->prev = HDR_OFFSET(gap);
    }
    else
    { // there is a gap, but it is too small to be used as free-list-node, so just make it padding of the last used block
      lastUsed->next = HDR_OFFSET(blk) | 1;
      pool->prev = NIL;
      blk->prev = HDR_OFFSET(lastUsed);
    }
  }
  else
  { // the pool is empty
    pool->prev = 1;
  }

  internal_assert(!_yalloc_defrag_in_progress(pool));
  _yalloc_validate(pool);
  _protect_pool(pool);
}
