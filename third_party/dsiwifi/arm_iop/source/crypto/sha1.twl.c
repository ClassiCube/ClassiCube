/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
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
/*
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

#include "common.h"

#if defined(MBEDTLS_SHA1_C)

#include "sha1.h"
#include "platform_util.h"
#include "error.h"

#include <string.h>

#define SHA1_VALIDATE_RET(cond)                             \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_SHA1_BAD_INPUT_DATA )

#define SHA1_VALIDATE(cond)  MBEDTLS_INTERNAL_VALIDATE( cond )

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

void mbedtls_sha1_init( mbedtls_sha1_context *ctx )
{
    SHA1_VALIDATE( ctx != NULL );

    memset( ctx, 0, sizeof( mbedtls_sha1_context ) );
}

void mbedtls_sha1_free( mbedtls_sha1_context *ctx )
{
    if( ctx == NULL )
        return;
}

void mbedtls_sha1_clone( mbedtls_sha1_context *dst,
                         const mbedtls_sha1_context *src )
{
    SHA1_VALIDATE( dst != NULL );
    SHA1_VALIDATE( src != NULL );

    *dst = *src;
}

/*
 * SHA-1 context setup
 */
int mbedtls_sha1_starts( mbedtls_sha1_context *ctx )
{
    SHA1_VALIDATE_RET( ctx != NULL );

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;

    return( 0 );
}

#if !defined(MBEDTLS_SHA1_PROCESS_ALT)
int mbedtls_internal_sha1_process( mbedtls_sha1_context *ctx,
                                   const unsigned char data[64] )
{
    struct
    {
        uint32_t temp, W[16], A, B, C, D, E;
    } local;

    SHA1_VALIDATE_RET( ctx != NULL );
    SHA1_VALIDATE_RET( (const unsigned char *)data != NULL );

    GET_UINT32_BE( local.W[ 0], data,  0 );
    GET_UINT32_BE( local.W[ 1], data,  4 );
    GET_UINT32_BE( local.W[ 2], data,  8 );
    GET_UINT32_BE( local.W[ 3], data, 12 );
    GET_UINT32_BE( local.W[ 4], data, 16 );
    GET_UINT32_BE( local.W[ 5], data, 20 );
    GET_UINT32_BE( local.W[ 6], data, 24 );
    GET_UINT32_BE( local.W[ 7], data, 28 );
    GET_UINT32_BE( local.W[ 8], data, 32 );
    GET_UINT32_BE( local.W[ 9], data, 36 );
    GET_UINT32_BE( local.W[10], data, 40 );
    GET_UINT32_BE( local.W[11], data, 44 );
    GET_UINT32_BE( local.W[12], data, 48 );
    GET_UINT32_BE( local.W[13], data, 52 );
    GET_UINT32_BE( local.W[14], data, 56 );
    GET_UINT32_BE( local.W[15], data, 60 );

#define S(x,n) (((x) << (n)) | (((x) & 0xFFFFFFFF) >> (32 - (n))))

#define R(t)                                                    \
    (                                                           \
        local.temp = local.W[( (t) -  3 ) & 0x0F] ^             \
                     local.W[( (t) -  8 ) & 0x0F] ^             \
                     local.W[( (t) - 14 ) & 0x0F] ^             \
                     local.W[  (t)        & 0x0F],              \
        ( local.W[(t) & 0x0F] = S(local.temp,1) )               \
    )

#define P(a,b,c,d,e,x)                                          \
    do                                                          \
    {                                                           \
        (e) += S((a),5) + F((b),(c),(d)) + K + (x);             \
        (b) = S((b),30);                                        \
    } while( 0 )

    local.A = ctx->state[0];
    local.B = ctx->state[1];
    local.C = ctx->state[2];
    local.D = ctx->state[3];
    local.E = ctx->state[4];

#define F(x,y,z) ((z) ^ ((x) & ((y) ^ (z))))
#define K 0x5A827999

    P( local.A, local.B, local.C, local.D, local.E, local.W[0]  );
    P( local.E, local.A, local.B, local.C, local.D, local.W[1]  );
    P( local.D, local.E, local.A, local.B, local.C, local.W[2]  );
    P( local.C, local.D, local.E, local.A, local.B, local.W[3]  );
    P( local.B, local.C, local.D, local.E, local.A, local.W[4]  );
    P( local.A, local.B, local.C, local.D, local.E, local.W[5]  );
    P( local.E, local.A, local.B, local.C, local.D, local.W[6]  );
    P( local.D, local.E, local.A, local.B, local.C, local.W[7]  );
    P( local.C, local.D, local.E, local.A, local.B, local.W[8]  );
    P( local.B, local.C, local.D, local.E, local.A, local.W[9]  );
    P( local.A, local.B, local.C, local.D, local.E, local.W[10] );
    P( local.E, local.A, local.B, local.C, local.D, local.W[11] );
    P( local.D, local.E, local.A, local.B, local.C, local.W[12] );
    P( local.C, local.D, local.E, local.A, local.B, local.W[13] );
    P( local.B, local.C, local.D, local.E, local.A, local.W[14] );
    P( local.A, local.B, local.C, local.D, local.E, local.W[15] );
    P( local.E, local.A, local.B, local.C, local.D, R(16) );
    P( local.D, local.E, local.A, local.B, local.C, R(17) );
    P( local.C, local.D, local.E, local.A, local.B, R(18) );
    P( local.B, local.C, local.D, local.E, local.A, R(19) );

#undef K
#undef F

#define F(x,y,z) ((x) ^ (y) ^ (z))
#define K 0x6ED9EBA1

    P( local.A, local.B, local.C, local.D, local.E, R(20) );
    P( local.E, local.A, local.B, local.C, local.D, R(21) );
    P( local.D, local.E, local.A, local.B, local.C, R(22) );
    P( local.C, local.D, local.E, local.A, local.B, R(23) );
    P( local.B, local.C, local.D, local.E, local.A, R(24) );
    P( local.A, local.B, local.C, local.D, local.E, R(25) );
    P( local.E, local.A, local.B, local.C, local.D, R(26) );
    P( local.D, local.E, local.A, local.B, local.C, R(27) );
    P( local.C, local.D, local.E, local.A, local.B, R(28) );
    P( local.B, local.C, local.D, local.E, local.A, R(29) );
    P( local.A, local.B, local.C, local.D, local.E, R(30) );
    P( local.E, local.A, local.B, local.C, local.D, R(31) );
    P( local.D, local.E, local.A, local.B, local.C, R(32) );
    P( local.C, local.D, local.E, local.A, local.B, R(33) );
    P( local.B, local.C, local.D, local.E, local.A, R(34) );
    P( local.A, local.B, local.C, local.D, local.E, R(35) );
    P( local.E, local.A, local.B, local.C, local.D, R(36) );
    P( local.D, local.E, local.A, local.B, local.C, R(37) );
    P( local.C, local.D, local.E, local.A, local.B, R(38) );
    P( local.B, local.C, local.D, local.E, local.A, R(39) );

#undef K
#undef F

#define F(x,y,z) (((x) & (y)) | ((z) & ((x) | (y))))
#define K 0x8F1BBCDC

    P( local.A, local.B, local.C, local.D, local.E, R(40) );
    P( local.E, local.A, local.B, local.C, local.D, R(41) );
    P( local.D, local.E, local.A, local.B, local.C, R(42) );
    P( local.C, local.D, local.E, local.A, local.B, R(43) );
    P( local.B, local.C, local.D, local.E, local.A, R(44) );
    P( local.A, local.B, local.C, local.D, local.E, R(45) );
    P( local.E, local.A, local.B, local.C, local.D, R(46) );
    P( local.D, local.E, local.A, local.B, local.C, R(47) );
    P( local.C, local.D, local.E, local.A, local.B, R(48) );
    P( local.B, local.C, local.D, local.E, local.A, R(49) );
    P( local.A, local.B, local.C, local.D, local.E, R(50) );
    P( local.E, local.A, local.B, local.C, local.D, R(51) );
    P( local.D, local.E, local.A, local.B, local.C, R(52) );
    P( local.C, local.D, local.E, local.A, local.B, R(53) );
    P( local.B, local.C, local.D, local.E, local.A, R(54) );
    P( local.A, local.B, local.C, local.D, local.E, R(55) );
    P( local.E, local.A, local.B, local.C, local.D, R(56) );
    P( local.D, local.E, local.A, local.B, local.C, R(57) );
    P( local.C, local.D, local.E, local.A, local.B, R(58) );
    P( local.B, local.C, local.D, local.E, local.A, R(59) );

#undef K
#undef F

#define F(x,y,z) ((x) ^ (y) ^ (z))
#define K 0xCA62C1D6

    P( local.A, local.B, local.C, local.D, local.E, R(60) );
    P( local.E, local.A, local.B, local.C, local.D, R(61) );
    P( local.D, local.E, local.A, local.B, local.C, R(62) );
    P( local.C, local.D, local.E, local.A, local.B, R(63) );
    P( local.B, local.C, local.D, local.E, local.A, R(64) );
    P( local.A, local.B, local.C, local.D, local.E, R(65) );
    P( local.E, local.A, local.B, local.C, local.D, R(66) );
    P( local.D, local.E, local.A, local.B, local.C, R(67) );
    P( local.C, local.D, local.E, local.A, local.B, R(68) );
    P( local.B, local.C, local.D, local.E, local.A, R(69) );
    P( local.A, local.B, local.C, local.D, local.E, R(70) );
    P( local.E, local.A, local.B, local.C, local.D, R(71) );
    P( local.D, local.E, local.A, local.B, local.C, R(72) );
    P( local.C, local.D, local.E, local.A, local.B, R(73) );
    P( local.B, local.C, local.D, local.E, local.A, R(74) );
    P( local.A, local.B, local.C, local.D, local.E, R(75) );
    P( local.E, local.A, local.B, local.C, local.D, R(76) );
    P( local.D, local.E, local.A, local.B, local.C, R(77) );
    P( local.C, local.D, local.E, local.A, local.B, R(78) );
    P( local.B, local.C, local.D, local.E, local.A, R(79) );

#undef K
#undef F

    ctx->state[0] += local.A;
    ctx->state[1] += local.B;
    ctx->state[2] += local.C;
    ctx->state[3] += local.D;
    ctx->state[4] += local.E;

    return( 0 );
}

#endif /* !MBEDTLS_SHA1_PROCESS_ALT */

/*
 * SHA-1 process buffer
 */
int mbedtls_sha1_update( mbedtls_sha1_context *ctx,
                             const unsigned char *input,
                             size_t ilen )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t fill;
    uint32_t left;

    SHA1_VALIDATE_RET( ctx != NULL );
    SHA1_VALIDATE_RET( ilen == 0 || input != NULL );

    if( ilen == 0 )
        return( 0 );

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );

        if( ( ret = mbedtls_internal_sha1_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        if( ( ret = mbedtls_internal_sha1_process( ctx, input ) ) != 0 )
            return( ret );

        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );

    return( 0 );
}

/*
 * SHA-1 final digest
 */
int mbedtls_sha1_finish( mbedtls_sha1_context *ctx,
                             unsigned char output[20] )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    uint32_t used;
    uint32_t high, low;

    SHA1_VALIDATE_RET( ctx != NULL );
    SHA1_VALIDATE_RET( (unsigned char *)output != NULL );

    /*
     * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
     */
    used = ctx->total[0] & 0x3F;

    ctx->buffer[used++] = 0x80;

    if( used <= 56 )
    {
        /* Enough room for padding + length in current block */
        memset( ctx->buffer + used, 0, 56 - used );
    }
    else
    {
        /* We'll need an extra block */
        memset( ctx->buffer + used, 0, 64 - used );

        if( ( ret = mbedtls_internal_sha1_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        memset( ctx->buffer, 0, 56 );
    }

    /*
     * Add message length
     */
    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE( high, ctx->buffer, 56 );
    PUT_UINT32_BE( low,  ctx->buffer, 60 );

    if( ( ret = mbedtls_internal_sha1_process( ctx, ctx->buffer ) ) != 0 )
        return( ret );

    /*
     * Output final state
     */
    PUT_UINT32_BE( ctx->state[0], output,  0 );
    PUT_UINT32_BE( ctx->state[1], output,  4 );
    PUT_UINT32_BE( ctx->state[2], output,  8 );
    PUT_UINT32_BE( ctx->state[3], output, 12 );
    PUT_UINT32_BE( ctx->state[4], output, 16 );

    return( 0 );
}

/*
 * output = SHA-1( input buffer )
 */
int mbedtls_sha1( const unsigned char *input,
                      size_t ilen,
                      unsigned char output[20] )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_sha1_context ctx;

    SHA1_VALIDATE_RET( ilen == 0 || input != NULL );
    SHA1_VALIDATE_RET( (unsigned char *)output != NULL );

    mbedtls_sha1_init( &ctx );

    if( ( ret = mbedtls_sha1_starts( &ctx ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha1_update( &ctx, input, ilen ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha1_finish( &ctx, output ) ) != 0 )
        goto exit;

exit:
    mbedtls_sha1_free( &ctx );

    return( ret );
}

/*
 * SHA-1 HMAC context setup
 */
void sha1_hmac_starts( mbedtls_sha1_context *ctx, const unsigned char *key, size_t keylen )
{
    size_t i;
    unsigned char sum[20];

    if( keylen > 64 )
    {
        mbedtls_sha1( key, keylen, sum );
        keylen = 20;
        key = sum;
    }

    memset( ctx->ipad, 0x36, 64 );
    memset( ctx->opad, 0x5C, 64 );

    for( i = 0; i < keylen; i++ )
    {
        ctx->ipad[i] = (unsigned char)( ctx->ipad[i] ^ key[i] );
        ctx->opad[i] = (unsigned char)( ctx->opad[i] ^ key[i] );
    }

    mbedtls_sha1_starts( ctx );
    mbedtls_sha1_update( ctx, ctx->ipad, 64 );

    memset( sum, 0, sizeof( sum ) );
}

/*
 * SHA-1 HMAC process buffer
 */
void sha1_hmac_update( mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen )
{
    mbedtls_sha1_update( ctx, input, ilen );
}

/*
 * SHA-1 HMAC final digest
 */
void sha1_hmac_finish( mbedtls_sha1_context *ctx, unsigned char output[20] )
{
    unsigned char tmpbuf[20];

    mbedtls_sha1_finish( ctx, tmpbuf );
    mbedtls_sha1_starts( ctx );
    mbedtls_sha1_update( ctx, ctx->opad, 64 );
    mbedtls_sha1_update( ctx, tmpbuf, 20 );
    mbedtls_sha1_finish( ctx, output );

    memset( tmpbuf, 0, sizeof( tmpbuf ) );
}

/*
 * SHA1 HMAC context reset
 */
void sha1_hmac_reset( mbedtls_sha1_context *ctx )
{
    mbedtls_sha1_starts( ctx );
    mbedtls_sha1_update( ctx, ctx->ipad, 64 );
}

/*
 * output = HMAC-SHA-1( hmac key, input buffer )
 */
void sha1_hmac( const unsigned char *key, size_t keylen,
                const unsigned char *input, size_t ilen,
                unsigned char output[20] )
{
    mbedtls_sha1_context ctx;

    sha1_hmac_starts( &ctx, key, keylen );
    sha1_hmac_update( &ctx, input, ilen );
    sha1_hmac_finish( &ctx, output );

    memset( &ctx, 0, sizeof( mbedtls_sha1_context ) );
}

#endif /* MBEDTLS_SHA1_C */
