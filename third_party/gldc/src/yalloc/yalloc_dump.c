#include "yalloc_internals.h"

#include <stdio.h>

static void printOffset(void * pool, char * name, uint16_t offset)
{
  if (isNil(offset))
    printf("  %s: nil\n", name);
  else
    printf("  %s: %td\n", name, (char*)HDR_PTR(offset) - (char*)pool);
}

void yalloc_dump(void * pool, char * name)
{
  printf("---- %s ----\n", name);
  Header * cur = (Header*)pool;
  for (;;)
  {
    printf(isFree(cur) ? "%td: free @%p\n" : "%td: used @%p\n", (char*)cur - (char*)pool, cur);
    printOffset(pool, cur == pool ? "first free" : "prev", cur->prev);
    printOffset(pool, "next", cur->next);
    if (isFree(cur))
    {
      printOffset(pool, "prevFree", cur[1].prev);
      printOffset(pool, "nextFree", cur[1].next);
    }
    else
      printf("  payload includes padding: %i\n", isPadded(cur));

    if (isNil(cur->next))
      break;

    printf("  %td bytes payload\n", (char*)HDR_PTR(cur->next) - (char*)cur - sizeof(Header));

    cur = HDR_PTR(cur->next);
  }

  fflush(stdout);
}
