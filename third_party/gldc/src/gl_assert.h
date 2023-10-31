
#ifndef NDEBUG
/* We're debugging, use normal assert */
#include <assert.h>
#define gl_assert assert
#else
/* Release mode, use our custom assert */
#include <stdio.h>
#include <stdlib.h>

#define gl_assert(x) \
    do {\
        if(!(x)) {\
            fprintf(stderr, "Assertion failed at %s:%d\n", __FILE__, __LINE__);\
            exit(1);\
        }\
    } while(0); \

#endif

