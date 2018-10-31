/***************************************************************************/
/*                                                                         */
/*  ftmac.h                                                                */
/*                                                                         */
/*    Additional Mac-specific API.                                         */
/*                                                                         */
/*  Copyright 1996-2018 by                                                 */
/*  Just van Rossum, David Turner, Robert Wilhelm, and Werner Lemberg.     */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


/***************************************************************************/
/*                                                                         */
/* NOTE: Include this file after FT_FREETYPE_H and after any               */
/*       Mac-specific headers (because this header uses Mac types such as  */
/*       Handle, FSSpec, FSRef, etc.)                                      */
/*                                                                         */
/***************************************************************************/


#ifndef FTMAC_H_
#define FTMAC_H_


#include "ft2build.h"


FT_BEGIN_HEADER


  /* gcc-3.1 and later can warn about functions tagged as deprecated */
#ifndef FT_DEPRECATED_ATTRIBUTE
#if defined( __GNUC__ )                                     && \
    ( ( __GNUC__ >= 4 )                                  ||    \
      ( ( __GNUC__ == 3 ) && ( __GNUC_MINOR__ >= 1 ) ) )
#define FT_DEPRECATED_ATTRIBUTE  __attribute__(( deprecated ))
#else
#define FT_DEPRECATED_ATTRIBUTE
#endif
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    mac_specific                                                       */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Mac Specific Interface                                             */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    Only available on the Macintosh.                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The following definitions are only available if FreeType is        */
  /*    compiled on a Macintosh.                                           */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Face_From_FOND                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Create a new face object from a FOND resource.                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    fond       :: A FOND resource.                                     */
  /*                                                                       */
  /*    face_index :: Only supported for the -1 `sanity check' special     */
  /*                  case.                                                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0~means success.                             */
  /*                                                                       */
  /* <Notes>                                                               */
  /*    This function can be used to create @FT_Face objects from fonts    */
  /*    that are installed in the system as follows.                       */
  /*                                                                       */
  /*    {                                                                  */
  /*      fond = GetResource( 'FOND', fontName );                          */
  /*      error = FT_New_Face_From_FOND( library, fond, 0, &face );        */
  /*    }                                                                  */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Face_From_FOND( FT_Library  library,
                         Handle      fond,
                         FT_Long     face_index,
                         FT_Face    *aface )
                       FT_DEPRECATED_ATTRIBUTE;
  /* */


FT_END_HEADER


#endif /* FTMAC_H_ */


/* END */
