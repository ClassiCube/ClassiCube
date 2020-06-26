/***************************************************************************/
/*                                                                         */
/*  afmodule.c                                                             */
/*                                                                         */
/*    Auto-fitter module implementation (body).                            */
/*                                                                         */
/*  Copyright 2003-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afglobal.h"
#include "afmodule.h"
#include "afloader.h"
#include "aferrors.h"

#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_DRIVER_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_afmodule


  static const FT_ServiceDescRec  af_services[] =
  {
	  { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  af_get_interface( FT_Module    module,
                    const char*  module_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( af_services, module_interface );
  }


  FT_CALLBACK_DEF( FT_Error )
  af_autofitter_init( FT_Module  ft_module )      /* AF_Module */
  {
    AF_Module  module = (AF_Module)ft_module;


    module->fallback_style    = AF_STYLE_FALLBACK;
    module->default_script    = AF_SCRIPT_DEFAULT;
#ifdef AF_CONFIG_OPTION_USE_WARPER
    module->warping           = 0;
#endif
    module->no_stem_darkening = TRUE;

    module->darken_params[0]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_X1;
    module->darken_params[1]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y1;
    module->darken_params[2]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_X2;
    module->darken_params[3]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y2;
    module->darken_params[4]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_X3;
    module->darken_params[5]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y3;
    module->darken_params[6]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_X4;
    module->darken_params[7]  = CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y4;

    return FT_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  af_autofitter_done( FT_Module  ft_module )      /* AF_Module */
  {
    FT_UNUSED( ft_module );
  }


  FT_CALLBACK_DEF( FT_Error )
  af_autofitter_load_glyph( AF_Module     module,
                            FT_GlyphSlot  slot,
                            FT_Size       size,
                            FT_UInt       glyph_index,
                            FT_Int32      load_flags )
  {
    FT_Error   error  = FT_Err_Ok;
    FT_Memory  memory = module->root.library->memory;

    AF_GlyphHintsRec  hints[1];
    AF_LoaderRec      loader[1];

    FT_UNUSED( size );


    af_glyph_hints_init( hints, memory );
    af_loader_init( loader, hints );

    error = af_loader_load_glyph( loader, module, slot->face,
                                  glyph_index, load_flags );

    af_loader_done( loader );
    af_glyph_hints_done( hints );

    return error;
  }


  FT_DEFINE_AUTOHINTER_INTERFACE(
    af_autofitter_interface,

    NULL,                                                    /* reset_face */
    NULL,                                              /* get_global_hints */
    NULL,                                             /* done_global_hints */
    (FT_AutoHinter_GlyphLoadFunc)af_autofitter_load_glyph )  /* load_glyph */


  FT_DEFINE_MODULE(
    autofit_module_class,

    FT_MODULE_HINTER,
    sizeof ( AF_ModuleRec ),

    "autofitter",
    0x10000L,   /* version 1.0 of the autofitter  */
    0x20000L,   /* requires FreeType 2.0 or above */

    (const void*)&af_autofitter_interface,

    (FT_Module_Constructor)af_autofitter_init,  /* module_init   */
    (FT_Module_Destructor) af_autofitter_done,  /* module_done   */
    (FT_Module_Requester)  af_get_interface     /* get_interface */
  )


/* END */
