/***************************************************************************/
/*                                                                         */
/*  t1load.h                                                               */
/*                                                                         */
/*    Type 1 font loader (specification).                                  */
/*                                                                         */
/*  Copyright 1996-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef T1LOAD_H_
#define T1LOAD_H_


#include "ft2build.h"
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H

#include "t1parse.h"


FT_BEGIN_HEADER


  typedef struct  T1_Loader_
  {
    T1_ParserRec  parser;          /* parser used to read the stream */

    FT_Int        num_chars;       /* number of characters in encoding */
    PS_TableRec   encoding_table;  /* PS_Table used to store the       */
                                   /* encoding character names         */

    FT_Int        num_glyphs;
    PS_TableRec   glyph_names;
    PS_TableRec   charstrings;
    PS_TableRec   swap_table;      /* For moving .notdef glyph to index 0. */

    FT_Int        num_subrs;
    PS_TableRec   subrs;
    FT_Hash       subrs_hash;
    FT_Bool       fontdata;

    FT_UInt       keywords_encountered; /* T1_LOADER_ENCOUNTERED_XXX */

  } T1_LoaderRec, *T1_Loader;


  /* treatment of some keywords differs depending on whether */
  /* they precede or follow certain other keywords           */

#define T1_PRIVATE                ( 1 << 0 )
#define T1_FONTDIR_AFTER_PRIVATE  ( 1 << 1 )


  FT_LOCAL( FT_Error )
  T1_Open_Face( T1_Face  face );


FT_END_HEADER

#endif /* T1LOAD_H_ */


/* END */
