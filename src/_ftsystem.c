/***************************************************************************/
/*                                                                         */
/*  ftsystem.c                                                             */
/*                                                                         */
/*    ANSI-specific FreeType low-level system interface (body).            */
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
  /* This file contains the default interface used by FreeType to access   */
  /* low-level, i.e. memory management, i/o access as well as thread       */
  /* synchronisation.  It can be replaced by user-specific routines if     */
  /* necessary.                                                            */
  /*                                                                       */
  /*************************************************************************/


#include "freetype/ft2build.h"
#include FT_CONFIG_CONFIG_H_FT
#include FT_INTERNAL_DEBUG_H_FT
#include FT_INTERNAL_STREAM_H_FT
#include FT_SYSTEM_H_FT
#include FT_ERRORS_H_FT
#include FT_TYPES_H_FT


  /*************************************************************************/
  /*                                                                       */
  /*                     RESOURCE MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/

#ifndef FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_io

  /* We use the macro STREAM_FILE for convenience to extract the       */
  /* system-specific stream handle from a given FreeType stream object */
#define STREAM_FILE( stream )  ( (FT_FILE*)stream->descriptor.pointer )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_ansi_stream_close                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to close a stream.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  FT_CALLBACK_DEF( void )
  ft_ansi_stream_close( FT_Stream  stream )
  {
    ft_fclose( STREAM_FILE( stream ) );

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = NULL;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_ansi_stream_io                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to open a stream.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  /*    offset :: The position in the data stream to start reading.        */
  /*                                                                       */
  /*    buffer :: The address of buffer to store the read data.            */
  /*                                                                       */
  /*    count  :: The number of bytes to read from the stream.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The number of bytes actually read.  If `count' is zero (this is,   */
  /*    the function is used for seeking), a non-zero return value         */
  /*    indicates an error.                                                */
  /*                                                                       */
  FT_CALLBACK_DEF( unsigned long )
  ft_ansi_stream_io( FT_Stream       stream,
                     unsigned long   offset,
                     unsigned char*  buffer,
                     unsigned long   count )
  {
    FT_FILE*  file;


    if ( !count && offset > stream->size )
      return 1;

    file = STREAM_FILE( stream );

    if ( stream->pos != offset )
      ft_fseek( file, (long)offset, SEEK_SET );

    return (unsigned long)ft_fread( buffer, 1, count, file );
  }


  /* documentation is in ftstream.h */

  FT_BASE_DEF( FT_Error )
  FT_Stream_Open( FT_Stream    stream,
                  const char*  filepathname )
  {
    FT_FILE*  file;


    if ( !stream )
      return FT_THROW( Invalid_Stream_Handle );

    stream->descriptor.pointer = NULL;
    stream->pathname.pointer   = (char*)filepathname;
    stream->base               = NULL;
    stream->pos                = 0;
    stream->read               = NULL;
    stream->close              = NULL;

    file = ft_fopen( filepathname, "rb" );
    if ( !file )
    {
      FT_ERROR(( "FT_Stream_Open:"
                 " could not open `%s'\n", filepathname ));

      return FT_THROW( Cannot_Open_Resource );
    }

    ft_fseek( file, 0, SEEK_END );
    stream->size = (unsigned long)ft_ftell( file );
    if ( !stream->size )
    {
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " opened `%s' but zero-sized\n", filepathname ));
      ft_fclose( file );
      return FT_THROW( Cannot_Open_Stream );
    }
    ft_fseek( file, 0, SEEK_SET );

    stream->descriptor.pointer = file;
    stream->read  = ft_ansi_stream_io;
    stream->close = ft_ansi_stream_close;

    FT_TRACE1(( "FT_Stream_Open:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;
  }

#endif /* !FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT */
/* END */
