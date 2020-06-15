/***************************************************************************/
/*                                                                         */
/*  t1driver.c                                                             */
/*                                                                         */
/*    Type 1 driver interface (body).                                      */
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


#include "ft2build.h"
#include "t1driver.h"
#include "t1gload.h"
#include "t1load.h"

#include "t1errors.h"

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_HASH_H
#include FT_DRIVER_H

#include FT_SERVICE_GLYPH_DICT_H
#include FT_SERVICE_FONT_FORMAT_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1driver

 /*
  *  GLYPH DICT SERVICE
  *
  */

  static FT_Error
  t1_get_glyph_name( T1_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max )
  {
    FT_STRCPYN( buffer, face->type1.glyph_names[glyph_index], buffer_max );

    return FT_Err_Ok;
  }


  static const FT_Service_GlyphDictRec  t1_service_glyph_dict =
  {
    (FT_GlyphDict_GetNameFunc)  t1_get_glyph_name     /* get_name   */
  };


  /*
   *  SERVICE LIST
   *
   */

  static const FT_ServiceDescRec  t1_services[] =
  {
    { FT_SERVICE_ID_GLYPH_DICT,           &t1_service_glyph_dict },
    { FT_SERVICE_ID_FONT_FORMAT,          FT_FONT_FORMAT_TYPE_1 },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  Get_Interface( FT_Module         module,
                 const FT_String*  t1_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( t1_services, t1_interface );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  t1_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,

      sizeof ( PS_DriverRec ),

      "type1",
      0x10000L,
      0x20000L,

      NULL,    /* module-specific interface */

      T1_Driver_Init,           /* FT_Module_Constructor  module_init   */
      T1_Driver_Done,           /* FT_Module_Destructor   module_done   */
      Get_Interface,            /* FT_Module_Requester    get_interface */
    },

    sizeof ( T1_FaceRec ),
    sizeof ( T1_SizeRec ),
    sizeof ( T1_GlyphSlotRec ),

    T1_Face_Init,               /* FT_Face_InitFunc  init_face */
    T1_Face_Done,               /* FT_Face_DoneFunc  done_face */
    T1_Size_Init,               /* FT_Size_InitFunc  init_size */
    T1_Size_Done,               /* FT_Size_DoneFunc  done_size */
    T1_GlyphSlot_Init,          /* FT_Slot_InitFunc  init_slot */
    T1_GlyphSlot_Done,          /* FT_Slot_DoneFunc  done_slot */

    T1_Load_Glyph,              /* FT_Slot_LoadFunc  load_glyph */

    NULL,                       /* FT_Face_GetKerningFunc   get_kerning  */
    NULL,                       /* FT_Face_AttachFunc       attach_file  */
    T1_Get_Advances,            /* FT_Face_GetAdvancesFunc  get_advances */

    T1_Size_Request,            /* FT_Size_RequestFunc  request_size */
    NULL                        /* FT_Size_SelectFunc   select_size  */
  };


/* END */
