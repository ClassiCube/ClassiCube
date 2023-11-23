#pragma once

#include <kos.h>
#include <dc/matrix.h>
#include <dc/pvr.h>
#include <dc/vec3f.h>
#include <dc/fmath.h>
#include <dc/matrix3d.h>

#include "types.h"
#include "private.h"
#include "sh4_math.h"

#ifndef GL_FORCE_INLINE
#define GL_NO_INSTRUMENT inline __attribute__((no_instrument_function))
#define GL_INLINE_DEBUG GL_NO_INSTRUMENT __attribute__((always_inline))
#define GL_FORCE_INLINE static GL_INLINE_DEBUG
#endif

#define PREFETCH(addr) __builtin_prefetch((addr))

GL_FORCE_INLINE void* memcpy_fast(void *dest, const void *src, size_t len) {
  if(!len) {
    return dest;
  }

  const uint8_t *s = (uint8_t *)src;
  uint8_t *d = (uint8_t *)dest;

  uint32_t diff = (uint32_t)d - (uint32_t)(s + 1); // extra offset because input gets incremented before output is calculated
  // Underflow would be like adding a negative offset

  // Can use 'd' as a scratch reg now
  asm volatile (
    "clrs\n" // Align for parallelism (CO) - SH4a use "stc SR, Rn" instead with a dummy Rn
  ".align 2\n"
  "0:\n\t"
    "dt %[size]\n\t" // (--len) ? 0 -> T : 1 -> T (EX 1)
    "mov.b @%[in]+, %[scratch]\n\t" // scratch = *(s++) (LS 1/2)
    "bf.s 0b\n\t" // while(s != nexts) aka while(!T) (BR 1/2)
    " mov.b %[scratch], @(%[offset], %[in])\n" // *(datatype_of_s*) ((char*)s + diff) = scratch, where src + diff = dest (LS 1)
    : [in] "+&r" ((uint32_t)s), [scratch] "=&r" ((uint32_t)d), [size] "+&r" (len) // outputs
    : [offset] "z" (diff) // inputs
    : "t", "memory" // clobbers
  );

  return dest;
}

/* We use sq_cpy if the src and size is properly aligned. We control that the
 * destination is properly aligned so we assert that. */
#define FASTCPY(dst, src, bytes) \
    do { \
        if(bytes % 32 == 0 && ((uintptr_t) src % 4) == 0) { \
            gl_assert(((uintptr_t) dst) % 32 == 0); \
            sq_cpy(dst, src, bytes); \
        } else { \
            memcpy_fast(dst, src, bytes); \
        } \
    } while(0)


#define MEMCPY4(dst, src, bytes) memcpy_fast(dst, src, bytes)

GL_FORCE_INLINE void TransformVertex(const float* xyz, const float* w, float* oxyz, float* ow) {
    register float __x __asm__("fr12") = (xyz[0]);
    register float __y __asm__("fr13") = (xyz[1]);
    register float __z __asm__("fr14") = (xyz[2]);
    register float __w __asm__("fr15") = (*w);

    __asm__ __volatile__(
        "fldi1 fr15\n"
        "ftrv   xmtrx,fv12\n"
        : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w)
        : "0" (__x), "1" (__y), "2" (__z), "3" (__w)
    );

    oxyz[0] = __x;
    oxyz[1] = __y;
    oxyz[2] = __z;
    *ow = __w;
}

void InitGPU(_Bool autosort, _Bool fsaa);

static inline void* GPUMemoryAlloc(size_t size) {
    return pvr_mem_malloc(size);
}

#define PT_ALPHA_REF 0x011c

static inline void GPUSetAlphaCutOff(uint8_t val) {
    PVR_SET(PT_ALPHA_REF, val);
}

static inline void GPUSetClearDepth(float v) {
    pvr_set_zclip(v);
}