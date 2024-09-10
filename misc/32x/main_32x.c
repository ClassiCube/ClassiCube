#include <stdlib.h>

#include "32x.h"
#include "hw_32x.h"

int32_t fix16_sqrt(int32_t value)
{
    uint32_t t;
    uint32_t r = value;
    uint32_t b = 0x40000000;
    uint32_t q = 0;

    while (b > 0x40) {
        t = q + b;

        if (r >= t) {
            r -= t;
            q = t + b; /* Equivalent to q += 2 * b */
        }

        r <<= 1;
        b >>= 1;
    }

    q >>= 8;

    return q;
}

