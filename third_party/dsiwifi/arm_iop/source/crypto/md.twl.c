/**
 * \file md.c
 *
 * \brief Generic message digest wrapper for mbed TLS
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
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

#include "common.h"

#if defined(MBEDTLS_MD_C)

#include "md.h"
#include "md_wrap.h"
#include "platform_util.h"
#include "error.h"

#include "sha1.h"

#include <stdlib.h>
#define mbedtls_calloc    calloc
#define mbedtls_free       free

#include <string.h>

#if defined(MBEDTLS_FS_IO)
#include <stdio.h>
#endif

#if defined(MBEDTLS_SHA1_C)
const mbedtls_md_info_t mbedtls_sha1_info = {
    "SHA1",
    MBEDTLS_MD_SHA1,
    20,
    64,
};
#endif

/*
 * Reminder: update profiles in x509_crt.c when adding a new hash!
 */
static const int supported_digests[] = {

#if defined(MBEDTLS_SHA1_C)
        MBEDTLS_MD_SHA1,
#endif

        MBEDTLS_MD_NONE
};

const int *mbedtls_md_list( void )
{
    return( supported_digests );
}

const mbedtls_md_info_t *mbedtls_md_info_from_string( const char *md_name )
{
    if( NULL == md_name )
        return( NULL );

    /* Get the appropriate digest information */
#if defined(MBEDTLS_SHA1_C)
    if( !strcmp( "SHA1", md_name ) || !strcmp( "SHA", md_name ) )
        return mbedtls_md_info_from_type( MBEDTLS_MD_SHA1 );
#endif
    return( NULL );
}

const mbedtls_md_info_t *mbedtls_md_info_from_type( mbedtls_md_type_t md_type )
{
    switch( md_type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( &mbedtls_sha1_info );
#endif
        default:
            return( NULL );
    }
}

void mbedtls_md_init( mbedtls_md_context_t *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_md_context_t ) );
}

void mbedtls_md_free( mbedtls_md_context_t *ctx )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return;

    if( ctx->md_ctx != NULL )
    {
        switch( ctx->md_info->type )
        {
#if defined(MBEDTLS_SHA1_C)
            case MBEDTLS_MD_SHA1:
                mbedtls_sha1_free( ctx->md_ctx );
                break;
#endif
            default:
                /* Shouldn't happen */
                break;
        }
        mbedtls_free( ctx->md_ctx );
    }

    if( ctx->hmac_ctx != NULL )
    {
        mbedtls_free( ctx->hmac_ctx );
    }
}

int mbedtls_md_clone( mbedtls_md_context_t *dst,
                      const mbedtls_md_context_t *src )
{
    if( dst == NULL || dst->md_info == NULL ||
        src == NULL || src->md_info == NULL ||
        dst->md_info != src->md_info )
    {
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }

    switch( src->md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            mbedtls_sha1_clone( dst->md_ctx, src->md_ctx );
            break;
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }

    return( 0 );
}

#define ALLOC( type )                                                   \
    do {                                                                \
        ctx->md_ctx = mbedtls_calloc( 1, sizeof( mbedtls_##type##_context ) ); \
        if( ctx->md_ctx == NULL )                                       \
            return( MBEDTLS_ERR_MD_ALLOC_FAILED );                      \
        mbedtls_##type##_init( ctx->md_ctx );                           \
    }                                                                   \
    while( 0 )

int mbedtls_md_setup( mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info, int hmac )
{
    if( md_info == NULL || ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info = md_info;
    ctx->md_ctx = NULL;
    ctx->hmac_ctx = NULL;

    switch( md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            ALLOC( sha1 );
            break;
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }

    if( hmac != 0 )
    {
        ctx->hmac_ctx = mbedtls_calloc( 2, md_info->block_size );
        if( ctx->hmac_ctx == NULL )
        {
            mbedtls_md_free( ctx );
            return( MBEDTLS_ERR_MD_ALLOC_FAILED );
        }
    }

    return( 0 );
}
#undef ALLOC

int mbedtls_md_starts( mbedtls_md_context_t *ctx )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    switch( ctx->md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( mbedtls_sha1_starts( ctx->md_ctx ) );
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }
}

int mbedtls_md_update( mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    switch( ctx->md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( mbedtls_sha1_update( ctx->md_ctx, input, ilen ) );
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }
}

int mbedtls_md_finish( mbedtls_md_context_t *ctx, unsigned char *output )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    switch( ctx->md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( mbedtls_sha1_finish( ctx->md_ctx, output ) );
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }
}

int mbedtls_md( const mbedtls_md_info_t *md_info, const unsigned char *input, size_t ilen,
            unsigned char *output )
{
    if( md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    switch( md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( mbedtls_sha1( input, ilen, output ) );
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }
}

int mbedtls_md_hmac_starts( mbedtls_md_context_t *ctx, const unsigned char *key, size_t keylen )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char sum[MBEDTLS_MD_MAX_SIZE];
    unsigned char *ipad, *opad;
    size_t i;

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    if( keylen > (size_t) ctx->md_info->block_size )
    {
        if( ( ret = mbedtls_md_starts( ctx ) ) != 0 )
            goto cleanup;
        if( ( ret = mbedtls_md_update( ctx, key, keylen ) ) != 0 )
            goto cleanup;
        if( ( ret = mbedtls_md_finish( ctx, sum ) ) != 0 )
            goto cleanup;

        keylen = ctx->md_info->size;
        key = sum;
    }

    ipad = (unsigned char *) ctx->hmac_ctx;
    opad = (unsigned char *) ctx->hmac_ctx + ctx->md_info->block_size;

    memset( ipad, 0x36, ctx->md_info->block_size );
    memset( opad, 0x5C, ctx->md_info->block_size );

    for( i = 0; i < keylen; i++ )
    {
        ipad[i] = (unsigned char)( ipad[i] ^ key[i] );
        opad[i] = (unsigned char)( opad[i] ^ key[i] );
    }

    if( ( ret = mbedtls_md_starts( ctx ) ) != 0 )
        goto cleanup;
    if( ( ret = mbedtls_md_update( ctx, ipad,
                                   ctx->md_info->block_size ) ) != 0 )
        goto cleanup;

cleanup:

    return( ret );
}

int mbedtls_md_hmac_update( mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen )
{
    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    return( mbedtls_md_update( ctx, input, ilen ) );
}

int mbedtls_md_hmac_finish( mbedtls_md_context_t *ctx, unsigned char *output )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char tmp[MBEDTLS_MD_MAX_SIZE];
    unsigned char *opad;

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    opad = (unsigned char *) ctx->hmac_ctx + ctx->md_info->block_size;

    if( ( ret = mbedtls_md_finish( ctx, tmp ) ) != 0 )
        return( ret );
    if( ( ret = mbedtls_md_starts( ctx ) ) != 0 )
        return( ret );
    if( ( ret = mbedtls_md_update( ctx, opad,
                                   ctx->md_info->block_size ) ) != 0 )
        return( ret );
    if( ( ret = mbedtls_md_update( ctx, tmp,
                                   ctx->md_info->size ) ) != 0 )
        return( ret );
    return( mbedtls_md_finish( ctx, output ) );
}

int mbedtls_md_hmac_reset( mbedtls_md_context_t *ctx )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *ipad;

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ipad = (unsigned char *) ctx->hmac_ctx;

    if( ( ret = mbedtls_md_starts( ctx ) ) != 0 )
        return( ret );
    return( mbedtls_md_update( ctx, ipad, ctx->md_info->block_size ) );
}

int mbedtls_md_hmac( const mbedtls_md_info_t *md_info,
                     const unsigned char *key, size_t keylen,
                     const unsigned char *input, size_t ilen,
                     unsigned char *output )
{
    mbedtls_md_context_t ctx;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if( md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    mbedtls_md_init( &ctx );

    if( ( ret = mbedtls_md_setup( &ctx, md_info, 1 ) ) != 0 )
        goto cleanup;

    if( ( ret = mbedtls_md_hmac_starts( &ctx, key, keylen ) ) != 0 )
        goto cleanup;
    if( ( ret = mbedtls_md_hmac_update( &ctx, input, ilen ) ) != 0 )
        goto cleanup;
    if( ( ret = mbedtls_md_hmac_finish( &ctx, output ) ) != 0 )
        goto cleanup;

cleanup:
    mbedtls_md_free( &ctx );

    return( ret );
}

int mbedtls_md_process( mbedtls_md_context_t *ctx, const unsigned char *data )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    switch( ctx->md_info->type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( mbedtls_internal_sha1_process( ctx->md_ctx, data ) );
#endif
        default:
            return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }
}

unsigned char mbedtls_md_get_size( const mbedtls_md_info_t *md_info )
{
    if( md_info == NULL )
        return( 0 );

    return md_info->size;
}

mbedtls_md_type_t mbedtls_md_get_type( const mbedtls_md_info_t *md_info )
{
    if( md_info == NULL )
        return( MBEDTLS_MD_NONE );

    return md_info->type;
}

const char *mbedtls_md_get_name( const mbedtls_md_info_t *md_info )
{
    if( md_info == NULL )
        return( NULL );

    return md_info->name;
}

#endif /* MBEDTLS_MD_C */
