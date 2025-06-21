#include <stdint.h>
/*-
 * Copyright (c) 2019 Christophe Meessen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

/*-
 * Computing the square root of an integer or a fixed point integer into a fixed
 * point integer. A fixed point is a 32 bit value with the comma between the
 * bits 15 and 16, where bit 0 is the less significant bit of the value.
 *
 * The algorithm uses the property that computing x² is trivial compared to the
 * sqrt. It will thus search the biggest x so that x² <= v, assuming we compute
 * sqrt(v).
 *
 * The algorithm tests each bit of x starting with the most significant toward
 * the less significant. It tests if the bit must be set or not.
 *
 * The algorithm uses the relation (x + a)² = x² + 2ax + a² to add the bit
 * efficiently. Instead of computing x² it keeps track of (x + a)² - x².
 *
 * When computing sqrt(v), r = v - x², q = 2ax, b = a² and t = 2ax + a2.
 *
 * Note that the input integers are signed and that the sign bit is used in the
 * computation. To accept unsigned integer as input, unfolding the initial loop
 * is required to handle this particular case. See the usenet discussion for the
 * proposed solution.
 *
 * Algorithm and code Author Christophe Meessen 1993. */

static int32_t sqrt_fix16(int32_t value)
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

