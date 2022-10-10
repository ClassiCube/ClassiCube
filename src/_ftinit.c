/***************************************************************************/
/*                                                                         */
/*  ftinit.c                                                               */
/*                                                                         */
/*    FreeType initialization layer (body).                                */
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
  /*  The purpose of this file is to implement the following two           */
  /*  functions:                                                           */
  /*                                                                       */
  /*  FT_Add_Default_Modules():                                            */
  /*     This function is used to add the set of default modules to a      */
  /*     fresh new library object.  The set is taken from the header file  */
  /*     `freetype/config/ftmodule.h'.  See the document `FreeType 2.0     */
  /*     Build System' for more information.                               */
  /*                                                                       */
  /*  FT_Init_FreeType():                                                  */
  /*     This function creates a system object for the current platform,   */
  /*     builds a library out of it, then calls FT_Default_Drivers().      */
  /*                                                                       */
  /*  Note that even if FT_Init_FreeType() uses the implementation of the  */
  /*  system object defined at build time, client applications are still   */
  /*  able to provide their own `ftsystem.c'.                              */
  /*                                                                       */
  /*************************************************************************/

#include "Core.h"
#ifdef CC_BUILD_FREETYPE
#include "freetype/ft2build.h"
#include "freetype/ftconfig.h"
#include "freetype/ftobjs.h"
#include "freetype/ftdebug.h"
#include "freetype/ftmodapi.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_init


#undef  FT_USE_MODULE
#ifdef __cplusplus
#define FT_USE_MODULE( type, x )  extern "C" const type  x;
#else
#define FT_USE_MODULE( type, x )  extern const type  x;
#endif

#include "freetype/ftmodule.h"

#undef  FT_USE_MODULE
#define FT_USE_MODULE( type, x )  (const FT_Module_Class*)&(x),

  static
  const FT_Module_Class*  const ft_default_modules[] =
  {
#include "freetype/ftmodule.h"
    0
  };


  /* documentation is in ftmodapi.h */

  FT_EXPORT_DEF( void )
  FT_Add_Default_Modules( FT_Library  library )
  {
    FT_Error                       error;
    const FT_Module_Class* const*  cur;

    /* GCC 4.6 warns the type difference:
     *   FT_Module_Class** != const FT_Module_Class* const*
     */
    cur = (const FT_Module_Class* const*)ft_default_modules;

    /* test for valid `library' delayed to FT_Add_Module() */
    while ( *cur )
    {
      error = FT_Add_Module( library, *cur );
      /* notify errors, but don't stop */
      if ( error )
        FT_TRACE0(( "FT_Add_Default_Module:"
                    " Cannot install `%s', error = 0x%x\n",
                    (*cur)->module_name, error ));
      cur++;
    }
  }

#endif
/* END */
