/***************************************************************************/
/*                                                                         */
/*  t1decode.c                                                             */
/*                                                                         */
/*    PostScript Type 1 decoding routines (body).                          */
/*                                                                         */
/*  Copyright 2000-2018 by                                                 */
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
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_INTERNAL_HASH_H
#include FT_OUTLINE_H

#include "t1decode.h"
#include "psobjs.h"

#include "psauxerr.h"

/* ensure proper sign extension */
#define Fix2Int( f )  ( (FT_Int)(FT_Short)( (f) >> 16 ) )

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1decode


  typedef enum  T1_Operator_
  {
    op_none = 0,
    op_endchar,
    op_hsbw,
    op_seac,
    op_sbw,
    op_closepath,
    op_hlineto,
    op_hmoveto,
    op_hvcurveto,
    op_rlineto,
    op_rmoveto,
    op_rrcurveto,
    op_vhcurveto,
    op_vlineto,
    op_vmoveto,
    op_dotsection,
    op_hstem,
    op_hstem3,
    op_vstem,
    op_vstem3,
    op_div,
    op_callothersubr,
    op_callsubr,
    op_pop,
    op_return,
    op_setcurrentpoint,
    op_unknown15,

    op_max    /* never remove this one */

  } T1_Operator;


  static
  const FT_Int  t1_args_count[op_max] =
  {
    0, /* none */
    0, /* endchar */
    2, /* hsbw */
    5, /* seac */
    4, /* sbw */
    0, /* closepath */
    1, /* hlineto */
    1, /* hmoveto */
    4, /* hvcurveto */
    2, /* rlineto */
    2, /* rmoveto */
    6, /* rrcurveto */
    4, /* vhcurveto */
    1, /* vlineto */
    1, /* vmoveto */
    0, /* dotsection */
    2, /* hstem */
    6, /* hstem3 */
    2, /* vstem */
    6, /* vstem3 */
    2, /* div */
   -1, /* callothersubr */
    1, /* callsubr */
    0, /* pop */
    0, /* return */
    2, /* setcurrentpoint */
    2  /* opcode 15 (undocumented and obsolete) */
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_lookup_glyph_by_stdcharcode_ps                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks up a given glyph by its StandardEncoding charcode.  Used to  */
  /*    implement the SEAC Type 1 operator in the Adobe engine             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: The current face object.                               */
  /*                                                                       */
  /*    charcode :: The character code to look for.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A glyph index in the font face.  Returns -1 if the corresponding   */
  /*    glyph wasn't found.                                                */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Int )
  t1_lookup_glyph_by_stdcharcode_ps( PS_Decoder*  decoder,
                                     FT_Int       charcode )
  {
    FT_UInt             n;
    const FT_String*    glyph_name;
    FT_Service_PsCMaps  psnames = decoder->psnames;


    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    glyph_name = psnames->adobe_std_strings(
                   psnames->adobe_std_encoding[charcode]);

    for ( n = 0; n < decoder->num_glyphs; n++ )
    {
      FT_String*  name = (FT_String*)decoder->glyph_names[n];


      if ( name                               &&
           name[0] == glyph_name[0]           &&
           ft_strcmp( name, glyph_name ) == 0 )
        return (FT_Int)n;
    }

    return -1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_decoder_parse_metrics                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 1 charstrings program to extract width         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /*    charstring_base :: The base address of the charstring stream.      */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_metrics( T1_Decoder  decoder,
                            FT_Byte*    charstring_base,
                            FT_UInt     charstring_len )
  {
    T1_Decoder_Zone  zone;
    FT_Byte*         ip;
    FT_Byte*         limit;
    T1_Builder       builder = &decoder->builder;


    /* First of all, initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;

    builder->parse_state = T1_Parse_Start;

    FT_TRACE4(( "\n"
                "Start charstring: get width\n" ));

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    /* now, execute loop */
    while ( ip < limit )
    {
      FT_Long*     top   = decoder->top;
      T1_Operator  op    = op_none;
      FT_Int32     value = 0;


      /*********************************************************************/
      /*                                                                   */
      /* Decode operator or operand                                        */
      /*                                                                   */
      /*                                                                   */

      /* first of all, decompress operator or value */
      switch ( *ip++ )
      {
      case 1:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 14:
      case 15:
      case 21:
      case 22:
      case 30:
      case 31:
        goto No_Width;

      case 13:
        op = op_hsbw;
        break;

      case 12:
        if ( ip >= limit )
        {
          FT_ERROR(( "t1_decoder_parse_metrics:"
                     " invalid escape (12+EOF)\n" ));
          goto Syntax_Error;
        }

        switch ( *ip++ )
        {
        case 7:
          op = op_sbw;
          break;

        default:
          goto No_Width;
        }
        break;

      case 255:    /* four bytes integer */
        if ( ip + 4 > limit )
        {
          FT_ERROR(( "t1_decoder_parse_metrics:"
                     " unexpected EOF in integer\n" ));
          goto Syntax_Error;
        }

        value = (FT_Int32)( ( (FT_UInt32)ip[0] << 24 ) |
                            ( (FT_UInt32)ip[1] << 16 ) |
                            ( (FT_UInt32)ip[2] << 8  ) |
                              (FT_UInt32)ip[3]         );
        ip += 4;

        /* According to the specification, values > 32000 or < -32000 must */
        /* be followed by a `div' operator to make the result be in the    */
        /* range [-32000;32000].  We expect that the second argument of    */
        /* `div' is not a large number.  Additionally, we don't handle     */
        /* stuff like `<large1> <large2> <num> div <num> div' or           */
        /* <large1> <large2> <num> div div'.  This is probably not allowed */
        /* anyway.                                                         */
        if ( value > 32000 || value < -32000 )
        {
          FT_ERROR(( "t1_decoder_parse_metrics:"
                     " large integer found for width\n" ));
          goto Syntax_Error;
        }
        else
        {
          value = (FT_Int32)( (FT_UInt32)value << 16 );
        }

        break;

      default:
        if ( ip[-1] >= 32 )
        {
          if ( ip[-1] < 247 )
            value = (FT_Int32)ip[-1] - 139;
          else
          {
            if ( ++ip > limit )
            {
              FT_ERROR(( "t1_decoder_parse_metrics:"
                         " unexpected EOF in integer\n" ));
              goto Syntax_Error;
            }

            if ( ip[-2] < 251 )
              value =    ( ( ip[-2] - 247 ) * 256 ) + ip[-1] + 108;
            else
              value = -( ( ( ip[-2] - 251 ) * 256 ) + ip[-1] + 108 );
          }

          value = (FT_Int32)( (FT_UInt32)value << 16 );
        }
        else
        {
          FT_ERROR(( "t1_decoder_parse_metrics:"
                     " invalid byte (%d)\n", ip[-1] ));
          goto Syntax_Error;
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /*  Push value on stack, or process operator                         */
      /*                                                                   */
      /*                                                                   */
      if ( op == op_none )
      {
        if ( top - decoder->stack >= T1_MAX_CHARSTRINGS_OPERANDS )
        {
          FT_ERROR(( "t1_decoder_parse_metrics: stack overflow\n" ));
          goto Syntax_Error;
        }

        *top++       = value;
        decoder->top = top;
      }
      else  /* general operator */
      {
        FT_Int  num_args = t1_args_count[op];


        FT_ASSERT( num_args >= 0 );

        if ( top - decoder->stack < num_args )
          goto Stack_Underflow;

        top -= num_args;

        switch ( op )
        {
        case op_hsbw:
          FT_TRACE4(( " hsbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x = ADD_LONG( builder->left_bearing.x,
                                              top[0] );

          builder->advance.x = top[1];
          builder->advance.y = 0;

          /* we only want to compute the glyph's metrics */
          /* (lsb + advance width), not load the rest of */
          /* it; so exit immediately                     */
          return FT_Err_Ok;

        case op_sbw:
          FT_TRACE4(( " sbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x = ADD_LONG( builder->left_bearing.x,
                                              top[0] );
          builder->left_bearing.y = ADD_LONG( builder->left_bearing.y,
                                              top[1] );

          builder->advance.x = top[2];
          builder->advance.y = top[3];

          /* we only want to compute the glyph's metrics */
          /* (lsb + advance width), not load the rest of */
          /* it; so exit immediately                     */
          return FT_Err_Ok;

        default:
          FT_ERROR(( "t1_decoder_parse_metrics:"
                     " unhandled opcode %d\n", op ));
          goto Syntax_Error;
        }

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));

  No_Width:
    FT_ERROR(( "t1_decoder_parse_metrics:"
               " no width, found op %d instead\n",
               ip[-1] ));
  Syntax_Error:
    return FT_THROW( Syntax_Error );

  Stack_Underflow:
    return FT_THROW( Stack_Underflow );
  }


  /* initialize T1 decoder */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_init( T1_Decoder           decoder,
                   FT_Face              face,
                   FT_Size              size,
                   FT_GlyphSlot         slot,
                   FT_Byte**            glyph_names,
                   PS_Blend             blend,
                   FT_Bool              hinting,
                   FT_Render_Mode       hint_mode,
                   T1_Decoder_Callback  parse_callback )
  {
    FT_ZERO( decoder );

    /* retrieve PSNames interface from list of current modules */
    {
      FT_Service_PsCMaps  psnames;


      FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
      if ( !psnames )
      {
        FT_ERROR(( "t1_decoder_init:"
                   " the `psnames' module is not available\n" ));
        return FT_THROW( Unimplemented_Feature );
      }

      decoder->psnames = psnames;
    }

    t1_builder_init( &decoder->builder, face, size, slot, hinting );

    /* decoder->buildchar and decoder->len_buildchar have to be  */
    /* initialized by the caller since we cannot know the length */
    /* of the BuildCharArray                                     */

    decoder->num_glyphs     = (FT_UInt)face->num_glyphs;
    decoder->glyph_names    = glyph_names;
    decoder->hint_mode      = hint_mode;
    decoder->blend          = blend;
    decoder->parse_callback = parse_callback;

    decoder->funcs          = t1_decoder_funcs;

    return FT_Err_Ok;
  }


  /* finalize T1 decoder */
  FT_LOCAL_DEF( void )
  t1_decoder_done( T1_Decoder  decoder )
  {
    FT_Memory  memory = decoder->builder.memory;


    t1_builder_done( &decoder->builder );

    if ( decoder->cf2_instance.finalizer )
    {
      decoder->cf2_instance.finalizer( decoder->cf2_instance.data );
      FT_FREE( decoder->cf2_instance.data );
    }
  }


/* END */
