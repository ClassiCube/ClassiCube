/***************************************************************************/
/*                                                                         */
/*  ftstdlib.h                                                             */
/*                                                                         */
/*    ANSI-specific library and header configuration file (specification   */
/*    only).                                                               */
/*                                                                         */
/*  Copyright 2002-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This file is used to group all #includes to the ANSI C library that   */
  /* FreeType normally requires.  It also defines macros to rename the     */
  /* standard functions within the FreeType source code.                   */
  /*                                                                       */
  /* Load a file which defines FTSTDLIB_H_ before this one to override it. */
  /*                                                                       */
  /*************************************************************************/


#ifndef FTSTDLIB_H_
#define FTSTDLIB_H_


#include <stddef.h>

#define ft_ptrdiff_t  ptrdiff_t


  /**********************************************************************/
  /*                                                                    */
  /*                           integer limits                           */
  /*                                                                    */
  /* UINT_MAX and ULONG_MAX are used to automatically compute the size  */
  /* of `int' and `long' in bytes at compile-time.  So far, this works  */
  /* for all platforms the library has been tested on.                  */
  /*                                                                    */
  /* Note that on the extremely rare platforms that do not provide      */
  /* integer types that are _exactly_ 16 and 32 bits wide (e.g. some    */
  /* old Crays where `int' is 36 bits), we do not make any guarantee    */
  /* about the correct behaviour of FT2 with all fonts.                 */
  /*                                                                    */
  /* In these case, `ftconfig.h' will refuse to compile anyway with a   */
  /* message like `couldn't find 32-bit type' or something similar.     */
  /*                                                                    */
  /**********************************************************************/


#include <limits.h>

#define FT_CHAR_BIT    CHAR_BIT
#define FT_USHORT_MAX  USHRT_MAX
#define FT_INT_MAX     INT_MAX
#define FT_INT_MIN     INT_MIN
#define FT_UINT_MAX    UINT_MAX
#define FT_LONG_MIN    LONG_MIN
#define FT_LONG_MAX    LONG_MAX
#define FT_ULONG_MAX   ULONG_MAX


  /**********************************************************************/
  /*                                                                    */
  /*                 character and string processing                    */
  /*                                                                    */
  /**********************************************************************/


#include <string.h>
/* ClassiCube functions to avoid stdlib */
extern int   cc_strncmp(const char* a, const char* b, size_t maxCount);
extern int    cc_strcmp(const char* a, const char* b);
extern size_t cc_strlen(const char* a);
extern char*  cc_strstr(const char* str, const char* substr);

extern int   cc_memcmp(const void* ptrA, const void* ptrB, size_t num);
extern void* cc_memchr(const void* ptr, int ch, size_t num);
extern void* Mem_Copy(void* dst, const void* src,  unsigned size);
extern void* Mem_Move(void* dst, const void* src,  unsigned size);
extern void* Mem_Set(void* dst, unsigned char val, unsigned size);

extern void cc_qsort(void* v, size_t count, size_t size,
					int (*comp)(const void*, const void*));

#define ft_memchr   cc_memchr
#define ft_memcmp   cc_memcmp
#define ft_memcpy   Mem_Copy
#define ft_memmove  Mem_Move
#define ft_memset   Mem_Set
#define ft_strcmp   cc_strcmp
#define ft_strlen   cc_strlen
#define ft_strncmp  cc_strncmp
#define ft_strstr   cc_strstr


  /**********************************************************************/
  /*                                                                    */
  /*                             sorting                                */
  /*                                                                    */
  /**********************************************************************/


#include <stdlib.h>

#define ft_qsort  cc_qsort


  /**********************************************************************/
  /*                                                                    */
  /*                         execution control                          */
  /*                                                                    */
  /**********************************************************************/


#include <setjmp.h>



#define ft_jmp_buf     jmp_buf  /* note: this cannot be a typedef since */
                                /*       jmp_buf is defined as a macro  */
                                /*       on certain platforms           */

#ifdef CC_BUILD_NOSTDLIB
	/* Avoid stdlib with MinGW for Win9x build */
	#define ft_longjmp     __builtin_longjmp
	#define ft_setjmp( b ) __builtin_setjmp( *(ft_jmp_buf*) &(b) ) /* same thing here */
#else
	#define ft_longjmp     longjmp
	#define ft_setjmp( b ) setjmp( *(ft_jmp_buf*) &(b) ) /* same thing here */
#endif

#endif /* FTSTDLIB_H_ */


/* END */
