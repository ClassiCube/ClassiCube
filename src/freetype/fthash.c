/***************************************************************************/
/*                                                                         */
/*  fthash.c                                                               */
/*                                                                         */
/*    Hashing functions (body).                                            */
/*                                                                         */
/***************************************************************************/

/*
 * Copyright 2000 Computing Research Labs, New Mexico State University
 * Copyright 2001-2015
 *   Francesco Zappa Nardelli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

  /*************************************************************************/
  /*                                                                       */
  /*  This file is based on code from bdf.c,v 1.22 2000/03/16 20:08:50     */
  /*                                                                       */
  /*  taken from Mark Leisher's xmbdfed package                            */
  /*                                                                       */
  /*************************************************************************/


#include "ft2build.h"
#include FT_INTERNAL_HASH_H
#include FT_INTERNAL_MEMORY_H


#define INITIAL_HT_SIZE  241


  static FT_ULong
  hash_num_lookup( FT_Int  key )
  {
    FT_ULong  num = (FT_ULong)key;
    FT_ULong  res;


    /* Mocklisp hash function. */
    res = num & 0xFF;
    res = ( res << 5 ) - res + ( ( num >>  8 ) & 0xFF );
    res = ( res << 5 ) - res + ( ( num >> 16 ) & 0xFF );
    res = ( res << 5 ) - res + ( ( num >> 24 ) & 0xFF );

    return res;
  }


  static FT_Bool
  hash_num_compare( FT_Int  a,
                    FT_Int  b )
  {
    if ( a == b )
      return 1;

    return 0;
  }


  static FT_Hashnode*
  hash_bucket( FT_Int   key,
               FT_Hash  hash )
  {
    FT_ULong      res = 0;
    FT_Hashnode*  bp  = hash->table;
    FT_Hashnode*  ndp;

    res = hash_num_lookup( key );


    ndp = bp + ( res % hash->size );
    while ( *ndp )
    {
      if ( hash_num_compare( (*ndp)->key, key ) )
        break;

      ndp--;
      if ( ndp < bp )
        ndp = bp + ( hash->size - 1 );
    }

    return ndp;
  }


  static FT_Error
  hash_rehash( FT_Hash    hash,
               FT_Memory  memory )
  {
    FT_Hashnode*  obp = hash->table;
    FT_Hashnode*  bp;
    FT_Hashnode*  nbp;

    FT_UInt   i, sz = hash->size;
    FT_Error  error = FT_Err_Ok;


    hash->size <<= 1;
    hash->limit  = hash->size / 3;

    if ( FT_NEW_ARRAY( hash->table, hash->size ) )
      goto Exit;

    for ( i = 0, bp = obp; i < sz; i++, bp++ )
    {
      if ( *bp )
      {
        nbp = hash_bucket( (*bp)->key, hash );
        *nbp = *bp;
      }
    }

    FT_FREE( obp );

  Exit:
    return error;
  }


  FT_Error
  ft_hash_num_init( FT_Hash    hash,
                    FT_Memory  memory )
  {
    FT_UInt   sz = INITIAL_HT_SIZE;
    FT_Error  error;

    hash->size  = sz;
    hash->limit = sz / 3;
    hash->used  = 0;

    FT_MEM_NEW_ARRAY( hash->table, sz );

    return error;
  }


  void
  ft_hash_num_free( FT_Hash    hash,
                    FT_Memory  memory )
  {
    if ( hash )
    {
      FT_UInt       sz = hash->size;
      FT_Hashnode*  bp = hash->table;
      FT_UInt       i;


      for ( i = 0; i < sz; i++, bp++ )
        FT_FREE( *bp );

      FT_FREE( hash->table );
    }
  }


  FT_Error
  ft_hash_num_insert( FT_Int     num,
                      size_t     data,
                      FT_Hash    hash,
                      FT_Memory  memory )
  {
    FT_Hashnode   nn;
    FT_Hashnode*  bp    = hash_bucket( num, hash );
    FT_Error      error = FT_Err_Ok;


    nn = *bp;
    if ( !nn )
    {
      if ( FT_NEW( nn ) )
        goto Exit;
      *bp = nn;

      nn->key  = num;
      nn->data = data;

      if ( hash->used >= hash->limit )
      {
        error = hash_rehash( hash, memory );
        if ( error )
          goto Exit;
      }

      hash->used++;
    }
    else
      nn->data = data;

  Exit:
    return error;
  }


  size_t*
  ft_hash_num_lookup( FT_Int   num,
                      FT_Hash  hash )
  {
    FT_Hashnode*  np = hash_bucket( num, hash );


    return (*np) ? &(*np)->data
                 : NULL;
  }


/* END */
