/**
 * \file platform_util.h
 *
 * \brief Common and shared functions used by multiple modules in the Mbed TLS
 *        library.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef MBEDTLS_PLATFORM_UTIL_H
#define MBEDTLS_PLATFORM_UTIL_H

#include <stddef.h>
#if defined(MBEDTLS_HAVE_TIME_DATE)
#include "platform_time.h"
#include <time.h>
#endif /* MBEDTLS_HAVE_TIME_DATE */

#ifdef __cplusplus
extern "C" {
#endif

/* Internal macros meant to be called only from within the library. */
#define MBEDTLS_INTERNAL_VALIDATE_RET( cond, ret )  do { } while( 0 )
#define MBEDTLS_INTERNAL_VALIDATE( cond )           do { } while( 0 )

/* Internal helper macros for deprecating API constants. */
#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
/* Deliberately don't (yet) export MBEDTLS_DEPRECATED here
 * to avoid conflict with other headers which define and use
 * it, too. We might want to move all these definitions here at
 * some point for uniformity. */
#define MBEDTLS_DEPRECATED __attribute__((deprecated))
MBEDTLS_DEPRECATED typedef char const * mbedtls_deprecated_string_constant_t;
#define MBEDTLS_DEPRECATED_STRING_CONSTANT( VAL )       \
    ( (mbedtls_deprecated_string_constant_t) ( VAL ) )
MBEDTLS_DEPRECATED typedef int mbedtls_deprecated_numeric_constant_t;
#define MBEDTLS_DEPRECATED_NUMERIC_CONSTANT( VAL )       \
    ( (mbedtls_deprecated_numeric_constant_t) ( VAL ) )
#undef MBEDTLS_DEPRECATED
#else /* MBEDTLS_DEPRECATED_WARNING */
#define MBEDTLS_DEPRECATED_STRING_CONSTANT( VAL ) VAL
#define MBEDTLS_DEPRECATED_NUMERIC_CONSTANT( VAL ) VAL
#endif /* MBEDTLS_DEPRECATED_WARNING */
#endif /* MBEDTLS_DEPRECATED_REMOVED */

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_PLATFORM_UTIL_H */
