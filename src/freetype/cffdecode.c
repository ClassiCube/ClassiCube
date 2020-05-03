/***************************************************************************/
/*                                                                         */
/*  cffdecode.c                                                            */
/*                                                                         */
/*    PostScript CFF (Type 2) decoding routines (body).                    */
/*                                                                         */
/*  Copyright 2017-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_SERVICE_H
#include FT_SERVICE_CFF_TABLE_LOAD_H

#include "cffdecode.h"
#include "psobjs.h"

#include "psauxerr.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffdecode


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********             GENERIC CHARSTRING PARSING               *********/
  /**********                                                      *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_compute_bias                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the bias value in dependence of the number of glyph       */
  /*    subroutines.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    in_charstring_type :: The `CharstringType' value of the top DICT   */
  /*                          dictionary.                                  */
  /*                                                                       */
  /*    num_subrs          :: The number of glyph subroutines.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The bias value.                                                    */
  static FT_Int
  cff_compute_bias( FT_Int   in_charstring_type,
                    FT_UInt  num_subrs )
  {
    FT_Int  result;


    if ( in_charstring_type == 1 )
      result = 0;
    else if ( num_subrs < 1240 )
      result = 107;
    else if ( num_subrs < 33900U )
      result = 1131;
    else
      result = 32768U;

    return result;
  }


  FT_LOCAL_DEF( FT_Int )
  cff_lookup_glyph_by_stdcharcode( CFF_Font  cff,
                                   FT_Int    charcode )
  {
    FT_UInt    n;
    FT_UShort  glyph_sid;

    FT_Service_CFFLoad  cffload;


    /* CID-keyed fonts don't have glyph names */
    if ( !cff->charset.sids )
      return -1;

    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    cffload = (FT_Service_CFFLoad)cff->cffload;

    /* Get code to SID mapping from `cff_standard_encoding'. */
    glyph_sid = cffload->get_standard_encoding( (FT_UInt)charcode );

    for ( n = 0; n < cff->num_glyphs; n++ )
    {
      if ( cff->charset.sids[n] == glyph_sid )
        return (FT_Int)n;
    }

    return -1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_decoder_init                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph decoder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face      :: The current face object.                              */
  /*                                                                       */
  /*    size      :: The current size object.                              */
  /*                                                                       */
  /*    slot      :: The current glyph object.                             */
  /*                                                                       */
  /*    hinting   :: Whether hinting is active.                            */
  /*                                                                       */
  /*    hint_mode :: The hinting mode.                                     */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  cff_decoder_init( CFF_Decoder*                     decoder,
                    TT_Face                          face,
                    CFF_Size                         size,
                    CFF_GlyphSlot                    slot,
                    FT_Bool                          hinting,
                    FT_Render_Mode                   hint_mode,
                    CFF_Decoder_Get_Glyph_Callback   get_callback,
                    CFF_Decoder_Free_Glyph_Callback  free_callback )
  {
    CFF_Font  cff = (CFF_Font)face->extra.data;


    /* clear everything */
    FT_ZERO( decoder );

    /* initialize builder */
    cff_builder_init( &decoder->builder, face, size, slot, hinting );

    /* initialize Type2 decoder */
    decoder->cff          = cff;
    decoder->num_globals  = cff->global_subrs_index.count;
    decoder->globals      = cff->global_subrs;
    decoder->globals_bias = cff_compute_bias(
                              cff->top_font.font_dict.charstring_type,
                              decoder->num_globals );

    decoder->hint_mode = hint_mode;

    decoder->get_glyph_callback  = get_callback;
    decoder->free_glyph_callback = free_callback;
  }


  /* this function is used to select the subfont */
  /* and the locals subrs array                  */
  FT_LOCAL_DEF( FT_Error )
  cff_decoder_prepare( CFF_Decoder*  decoder,
                       CFF_Size      size,
                       FT_UInt       glyph_index )
  {
    CFF_Builder  *builder = &decoder->builder;
    CFF_Font      cff     = (CFF_Font)builder->face->extra.data;
    CFF_SubFont   sub     = &cff->top_font;
    FT_Error      error   = FT_Err_Ok;

    FT_Service_CFFLoad  cffload = (FT_Service_CFFLoad)cff->cffload;


    /* manage CID fonts */
    if ( cff->num_subfonts )
    {
      FT_Byte  fd_index = cffload->fd_select_get( &cff->fd_select,
                                                  glyph_index );


      if ( fd_index >= cff->num_subfonts )
      {
        FT_TRACE4(( "cff_decoder_prepare: invalid CID subfont index\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }

      FT_TRACE3(( "  in subfont %d:\n", fd_index ));

      sub = cff->subfonts[fd_index];

      if ( builder->hints_funcs && size )
      {
        FT_Size       ftsize   = FT_SIZE( size );
        CFF_Internal  internal = (CFF_Internal)ftsize->internal->module_data;


        /* for CFFs without subfonts, this value has already been set */
        builder->hints_globals = (void *)internal->subfonts[fd_index];
      }
    }

    decoder->num_locals  = sub->local_subrs_index.count;
    decoder->locals      = sub->local_subrs;
    decoder->locals_bias = cff_compute_bias(
                             decoder->cff->top_font.font_dict.charstring_type,
                             decoder->num_locals );

    decoder->glyph_width   = sub->private_dict.default_width;
    decoder->nominal_width = sub->private_dict.nominal_width;

    decoder->current_subfont = sub;

  Exit:
    return error;
  }


/* END */
