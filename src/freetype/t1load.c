/***************************************************************************/
/*                                                                         */
/*  t1load.c                                                               */
/*                                                                         */
/*    Type 1 font loader (body).                                           */
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


  /*************************************************************************/
  /*                                                                       */
  /* This is the new and improved Type 1 data loader for FreeType 2.  The  */
  /* old loader has several problems: it is slow, complex, difficult to    */
  /* maintain, and contains incredible hacks to make it accept some        */
  /* ill-formed Type 1 fonts without hiccup-ing.  Moreover, about 5% of    */
  /* the Type 1 fonts on my machine still aren't loaded correctly by it.   */
  /*                                                                       */
  /* This version is much simpler, much faster and also easier to read and */
  /* maintain by a great order of magnitude.  The idea behind it is to     */
  /* _not_ try to read the Type 1 token stream with a state machine (i.e.  */
  /* a Postscript-like interpreter) but rather to perform simple pattern   */
  /* matching.                                                             */
  /*                                                                       */
  /* Indeed, nearly all data definitions follow a simple pattern like      */
  /*                                                                       */
  /*  ... /Field <data> ...                                                */
  /*                                                                       */
  /* where <data> can be a number, a boolean, a string, or an array of     */
  /* numbers.  There are a few exceptions, namely the encoding, font name, */
  /* charstrings, and subrs; they are handled with a special pattern       */
  /* matching routine.                                                     */
  /*                                                                       */
  /* All other common cases are handled very simply.  The matching rules   */
  /* are defined in the file `t1tokens.h' through the use of several       */
  /* macros calls PARSE_XXX.  This file is included twice here; the first  */
  /* time to generate parsing callback functions, the second time to       */
  /* generate a table of keywords (with pointers to the associated         */
  /* callback functions).                                                  */
  /*                                                                       */
  /* The function `parse_dict' simply scans *linearly* a given dictionary  */
  /* (either the top-level or private one) and calls the appropriate       */
  /* callback when it encounters an immediate keyword.                     */
  /*                                                                       */
  /* This is by far the fastest way one can find to parse and read all     */
  /* data.                                                                 */
  /*                                                                       */
  /* This led to tremendous code size reduction.  Note that later, the     */
  /* glyph loader will also be _greatly_ simplified, and the automatic     */
  /* hinter will replace the clumsy `t1hinter'.                            */
  /*                                                                       */
  /*************************************************************************/


#include "ft2build.h"
#include FT_INTERNAL_DEBUG_H
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_TYPE1_TYPES_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_HASH_H

#include "t1load.h"
#include "t1errors.h"


#ifdef FT_CONFIG_OPTION_INCREMENTAL
#define IS_INCREMENTAL  (FT_Bool)( face->root.internal->incremental_interface != 0 )
#else
#define IS_INCREMENTAL  0
#endif


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1load


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      TYPE 1 SYMBOL PARSING                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Error
  t1_load_keyword( T1_Face         face,
                   T1_Loader       loader,
                   const T1_Field  field )
  {
    FT_Error  error;
    void*     dummy_object;
    void**    objects;
    FT_UInt   max_objects;
    PS_Blend  blend = face->blend;


    if ( blend && blend->num_designs == 0 )
      blend = NULL;

    /* if the keyword has a dedicated callback, call it */
    if ( field->type == T1_FIELD_TYPE_CALLBACK )
    {
      field->reader( (FT_Face)face, loader );
      error = loader->parser.root.error;
      goto Exit;
    }

    /* now, the keyword is either a simple field, or a table of fields; */
    /* we are now going to take care of it                              */
    switch ( field->location )
    {
    case T1_FIELD_LOCATION_FONT_INFO:
      dummy_object = &face->type1.font_info;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->font_infos;
        max_objects = blend->num_designs;
      }
      break;

    case T1_FIELD_LOCATION_FONT_EXTRA:
      dummy_object = &face->type1.font_extra;
      objects      = &dummy_object;
      max_objects  = 0;
      break;

    case T1_FIELD_LOCATION_PRIVATE:
      dummy_object = &face->type1.private_dict;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->privates;
        max_objects = blend->num_designs;
      }
      break;

    case T1_FIELD_LOCATION_BBOX:
      dummy_object = &face->type1.font_bbox;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->bboxes;
        max_objects = blend->num_designs;
      }
      break;

    case T1_FIELD_LOCATION_LOADER:
      dummy_object = loader;
      objects      = &dummy_object;
      max_objects  = 0;
      break;

    case T1_FIELD_LOCATION_FACE:
      dummy_object = face;
      objects      = &dummy_object;
      max_objects  = 0;
      break;

    default:
      dummy_object = &face->type1;
      objects      = &dummy_object;
      max_objects  = 0;
    }

    if ( *objects )
    {
      if ( field->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
           field->type == T1_FIELD_TYPE_FIXED_ARRAY   )
        error = T1_Load_Field_Table( &loader->parser, field,
                                     objects, max_objects, 0 );
      else
        error = T1_Load_Field( &loader->parser, field,
                               objects, max_objects, 0 );
    }
    else
    {
      FT_TRACE1(( "t1_load_keyword: ignoring keyword `%s'"
                  " which is not valid at this point\n"
                  "                 (probably due to missing keywords)\n",
                 field->ident ));
      error = FT_Err_Ok;
    }

  Exit:
    return error;
  }


  static void
  parse_private( T1_Face    face,
                 T1_Loader  loader )
  {
    FT_UNUSED( face );

    loader->keywords_encountered |= T1_PRIVATE;
  }


  /* return 1 in case of success */

  static int
  read_binary_data( T1_Parser  parser,
                    FT_ULong*  size,
                    FT_Byte**  base,
                    FT_Bool    incremental )
  {
    FT_Byte*  cur;
    FT_Byte*  limit = parser->root.limit;


    /* the binary data has one of the following formats */
    /*                                                  */
    /*   `size' [white*] RD white ....... ND            */
    /*   `size' [white*] -| white ....... |-            */
    /*                                                  */

    T1_Skip_Spaces( parser );

    cur = parser->root.cursor;

    if ( cur < limit && ft_isdigit( *cur ) )
    {
      FT_Long  s = T1_ToInt( parser );


      T1_Skip_PS_Token( parser );   /* `RD' or `-|' or something else */

      /* there is only one whitespace char after the */
      /* `RD' or `-|' token                          */
      *base = parser->root.cursor + 1;

      if ( s >= 0 && s < limit - *base )
      {
        parser->root.cursor += s + 1;
        *size = (FT_ULong)s;
        return !parser->root.error;
      }
    }

    if( !incremental )
    {
      FT_ERROR(( "read_binary_data: invalid size field\n" ));
      parser->root.error = FT_THROW( Invalid_File_Format );
    }

    return 0;
  }


  /* We now define the routines to handle the `/Encoding', `/Subrs', */
  /* and `/CharStrings' dictionaries.                                */

  static void
  t1_parse_font_matrix( T1_Face    face,
                        T1_Loader  loader )
  {
    T1_Parser   parser = &loader->parser;
    FT_Matrix*  matrix = &face->type1.font_matrix;
    FT_Vector*  offset = &face->type1.font_offset;
    FT_Face     root   = (FT_Face)&face->root;
    FT_Fixed    temp[6];
    FT_Fixed    temp_scale;
    FT_Int      result;


    /* input is scaled by 1000 to accommodate default FontMatrix */
    result = T1_ToFixedArray( parser, 6, temp, 3 );

    if ( result < 6 )
    {
      parser->root.error = FT_THROW( Invalid_File_Format );
      return;
    }

    temp_scale = FT_ABS( temp[3] );

    if ( temp_scale == 0 )
    {
      FT_ERROR(( "t1_parse_font_matrix: invalid font matrix\n" ));
      parser->root.error = FT_THROW( Invalid_File_Format );
      return;
    }

    /* atypical case */
    if ( temp_scale != 0x10000L )
    {
      /* set units per EM based on FontMatrix values */
      root->units_per_EM = (FT_UShort)FT_DivFix( 1000, temp_scale );

      temp[0] = FT_DivFix( temp[0], temp_scale );
      temp[1] = FT_DivFix( temp[1], temp_scale );
      temp[2] = FT_DivFix( temp[2], temp_scale );
      temp[4] = FT_DivFix( temp[4], temp_scale );
      temp[5] = FT_DivFix( temp[5], temp_scale );
      temp[3] = temp[3] < 0 ? -0x10000L : 0x10000L;
    }

    matrix->xx = temp[0];
    matrix->yx = temp[1];
    matrix->xy = temp[2];
    matrix->yy = temp[3];

    /* note that the offsets must be expressed in integer font units */
    offset->x = temp[4] >> 16;
    offset->y = temp[5] >> 16;
  }


  static void
  parse_encoding( T1_Face    face,
                  T1_Loader  loader )
  {
    T1_Parser  parser = &loader->parser;
    FT_Byte*   cur;
    FT_Byte*   limit  = parser->root.limit;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    T1_Skip_Spaces( parser );
    cur = parser->root.cursor;
    if ( cur >= limit )
    {
      FT_ERROR(( "parse_encoding: out of bounds\n" ));
      parser->root.error = FT_THROW( Invalid_File_Format );
      return;
    }

    /* if we have a number or `[', the encoding is an array, */
    /* and we must load it now                               */
    if ( ft_isdigit( *cur ) || *cur == '[' )
    {
      T1_Encoding  encode          = &face->type1.encoding;
      FT_Int       count, array_size, n;
      PS_Table     char_table      = &loader->encoding_table;
      FT_Memory    memory          = parser->root.memory;
      FT_Error     error;
      FT_Bool      only_immediates = 0;


      /* read the number of entries in the encoding; should be 256 */
      if ( *cur == '[' )
      {
        count           = 256;
        only_immediates = 1;
        parser->root.cursor++;
      }
      else
        count = (FT_Int)T1_ToInt( parser );

      array_size = count;
      if ( count > 256 )
      {
        FT_TRACE2(( "parse_encoding:"
                    " only using first 256 encoding array entries\n" ));
        array_size = 256;
      }

      T1_Skip_Spaces( parser );
      if ( parser->root.cursor >= limit )
        return;

      /* PostScript happily allows overwriting of encoding arrays */
      if ( encode->char_index )
      {
        FT_FREE( encode->char_index );
        FT_FREE( encode->char_name );
        T1_Release_Table( char_table );
      }

      /* we use a T1_Table to store our charnames */
      loader->num_chars = encode->num_chars = array_size;
      if ( FT_NEW_ARRAY( encode->char_index, array_size )     ||
           FT_NEW_ARRAY( encode->char_name,  array_size )     ||
           FT_SET_ERROR( psaux->ps_table_funcs->init(
                           char_table, array_size, memory ) ) )
      {
        parser->root.error = error;
        return;
      }

      /* We need to `zero' out encoding_table.elements */
      for ( n = 0; n < array_size; n++ )
      {
        char*  notdef = (char *)".notdef";


        (void)T1_Add_Table( char_table, n, notdef, 8 );
      }

      /* Now we need to read records of the form                */
      /*                                                        */
      /*   ... charcode /charname ...                           */
      /*                                                        */
      /* for each entry in our table.                           */
      /*                                                        */
      /* We simply look for a number followed by an immediate   */
      /* name.  Note that this ignores correctly the sequence   */
      /* that is often seen in type1 fonts:                     */
      /*                                                        */
      /*   0 1 255 { 1 index exch /.notdef put } for dup        */
      /*                                                        */
      /* used to clean the encoding array before anything else. */
      /*                                                        */
      /* Alternatively, if the array is directly given as       */
      /*                                                        */
      /*   /Encoding [ ... ]                                    */
      /*                                                        */
      /* we only read immediates.                               */

      n = 0;
      T1_Skip_Spaces( parser );

      while ( parser->root.cursor < limit )
      {
        cur = parser->root.cursor;

        /* we stop when we encounter a `def' or `]' */
        if ( *cur == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e'         &&
               cur[2] == 'f'         &&
               IS_PS_DELIM( cur[3] ) )
          {
            FT_TRACE6(( "encoding end\n" ));
            cur += 3;
            break;
          }
        }
        if ( *cur == ']' )
        {
          FT_TRACE6(( "encoding end\n" ));
          cur++;
          break;
        }

        /* check whether we've found an entry */
        if ( ft_isdigit( *cur ) || only_immediates )
        {
          FT_Int  charcode;


          if ( only_immediates )
            charcode = n;
          else
          {
            charcode = (FT_Int)T1_ToInt( parser );
            T1_Skip_Spaces( parser );

            /* protect against invalid charcode */
            if ( cur == parser->root.cursor )
            {
              parser->root.error = FT_THROW( Unknown_File_Format );
              return;
            }
          }

          cur = parser->root.cursor;

          if ( cur + 2 < limit && *cur == '/' && n < count )
          {
            FT_UInt  len;


            cur++;

            parser->root.cursor = cur;
            T1_Skip_PS_Token( parser );
            if ( parser->root.cursor >= limit )
              return;
            if ( parser->root.error )
              return;

            len = (FT_UInt)( parser->root.cursor - cur );

            if ( n < array_size )
            {
              parser->root.error = T1_Add_Table( char_table, charcode,
                                                 cur, len + 1 );
              if ( parser->root.error )
                return;
              char_table->elements[charcode][len] = '\0';
            }

            n++;
          }
          else if ( only_immediates )
          {
            /* Since the current position is not updated for           */
            /* immediates-only mode we would get an infinite loop if   */
            /* we don't do anything here.                              */
            /*                                                         */
            /* This encoding array is not valid according to the type1 */
            /* specification (it might be an encoding for a CID type1  */
            /* font, however), so we conclude that this font is NOT a  */
            /* type1 font.                                             */
            parser->root.error = FT_THROW( Unknown_File_Format );
            return;
          }
        }
        else
        {
          T1_Skip_PS_Token( parser );
          if ( parser->root.error )
            return;
        }

        T1_Skip_Spaces( parser );
      }

      face->type1.encoding_type = T1_ENCODING_TYPE_ARRAY;
      parser->root.cursor       = cur;
    }

    /* Otherwise, we should have either `StandardEncoding', */
    /* `ExpertEncoding', or `ISOLatin1Encoding'             */
    else
    {
      if ( cur + 17 < limit                                            &&
           ft_strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_STANDARD;

      else if ( cur + 15 < limit                                          &&
                ft_strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_EXPERT;

      else if ( cur + 18 < limit                                             &&
                ft_strncmp( (const char*)cur, "ISOLatin1Encoding", 17 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_ISOLATIN1;

      else
        parser->root.error = FT_ERR( Ignore );
    }
  }


  static void
  parse_subrs( T1_Face    face,
               T1_Loader  loader )
  {
    T1_Parser  parser = &loader->parser;
    PS_Table   table  = &loader->subrs;
    FT_Memory  memory = parser->root.memory;
    FT_Error   error;
    FT_Int     num_subrs;
    FT_UInt    count;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    T1_Skip_Spaces( parser );

    /* test for empty array */
    if ( parser->root.cursor < parser->root.limit &&
         *parser->root.cursor == '['              )
    {
      T1_Skip_PS_Token( parser );
      T1_Skip_Spaces  ( parser );
      if ( parser->root.cursor >= parser->root.limit ||
           *parser->root.cursor != ']'               )
        parser->root.error = FT_THROW( Invalid_File_Format );
      return;
    }

    num_subrs = (FT_Int)T1_ToInt( parser );
    if ( num_subrs < 0 )
    {
      parser->root.error = FT_THROW( Invalid_File_Format );
      return;
    }

    /* we certainly need more than 8 bytes per subroutine */
    if ( parser->root.limit >= parser->root.cursor                     &&
         num_subrs > ( parser->root.limit - parser->root.cursor ) >> 3 )
    {
      /*
       * There are two possibilities.  Either the font contains an invalid
       * value for `num_subrs', or we have a subsetted font where the
       * subroutine indices are not adjusted, e.g.
       *
       *   /Subrs 812 array
       *     dup 0 { ... } NP
       *     dup 51 { ... } NP
       *     dup 681 { ... } NP
       *   ND
       *
       * In both cases, we use a number hash that maps from subr indices to
       * actual array elements.
       */

      FT_TRACE0(( "parse_subrs: adjusting number of subroutines"
                  " (from %d to %d)\n",
                  num_subrs,
                  ( parser->root.limit - parser->root.cursor ) >> 3 ));
      num_subrs = ( parser->root.limit - parser->root.cursor ) >> 3;

      if ( !loader->subrs_hash )
      {
        if ( FT_NEW( loader->subrs_hash ) )
          goto Fail;

        error = ft_hash_num_init( loader->subrs_hash, memory );
        if ( error )
          goto Fail;
      }
    }

    /* position the parser right before the `dup' of the first subr */
    T1_Skip_PS_Token( parser );         /* `array' */
    if ( parser->root.error )
      return;
    T1_Skip_Spaces( parser );

    /* initialize subrs array -- with synthetic fonts it is possible */
    /* we get here twice                                             */
    if ( !loader->num_subrs )
    {
      error = psaux->ps_table_funcs->init( table, num_subrs, memory );
      if ( error )
        goto Fail;
    }

    /* the format is simple:   */
    /*                         */
    /*   `index' + binary data */
    /*                         */
    for ( count = 0; ; count++ )
    {
      FT_Long   idx;
      FT_ULong  size;
      FT_Byte*  base;


      /* If we are out of data, or if the next token isn't `dup', */
      /* we are done.                                             */
      if ( parser->root.cursor + 4 >= parser->root.limit          ||
          ft_strncmp( (char*)parser->root.cursor, "dup", 3 ) != 0 )
        break;

      T1_Skip_PS_Token( parser );       /* `dup' */

      idx = T1_ToInt( parser );

      if ( !read_binary_data( parser, &size, &base, IS_INCREMENTAL ) )
        return;

      /* The binary string is followed by one token, e.g. `NP' */
      /* (bound to `noaccess put') or by two separate tokens:  */
      /* `noaccess' & `put'.  We position the parser right     */
      /* before the next `dup', if any.                        */
      T1_Skip_PS_Token( parser );   /* `NP' or `|' or `noaccess' */
      if ( parser->root.error )
        return;
      T1_Skip_Spaces  ( parser );

      if ( parser->root.cursor + 4 < parser->root.limit            &&
           ft_strncmp( (char*)parser->root.cursor, "put", 3 ) == 0 )
      {
        T1_Skip_PS_Token( parser ); /* skip `put' */
        T1_Skip_Spaces  ( parser );
      }

      /* if we use a hash, the subrs index is the key, and a running */
      /* counter specified for `T1_Add_Table' acts as the value      */
      if ( loader->subrs_hash )
      {
        ft_hash_num_insert( idx, count, loader->subrs_hash, memory );
        idx = count;
      }

      /* with synthetic fonts it is possible we get here twice */
      if ( loader->num_subrs )
        continue;

      /* some fonts use a value of -1 for lenIV to indicate that */
      /* the charstrings are unencoded                           */
      /*                                                         */
      /* thanks to Tom Kacvinsky for pointing this out           */
      /*                                                         */
      if ( face->type1.private_dict.lenIV >= 0 )
      {
        FT_Byte*  temp = NULL;


        /* some fonts define empty subr records -- this is not totally */
        /* compliant to the specification (which says they should at   */
        /* least contain a `return'), but we support them anyway       */
        if ( size < (FT_ULong)face->type1.private_dict.lenIV )
        {
          error = FT_THROW( Invalid_File_Format );
          goto Fail;
        }

        /* t1_decrypt() shouldn't write to base -- make temporary copy */
        if ( FT_ALLOC( temp, size ) )
          goto Fail;
        FT_MEM_COPY( temp, base, size );
        psaux->t1_decrypt( temp, size, 4330 );
        size -= (FT_ULong)face->type1.private_dict.lenIV;
        error = T1_Add_Table( table, (FT_Int)idx,
                              temp + face->type1.private_dict.lenIV, size );
        FT_FREE( temp );
      }
      else
        error = T1_Add_Table( table, (FT_Int)idx, base, size );
      if ( error )
        goto Fail;
    }

    if ( !loader->num_subrs )
      loader->num_subrs = num_subrs;

    return;

  Fail:
    parser->root.error = error;
  }


#define TABLE_EXTEND  5


  static void
  parse_charstrings( T1_Face    face,
                     T1_Loader  loader )
  {
    T1_Parser      parser       = &loader->parser;
    PS_Table       code_table   = &loader->charstrings;
    PS_Table       name_table   = &loader->glyph_names;
    PS_Table       swap_table   = &loader->swap_table;
    FT_Memory      memory       = parser->root.memory;
    FT_Error       error;

    PSAux_Service  psaux        = (PSAux_Service)face->psaux;

    FT_Byte*       cur          = parser->root.cursor;
    FT_Byte*       limit        = parser->root.limit;
    FT_Int         n, num_glyphs;
    FT_Int         notdef_index = 0;
    FT_Byte        notdef_found = 0;


    num_glyphs = (FT_Int)T1_ToInt( parser );
    if ( num_glyphs < 0 )
    {
      error = FT_THROW( Invalid_File_Format );
      goto Fail;
    }

    /* we certainly need more than 8 bytes per glyph */
    if ( num_glyphs > ( limit - cur ) >> 3 )
    {
      FT_TRACE0(( "parse_charstrings: adjusting number of glyphs"
                  " (from %d to %d)\n",
                  num_glyphs, ( limit - cur ) >> 3 ));
      num_glyphs = ( limit - cur ) >> 3;
    }

    /* some fonts like Optima-Oblique not only define the /CharStrings */
    /* array but access it also                                        */
    if ( num_glyphs == 0 || parser->root.error )
      return;

    /* initialize tables, leaving space for addition of .notdef, */
    /* if necessary, and a few other glyphs to handle buggy      */
    /* fonts which have more glyphs than specified.              */

    /* for some non-standard fonts like `Optima' which provides  */
    /* different outlines depending on the resolution it is      */
    /* possible to get here twice                                */
    if ( !loader->num_glyphs )
    {
      error = psaux->ps_table_funcs->init(
                code_table, num_glyphs + 1 + TABLE_EXTEND, memory );
      if ( error )
        goto Fail;

      error = psaux->ps_table_funcs->init(
                name_table, num_glyphs + 1 + TABLE_EXTEND, memory );
      if ( error )
        goto Fail;

      /* Initialize table for swapping index notdef_index and */
      /* index 0 names and codes (if necessary).              */

      error = psaux->ps_table_funcs->init( swap_table, 4, memory );
      if ( error )
        goto Fail;
    }

    n = 0;

    for (;;)
    {
      FT_ULong  size;
      FT_Byte*  base;


      /* the format is simple:        */
      /*   `/glyphname' + binary data */

      T1_Skip_Spaces( parser );

      cur = parser->root.cursor;
      if ( cur >= limit )
        break;

      /* we stop when we find a `def' or `end' keyword */
      if ( cur + 3 < limit && IS_PS_DELIM( cur[3] ) )
      {
        if ( cur[0] == 'd' &&
             cur[1] == 'e' &&
             cur[2] == 'f' )
        {
          /* There are fonts which have this: */
          /*                                  */
          /*   /CharStrings 118 dict def      */
          /*   Private begin                  */
          /*   CharStrings begin              */
          /*   ...                            */
          /*                                  */
          /* To catch this we ignore `def' if */
          /* no charstring has actually been  */
          /* seen.                            */
          if ( n )
            break;
        }

        if ( cur[0] == 'e' &&
             cur[1] == 'n' &&
             cur[2] == 'd' )
          break;
      }

      T1_Skip_PS_Token( parser );
      if ( parser->root.cursor >= limit )
      {
        error = FT_THROW( Invalid_File_Format );
        goto Fail;
      }
      if ( parser->root.error )
        return;

      if ( *cur == '/' )
      {
        FT_UInt  len;


        if ( cur + 2 >= limit )
        {
          error = FT_THROW( Invalid_File_Format );
          goto Fail;
        }

        cur++;                              /* skip `/' */
        len = (FT_UInt)( parser->root.cursor - cur );

        if ( !read_binary_data( parser, &size, &base, IS_INCREMENTAL ) )
          return;

        /* for some non-standard fonts like `Optima' which provides */
        /* different outlines depending on the resolution it is     */
        /* possible to get here twice                               */
        if ( loader->num_glyphs )
          continue;

        error = T1_Add_Table( name_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        /* add a trailing zero to the name table */
        name_table->elements[n][len] = '\0';

        /* record index of /.notdef */
        if ( *cur == '.'                                              &&
             ft_strcmp( ".notdef",
                        (const char*)(name_table->elements[n]) ) == 0 )
        {
          notdef_index = n;
          notdef_found = 1;
        }

        if ( face->type1.private_dict.lenIV >= 0 &&
             n < num_glyphs + TABLE_EXTEND       )
        {
          FT_Byte*  temp = NULL;


          if ( size <= (FT_ULong)face->type1.private_dict.lenIV )
          {
            error = FT_THROW( Invalid_File_Format );
            goto Fail;
          }

          /* t1_decrypt() shouldn't write to base -- make temporary copy */
          if ( FT_ALLOC( temp, size ) )
            goto Fail;
          FT_MEM_COPY( temp, base, size );
          psaux->t1_decrypt( temp, size, 4330 );
          size -= (FT_ULong)face->type1.private_dict.lenIV;
          error = T1_Add_Table( code_table, n,
                                temp + face->type1.private_dict.lenIV, size );
          FT_FREE( temp );
        }
        else
          error = T1_Add_Table( code_table, n, base, size );
        if ( error )
          goto Fail;

        n++;
      }
    }

    if ( !n )
    {
      error = FT_THROW( Invalid_File_Format );
      goto Fail;
    }

    loader->num_glyphs = n;

    /* if /.notdef is found but does not occupy index 0, do our magic. */
    if ( notdef_found                                                 &&
         ft_strcmp( ".notdef", (const char*)name_table->elements[0] ) )
    {
      /* Swap glyph in index 0 with /.notdef glyph.  First, add index 0  */
      /* name and code entries to swap_table.  Then place notdef_index   */
      /* name and code entries into swap_table.  Then swap name and code */
      /* entries at indices notdef_index and 0 using values stored in    */
      /* swap_table.                                                     */

      /* Index 0 name */
      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      /* Index 0 code */
      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      /* Index notdef_index name */
      error = T1_Add_Table( swap_table, 2,
                            name_table->elements[notdef_index],
                            name_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      /* Index notdef_index code */
      error = T1_Add_Table( swap_table, 3,
                            code_table->elements[notdef_index],
                            code_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, notdef_index,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, notdef_index,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0,
                            swap_table->elements[2],
                            swap_table->lengths [2] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0,
                            swap_table->elements[3],
                            swap_table->lengths [3] );
      if ( error )
        goto Fail;

    }
    else if ( !notdef_found )
    {
      /* notdef_index is already 0, or /.notdef is undefined in   */
      /* charstrings dictionary.  Worry about /.notdef undefined. */
      /* We take index 0 and add it to the end of the table(s)    */
      /* and add our own /.notdef glyph to index 0.               */

      /* 0 333 hsbw endchar */
      FT_Byte  notdef_glyph[] = { 0x8B, 0xF7, 0xE1, 0x0D, 0x0E };
      char*    notdef_name    = (char *)".notdef";


      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0, notdef_name, 8 );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0, notdef_glyph, 5 );

      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, n,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, n,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      /* we added a glyph. */
      loader->num_glyphs += 1;
    }

    return;

  Fail:
    parser->root.error = error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Define the token field static variables.  This is a set of            */
  /* T1_FieldRec variables.                                                */
  /*                                                                       */
  /*************************************************************************/


  static
  const T1_FieldRec  t1_keywords[] =
  {

#include "t1tokens.h"

    /* now add the special functions... */
    T1_FIELD_CALLBACK( "FontMatrix",           t1_parse_font_matrix,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "Encoding",             parse_encoding,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "Subrs",                parse_subrs,
                       T1_FIELD_DICT_PRIVATE )
    T1_FIELD_CALLBACK( "CharStrings",          parse_charstrings,
                       T1_FIELD_DICT_PRIVATE )
    T1_FIELD_CALLBACK( "Private",              parse_private,
                       T1_FIELD_DICT_FONTDICT )

    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0, 0 }
  };


  static FT_Error
  parse_dict( T1_Face    face,
              T1_Loader  loader,
              FT_Byte*   base,
              FT_ULong   size )
  {
    T1_Parser  parser = &loader->parser;
    FT_Byte   *limit, *start_binary = NULL;
    FT_Bool    have_integer = 0;


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = FT_Err_Ok;

    limit = parser->root.limit;

    T1_Skip_Spaces( parser );

    while ( parser->root.cursor < limit )
    {
      FT_Byte*  cur;


      cur = parser->root.cursor;

      /* look for `eexec' */
      if ( IS_PS_TOKEN( cur, limit, "eexec" ) )
        break;

      /* look for `closefile' which ends the eexec section */
      else if ( IS_PS_TOKEN( cur, limit, "closefile" ) )
        break;

      /* in a synthetic font the base font starts after a           */
      /* `FontDictionary' token that is placed after a Private dict */
      else if ( IS_PS_TOKEN( cur, limit, "FontDirectory" ) )
      {
        if ( loader->keywords_encountered & T1_PRIVATE )
          loader->keywords_encountered |=
            T1_FONTDIR_AFTER_PRIVATE;
        parser->root.cursor += 13;
      }

      /* check whether we have an integer */
      else if ( ft_isdigit( *cur ) )
      {
        start_binary = cur;
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;
        have_integer = 1;
      }

      /* in valid Type 1 fonts we don't see `RD' or `-|' directly */
      /* since those tokens are handled by parse_subrs and        */
      /* parse_charstrings                                        */
      else if ( *cur == 'R' && cur + 6 < limit && *(cur + 1) == 'D' &&
                have_integer )
      {
        FT_ULong  s;
        FT_Byte*  b;


        parser->root.cursor = start_binary;
        if ( !read_binary_data( parser, &s, &b, IS_INCREMENTAL ) )
          return FT_THROW( Invalid_File_Format );
        have_integer = 0;
      }

      else if ( *cur == '-' && cur + 6 < limit && *(cur + 1) == '|' &&
                have_integer )
      {
        FT_ULong  s;
        FT_Byte*  b;


        parser->root.cursor = start_binary;
        if ( !read_binary_data( parser, &s, &b, IS_INCREMENTAL ) )
          return FT_THROW( Invalid_File_Format );
        have_integer = 0;
      }

      /* look for immediates */
      else if ( *cur == '/' && cur + 2 < limit )
      {
        FT_UInt  len;


        cur++;

        parser->root.cursor = cur;
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;

        len = (FT_UInt)( parser->root.cursor - cur );

        if ( len > 0 && len < 22 && parser->root.cursor < limit )
        {
          /* now compare the immediate name to the keyword table */
          T1_Field  keyword = (T1_Field)t1_keywords;


          for (;;)
          {
            FT_Byte*  name;


            name = (FT_Byte*)keyword->ident;
            if ( !name )
              break;

            if ( cur[0] == name[0]                      &&
                 len == ft_strlen( (const char *)name ) &&
                 ft_memcmp( cur, name, len ) == 0       )
            {
              /* We found it -- run the parsing callback!     */
              /* We record every instance of every field      */
              /* (until we reach the base font of a           */
              /* synthetic font) to deal adequately with      */
              /* multiple master fonts; this is also          */
              /* necessary because later PostScript           */
              /* definitions override earlier ones.           */

              /* Once we encounter `FontDirectory' after      */
              /* `/Private', we know that this is a synthetic */
              /* font; except for `/CharStrings' we are not   */
              /* interested in anything that follows this     */
              /* `FontDirectory'.                             */

              /* MM fonts have more than one /Private token at */
              /* the top level; let's hope that all the junk   */
              /* that follows the first /Private token is not  */
              /* interesting to us.                            */

              /* According to Adobe Tech Note #5175 (CID-Keyed */
              /* Font Installation for ATM Software) a `begin' */
              /* must be followed by exactly one `end', and    */
              /* `begin' -- `end' pairs must be accurately     */
              /* paired.  We could use this to distinguish     */
              /* between the global Private and the Private    */
              /* dict that is a member of the Blend dict.      */

              const FT_UInt dict =
                ( loader->keywords_encountered & T1_PRIVATE )
                    ? T1_FIELD_DICT_PRIVATE
                    : T1_FIELD_DICT_FONTDICT;

              if ( !( dict & keyword->dict ) )
              {
                FT_TRACE1(( "parse_dict: found `%s' but ignoring it"
                            " since it is in the wrong dictionary\n",
                            keyword->ident ));
                break;
              }

              if ( !( loader->keywords_encountered &
                      T1_FONTDIR_AFTER_PRIVATE     )                  ||
                   ft_strcmp( (const char*)name, "CharStrings" ) == 0 )
              {
                parser->root.error = t1_load_keyword( face,
                                                      loader,
                                                      keyword );
                if ( parser->root.error )
                {
                  if ( FT_ERR_EQ( parser->root.error, Ignore ) )
                    parser->root.error = FT_Err_Ok;
                  else
                    return parser->root.error;
                }
              }
              break;
            }

            keyword++;
          }
        }

        have_integer = 0;
      }
      else
      {
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;
        have_integer = 0;
      }

      T1_Skip_Spaces( parser );
    }

  Exit:
    return parser->root.error;
  }


  static void
  t1_init_loader( T1_Loader  loader,
                  T1_Face    face )
  {
    FT_UNUSED( face );

    FT_ZERO( loader );
  }


  static void
  t1_done_loader( T1_Loader  loader )
  {
    T1_Parser  parser = &loader->parser;
    FT_Memory  memory = parser->root.memory;


    /* finalize tables */
    T1_Release_Table( &loader->encoding_table );
    T1_Release_Table( &loader->charstrings );
    T1_Release_Table( &loader->glyph_names );
    T1_Release_Table( &loader->swap_table );
    T1_Release_Table( &loader->subrs );

    /* finalize hash */
    ft_hash_num_free( loader->subrs_hash, memory );
    FT_FREE( loader->subrs_hash );

    /* finalize parser */
    T1_Finalize_Parser( parser );
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Open_Face( T1_Face  face )
  {
    T1_LoaderRec   loader;
    T1_Parser      parser;
    T1_Font        type1 = &face->type1;
    PS_Private     priv  = &type1->private_dict;
    FT_Error       error;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    t1_init_loader( &loader, face );

    /* default values */
    face->ndv_idx          = -1;
    face->cdv_idx          = -1;
    face->len_buildchar    = 0;

    priv->blue_shift       = 7;
    priv->blue_fuzz        = 1;
    priv->lenIV            = 4;
    priv->expansion_factor = (FT_Fixed)( 0.06 * 0x10000L );
    priv->blue_scale       = (FT_Fixed)( 0.039625 * 0x10000L * 1000 );

    parser = &loader.parser;
    error  = T1_New_Parser( parser,
                            face->root.stream,
                            face->root.memory,
                            psaux );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader,
                        parser->base_dict, parser->base_len );
    if ( error )
      goto Exit;

    error = T1_Get_Private_Dict( parser, psaux );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader,
                        parser->private_dict, parser->private_len );
    if ( error )
      goto Exit;

    /* ensure even-ness of `num_blue_values' */
    priv->num_blue_values &= ~1;

    /* now, propagate the subrs, charstrings, and glyphnames tables */
    /* to the Type1 data                                            */
    type1->num_glyphs = loader.num_glyphs;

    if ( loader.subrs.init )
    {
      type1->num_subrs   = loader.num_subrs;
      type1->subrs_block = loader.subrs.block;
      type1->subrs       = loader.subrs.elements;
      type1->subrs_len   = loader.subrs.lengths;
      type1->subrs_hash  = loader.subrs_hash;

      /* prevent `t1_done_loader' from freeing the propagated data */
      loader.subrs.init = 0;
      loader.subrs_hash = NULL;
    }

    if ( !IS_INCREMENTAL )
      if ( !loader.charstrings.init )
      {
        FT_ERROR(( "T1_Open_Face: no `/CharStrings' array in face\n" ));
        error = FT_THROW( Invalid_File_Format );
      }

    loader.charstrings.init  = 0;
    type1->charstrings_block = loader.charstrings.block;
    type1->charstrings       = loader.charstrings.elements;
    type1->charstrings_len   = loader.charstrings.lengths;

    /* we copy the glyph names `block' and `elements' fields; */
    /* the `lengths' field must be released later             */
    type1->glyph_names_block    = loader.glyph_names.block;
    type1->glyph_names          = (FT_String**)loader.glyph_names.elements;
    loader.glyph_names.block    = NULL;
    loader.glyph_names.elements = NULL;

    /* we must now build type1.encoding when we have a custom array */
    if ( type1->encoding_type == T1_ENCODING_TYPE_ARRAY )
    {
      FT_Int    charcode, idx, min_char, max_char;
      FT_Byte*  glyph_name;


      /* OK, we do the following: for each element in the encoding  */
      /* table, look up the index of the glyph having the same name */
      /* the index is then stored in type1.encoding.char_index, and */
      /* the name to type1.encoding.char_name                       */

      min_char = 0;
      max_char = 0;

      charcode = 0;
      for ( ; charcode < loader.encoding_table.max_elems; charcode++ )
      {
        FT_Byte*  char_name;


        type1->encoding.char_index[charcode] = 0;
        type1->encoding.char_name [charcode] = (char *)".notdef";

        char_name = loader.encoding_table.elements[charcode];
        if ( char_name )
          for ( idx = 0; idx < type1->num_glyphs; idx++ )
          {
            glyph_name = (FT_Byte*)type1->glyph_names[idx];
            if ( ft_strcmp( (const char*)char_name,
                            (const char*)glyph_name ) == 0 )
            {
              type1->encoding.char_index[charcode] = (FT_UShort)idx;
              type1->encoding.char_name [charcode] = (char*)glyph_name;

              /* Change min/max encoded char only if glyph name is */
              /* not /.notdef                                      */
              if ( ft_strcmp( (const char*)".notdef",
                              (const char*)glyph_name ) != 0 )
              {
                if ( charcode < min_char )
                  min_char = charcode;
                if ( charcode >= max_char )
                  max_char = charcode + 1;
              }
              break;
            }
          }
      }

      type1->encoding.code_first = min_char;
      type1->encoding.code_last  = max_char;
      type1->encoding.num_chars  = loader.num_chars;
    }

    /* some sanitizing to avoid overflows later on; */
    /* the upper limits are ad-hoc values           */
    if ( priv->blue_shift > 1000 || priv->blue_shift < 0 )
    {
      FT_TRACE2(( "T1_Open_Face:"
                  " setting unlikely BlueShift value %d to default (7)\n",
                  priv->blue_shift ));
      priv->blue_shift = 7;
    }

    if ( priv->blue_fuzz > 1000 || priv->blue_fuzz < 0 )
    {
      FT_TRACE2(( "T1_Open_Face:"
                  " setting unlikely BlueFuzz value %d to default (1)\n",
                  priv->blue_fuzz ));
      priv->blue_fuzz = 1;
    }

  Exit:
    t1_done_loader( &loader );
    return error;
  }


/* END */
