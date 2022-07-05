/***************************************************************************/
/*                                                                         */
/*  afshaper.c                                                             */
/*                                                                         */
/*    HarfBuzz interface for accessing OpenType features (body).           */
/*                                                                         */
/*  Copyright 2013-2018 by                                                 */
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
#include FT_ADVANCES_H
#include "afglobal.h"
#include "aftypes.h"
#include "afshaper.h"


  FT_Error
  af_shaper_get_coverage( AF_FaceGlobals  globals,
                          AF_StyleClass   style_class,
                          FT_UShort*      gstyles,
                          FT_Bool         default_script )
  {
    FT_UNUSED( globals );
    FT_UNUSED( style_class );
    FT_UNUSED( gstyles );
    FT_UNUSED( default_script );

    return FT_Err_Ok;
  }


  void*
  af_shaper_buf_create( FT_Face  face )
  {
    FT_Error   error;
    FT_Memory  memory = face->memory;
    FT_ULong*  buf;


    FT_MEM_ALLOC( buf, sizeof ( FT_ULong ) );

    return (void*)buf;
  }


  void
  af_shaper_buf_destroy( FT_Face  face,
                         void*    buf )
  {
    FT_Memory  memory = face->memory;


    FT_FREE( buf );
  }


  const char*
  af_shaper_get_cluster( const char*      p,
                         AF_StyleMetrics  metrics,
                         void*            buf_,
                         unsigned int*    count )
  {
    FT_Face    face      = metrics->globals->face;
    FT_ULong   ch, dummy = 0;
    FT_ULong*  buf       = (FT_ULong*)buf_;


    while ( *p == ' ' )
      p++;

    GET_UTF8_CHAR( ch, p );

    /* since we don't have an engine to handle clusters, */
    /* we scan the characters but return zero            */
    while ( !( *p == ' ' || *p == '\0' ) )
      GET_UTF8_CHAR( dummy, p );

    if ( dummy )
    {
      *buf   = 0;
      *count = 0;
    }
    else
    {
      *buf   = FT_Get_Char_Index( face, ch );
      *count = 1;
    }

    return p;
  }


  FT_ULong
  af_shaper_get_elem( AF_StyleMetrics  metrics,
                      void*            buf_,
                      unsigned int     idx,
                      FT_Long*         advance,
                      FT_Long*         y_offset )
  {
    FT_Face   face        = metrics->globals->face;
    FT_ULong  glyph_index = *(FT_ULong*)buf_;

    FT_UNUSED( idx );


    if ( advance )
      FT_Get_Advance( face,
                      glyph_index,
                      FT_LOAD_NO_SCALE         |
                      FT_LOAD_NO_HINTING       |
                      FT_LOAD_IGNORE_TRANSFORM,
                      advance );

    if ( y_offset )
      *y_offset = 0;

    return glyph_index;
  }


/* END */
